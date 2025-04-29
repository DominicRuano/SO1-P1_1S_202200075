#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>

#define PROC_NAME "sysinfo_202200075"

static int sysinfo_show(struct seq_file *m, void *v) {
    seq_printf(m, "Modulo de Kernel funcionando correctamente\n");
    return 0;
}

static int sysinfo_open(struct inode *inode, struct file *file) {
    return single_open(file, sysinfo_show, NULL);
}

static const struct proc_ops sysinfo_fops = {
    .proc_open = sysinfo_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init sysinfo_init(void) {
    proc_create(PROC_NAME, 0, NULL, &sysinfo_fops);
    printk(KERN_INFO "Modulo sysinfo_202200075 cargado\n");
    return 0;
}

static void __exit sysinfo_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "Modulo sysinfo_202200075 eliminado\n");
}

module_init(sysinfo_init);
module_exit(sysinfo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dominic Ruano");
MODULE_DESCRIPTION("Modulo de Kernel para obtener informacion del sistema, sopes1 - 202200075");

