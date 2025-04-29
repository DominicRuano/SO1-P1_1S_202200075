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
static unsigned long prev_user = 0;
static unsigned long prev_nice = 0;
static unsigned long prev_system = 0;
static unsigned long prev_idle = 0;
static unsigned long prev_iowait = 0;
static unsigned long prev_irq = 0;
static unsigned long prev_softirq = 0;
static unsigned long prev_steal = 0;

/* Declaración anticipada de funciones */
static int sysinfo_show(struct seq_file *m, void *v);
static int sysinfo_open(struct inode *inode, struct file *file);

// Función para obtener la línea de comandos de un proceso y retornar un apuntador a la cadena
static char *get_process_cmdline(struct task_struct *task) {

    /* 
        Creamos una estructura mm_struct para obtener la información de memoria
        Creamos un apuntador char para la línea de comandos
        Creamos un apuntador char para recorrer la línea de comandos
        Creamos variables para guardar las direcciones de inicio y fin de los argumentos y el entorno
        Creamos variables para recorrer la línea de comandos
    */
    struct mm_struct *mm;
    char *cmdline, *p;
    unsigned long arg_start, arg_end, env_start;
    int i, len;


    // Reservamos memoria para la línea de comandos
    cmdline = kmalloc(MAX_CMDLINE_LENGTH, GFP_KERNEL);
    if (!cmdline)
        return NULL;

    // Obtenemos la información de memoria
    mm = get_task_mm(task);
    if (!mm) {
        kfree(cmdline);
        return NULL;
    }

    /* 
       1. Primero obtenemos el bloqueo de lectura de la estructura mm_struct para una lectura segura
       2. Obtenemos las direcciones de inicio y fin de los argumentos y el entorno
       3. Liberamos el bloqueo de lectura de la estructura mm_struct
    */
    down_read(&mm->mmap_lock);
    arg_start = mm->arg_start;
    arg_end = mm->arg_end;
    env_start = mm->env_start;
    up_read(&mm->mmap_lock);

    // Obtenemos la longitud de la línea de comandos y validamos que no sea mayor a MAX_CMDLINE_LENGTH - 1
    len = arg_end - arg_start;

    if (len > MAX_CMDLINE_LENGTH - 1)
        len = MAX_CMDLINE_LENGTH - 1;

    // Obtenemos la línea de comandos de  la memoria virtual del proceso
    /* 
        Por qué de la memoria virtual del proceso?
        La memoria virtual es la memoria que un proceso puede direccionar, es decir, la memoria que un proceso puede acceder
    */
    if (access_process_vm(task, arg_start, cmdline, len, 0) != len) {
        mmput(mm);
        kfree(cmdline);
        return NULL;
    }

    // Agregamos un caracter nulo al final de la línea de comandos
    cmdline[len] = '\0';

    // Reemplazar caracteres nulos por espacios
    p = cmdline;
    for (i = 0; i < len; i++)
        if (p[i] == '\0')
            p[i] = ' ';

    // Liberamos la estructura mm_struct
    mmput(mm);
    return cmdline;
}


/**
 * Obtiene estadísticas de uso de CPU leyendo /proc/stat
 * @param cpu_usage: puntero donde se almacenará el porcentaje de uso de CPU
 * @return 1 si el cálculo fue exitoso, 0 en caso contrario
 */
static int get_cpu_usage(unsigned long *cpu_usage) {
    char buf[256];
    struct file *file;
    loff_t pos = 0;
    ssize_t bytes;
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long total_delta, idle_delta;
    unsigned long user_delta, nice_delta, system_delta, iowait_delta;
    unsigned long irq_delta, softirq_delta, steal_delta;

    /* Inicializa el valor por defecto */
    *cpu_usage = 0;

    /* Abre el archivo /proc/stat */
    file = filp_open("/proc/stat", O_RDONLY, 0);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Error al abrir /proc/stat\n");
        return 0;
    }

    /* Lee la primera línea del archivo */
    bytes = kernel_read(file, buf, sizeof(buf) - 1, &pos);
    filp_close(file, NULL);
    
    if (bytes <= 0) {
        printk(KERN_ERR "Error al leer /proc/stat\n");
        return 0;
    }

    buf[bytes] = '\0';
    
    /* Parsea los valores de CPU: user, nice, system, idle, iowait, irq, softirq, steal */
    if (sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 4) {
        printk(KERN_ERR "Error al parsear /proc/stat\n");
        return 0;
    }

    /* Si es la primera ejecución, solo guardar los valores y retornar */
    if (prev_user == 0 && prev_nice == 0 && prev_system == 0 && prev_idle == 0) {
        prev_user = user;
        prev_nice = nice;
        prev_system = system;
        prev_idle = idle;
        prev_iowait = iowait;
        prev_irq = irq;
        prev_softirq = softirq;
        prev_steal = steal;
        /* Retraso corto para asegurar tener muestras diferentes */
        msleep(100);
        return 0;
    }

    /* Calcula los deltas */
    user_delta = user - prev_user;
    nice_delta = nice - prev_nice; 
    system_delta = system - prev_system;
    idle_delta = idle - prev_idle;
    iowait_delta = iowait - prev_iowait;
    irq_delta = irq - prev_irq;
    softirq_delta = softirq - prev_softirq;
    steal_delta = steal - prev_steal;

    /* Calcula el total de tiempo */
    total_delta = user_delta + nice_delta + system_delta + idle_delta + 
                  iowait_delta + irq_delta + softirq_delta + steal_delta;

    /* Tiempo idle total (idle + iowait) */
    idle_delta = idle_delta + iowait_delta;

    /* Actualiza los valores para la próxima lectura */
    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    /* Evita división por cero */
    if (total_delta == 0) {
        return 0;
    }

    /* Calcula el porcentaje de uso de CPU: 100% - porcentaje de tiempo idle */
    *cpu_usage = 100 - ((idle_delta * 100) / total_delta);
    return 1;
}

/**
 * Función que genera la salida JSON con la información del sistema
 */
static int sysinfo_show(struct seq_file *m, void *v) {
    struct sysinfo si;
    struct task_struct *task;
    unsigned long cpu_usage = 0;
    unsigned long mem_total_kb, mem_free_kb, mem_used_kb;
    unsigned long mem_usage = 0, disk_read = 0, disk_write = 0;
    char *cmdline = NULL, cmd_path[64], io_path[64], io_buffer[256] = {0};
    struct file *file, *cmd_file, *io_file;
    loff_t pos = 0;
    ssize_t bytes;
    int first_process = 1;

    /* Obtiene información de memoria */
    si_meminfo(&si);
    /* Convertir páginas a KB (cada página es PAGE_SIZE bytes, normalmente 4KB) */
    mem_total_kb = si.totalram * (PAGE_SIZE / 1024);
    mem_free_kb = si.freeram * (PAGE_SIZE / 1024);
    mem_used_kb = mem_total_kb - mem_free_kb;

    /* Obtiene información de CPU */
    get_cpu_usage(&cpu_usage);

    /* Comienza la generación del JSON */
    seq_printf(m, "{\n");
    
    /* Sección de memoria */
    seq_printf(m, "  \"Memory\": {\n");
    seq_printf(m, "    \"Total\": %lu,\n", mem_total_kb);
    seq_printf(m, "    \"Free\": %lu,\n", mem_free_kb);
    seq_printf(m, "    \"Used\": %lu\n", mem_used_kb);
    seq_printf(m, "  },\n");
    
    /* Sección de CPU */
    seq_printf(m, "  \"CPU\": {\n");
    seq_printf(m, "    \"Usage\": %lu\n", cpu_usage);
    seq_printf(m, "  },\n");
    
    /* Sección de procesos */
    seq_printf(m, "  \"Processes\": [\n");

    /* Itera a través de todos los procesos */
    for_each_process(task) {
        if (strstr(task->comm, "containerd-shim")) {
            if (!first_process) {
                seq_printf(m, ",\n");
            } else {
                first_process = 0;
            }

	    // Obtener la ram asignada al  contenedor
	    unsigned long process_mem_usage = get_mm_rss(task->mm) * PAGE_SIZE / 1024; // Convertir a KB

	    // Obtener información de E/S del disco para el proceso - AGREGAR AQUÍ
	    snprintf(io_path, sizeof(io_path), "/proc/%d/io", task->pid);
	    io_file = filp_open(io_path, O_RDONLY, 0);
	    pos = 0;
	    if (!IS_ERR(io_file)) {
		    kernel_read(io_file, io_buffer, sizeof(io_buffer) - 1, &pos);
		    sscanf(io_buffer, "read_bytes: %lu\nwrite_bytes: %lu", &disk_read, &disk_write);
            // Convertir bytes a KB si es necesario
	    disk_read /= 1024;
            disk_write /= 1024;
            filp_close(io_file, NULL);
	    } else {
		    disk_read = 0;
		    disk_write = 0;
	    }

	    // Calcular el tiempo totla del contenedor
	    unsigned long total_time = task->start_time;

	    // Obtener el comando
	    cmdline = get_process_cmdline(task);

            seq_printf(m, "	{");
            seq_printf(m, "    	 \"PID\": %d,\n", task->pid);
            seq_printf(m, "    	 \"Name\": \"%s\",\n", task->comm);
	    seq_printf(m, "      \"Cmdline\": \"%s\",\n", cmdline ? cmdline : "N/A");
            seq_printf(m, "    	 \"MemoryUsage\": %lu,\n", process_mem_usage);
            seq_printf(m, "    	 \"CPUUsage\": %lu.%02lu,\n", cpu_usage / 100, cpu_usage % 100);
            seq_printf(m, "    	 \"DiskRead\": %lu,\n", disk_read);
            seq_printf(m, "    	 \"DiskWrite\": %lu,\n", disk_write);
            seq_printf(m, "      \"Time\": %lu\n", total_time);
            seq_printf(m, "  	}");

	    // Libera espacio
            if (cmdline){
	    	kfree(cmdline);
	    }


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
static const struct proc_ops sysinfo_ops = {
    .proc_open = sysinfo_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
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
