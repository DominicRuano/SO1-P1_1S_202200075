#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h> 
#include <linux/init.h>
#include <linux/proc_fs.h> 
#include <linux/seq_file.h> 
#include <linux/mm.h> 
#include <linux/sched.h> 
#include <linux/timer.h> 
#include <linux/jiffies.h> 
#include <linux/uaccess.h>
#include <linux/tty.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>        
#include <linux/slab.h>      
#include <linux/sched/mm.h>
#include <linux/binfmts.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tu Nombre");
MODULE_DESCRIPTION("Modulo para leer informacion de memoria, CPU y disco en JSON");
MODULE_VERSION("1.0");

#define PROC_NAME "sysinfo_202200075" // Cambiar por tu carnet
#define MAX_CMDLINE_LENGTH 256
#define CONTAINER_ID_LENGTH 64

/* Variables globales para seguimiento de uso de CPU */
static unsigned long prev_total_jiffies = 0;
static unsigned long prev_idle_jiffies = 0;

/* Declaración anticipada de funciones */
static int sysinfo_show(struct seq_file *m, void *v);
static int sysinfo_open(struct inode *inode, struct file *file);

/**
 * Obtiene estadísticas de uso de CPU leyendo /proc/stat
 * @param total: puntero donde se almacenará el total de jiffies
 * @param idle: puntero donde se almacenará los jiffies en estado idle
 */
static void get_cpu_usage(unsigned long *total, unsigned long *idle) {
    char buf[256];
    struct file *file;
    loff_t pos = 0;
    ssize_t bytes;
    unsigned long user, nice, system;

    /* Inicializa valores en caso de error */
    *total = 0;
    *idle = 0;

    /* Abre el archivo /proc/stat */
    file = filp_open("/proc/stat", O_RDONLY, 0);
    if (IS_ERR(file)) {
        return;
    }

    /* Lee la primera línea del archivo */
    bytes = kernel_read(file, buf, sizeof(buf) - 1, &pos);
    if (bytes > 0) {
        buf[bytes] = '\0';
        
        /* Parsea los valores de CPU: user, nice, system, idle */
        if (sscanf(buf, "cpu %lu %lu %lu %lu", &user, &nice, &system, idle) >= 4) {
            /* Calcula el total sumando todos los estados */
            *total = user + nice + system + *idle;
        }
    }

    filp_close(file, NULL);
}

/**
 * Función que genera la salida JSON con la información del sistema
 */
static int sysinfo_show(struct seq_file *m, void *v) {
    struct sysinfo si;
    struct task_struct *task;
    unsigned long total_jiffies, idle_jiffies, cpu_usage_system = 0;
    unsigned long mem_total, mem_free, mem_used;
    int first_process = 1;

    /* Obtiene información de memoria */
    si_meminfo(&si);
    mem_total = si.totalram * si.mem_unit / 1024;  // Convertir a KB
    mem_free = si.freeram * si.mem_unit / 1024;    // Convertir a KB
    mem_used = mem_total - mem_free;

    /* Obtiene información de CPU */
    get_cpu_usage(&total_jiffies, &idle_jiffies);
    
    /* Calcula el porcentaje de uso de CPU comparando con valores anteriores */
    if (prev_total_jiffies > 0) {
        unsigned long total_diff = total_jiffies - prev_total_jiffies;
        if (total_diff > 0) {  // Evita división por cero
            unsigned long idle_diff = idle_jiffies - prev_idle_jiffies;
            cpu_usage_system = 100 - (idle_diff * 100 / total_diff);
        }
    }
    
    /* Guarda los valores actuales para la próxima lectura */
    prev_total_jiffies = total_jiffies;
    prev_idle_jiffies = idle_jiffies;

    /* Comienza la generación del JSON */
    seq_printf(m, "{\n");
    
    /* Sección de memoria */
    seq_printf(m, "  \"Memory\": {\n");
    seq_printf(m, "    \"Total\": %lu,\n", mem_total);
    seq_printf(m, "    \"Free\": %lu,\n", mem_free);
    seq_printf(m, "    \"Used\": %lu\n", mem_used);
    seq_printf(m, "  },\n");
    
    /* Sección de CPU */
    seq_printf(m, "  \"CPU\": {\n");
    seq_printf(m, "    \"Usage\": %lu\n", cpu_usage_system);
    seq_printf(m, "  },\n");
    
    /* Sección de procesos */
    seq_printf(m, "  \"Processes\": [\n");

    /* Itera a través de todos los procesos */
    for_each_process(task) {
        if (strstr(task->comm, "docker") || strstr(task->comm, "containerd")) {
            if (!first_process) {
                seq_printf(m, ",\n");
            } else {
                first_process = 0;
            }

            seq_printf(m, "    {\n");
            seq_printf(m, "      \"PID\": %d,\n", task->pid);
            seq_printf(m, "      \"Name\": \"%s\"\n", task->comm);
            seq_printf(m, "    }");
        }
    }
    
    /* Cierra el JSON */
    seq_printf(m, "\n  ]\n}\n");
    
    return 0;
}

/**
 * Función que se llama cuando se abre la entrada de /proc
 */
static int sysinfo_open(struct inode *inode, struct file *file) {
    return single_open(file, sysinfo_show, NULL);
}

/* Define las operaciones de archivo para la entrada de /proc usando file_operations */
static const struct file_operations sysinfo_ops = {
    .owner = THIS_MODULE,
    .open = sysinfo_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

/**
 * Función de inicialización del módulo
 */
static int __init sysinfo_init(void) {
    struct proc_dir_entry *entry;
    
    /* Crea la entrada en /proc */
    entry = proc_create(PROC_NAME, 0444, NULL, &sysinfo_ops);
    if (!entry) {
        printk(KERN_ERR "No se pudo crear /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "sysinfo_json modulo cargado en /proc/%s\n", PROC_NAME);
    return 0;
}

/**
 * Función de limpieza cuando se descarga el módulo
 */
static void __exit sysinfo_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "sysinfo_json modulo eliminado\n");
}

/* Registra las funciones de inicialización y salida */
module_init(sysinfo_init);
module_exit(sysinfo_exit);
