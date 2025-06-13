#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define DEVICE_NAME "cdd_TP5"
#define CLASS_NAME  "cdd_TP5_class"
// A partir del version de kernel 6.2, los pines GPIO ya no son tan directos como antes para direccionarse:
#define GPIO24 536   // PIN 18 -> echo 0 | sudo tee /dev/cdd_TP5
#define GPIO25 537   // PIN 22 -> echo 1 | sudo tee /dev/cdd_TP5
#define SAMPLE_RATE 20000 // 20kHz
#define ACQ_TIME_SEC 1
#define N_SAMPLES (SAMPLE_RATE * ACQ_TIME_SEC)

static int major;
static struct class*  mi_cdd_class  = NULL;
static struct device* mi_cdd_device = NULL;

static int selected_signal = 0; // 0 o 1
static int *sample_buffer = NULL;
static int acq_ready = 0;
static DEFINE_MUTEX(signal_mutex);
static struct gpio_desc *desc24 = NULL;
static struct gpio_desc *desc25 = NULL;

static int acquire_signal(int gpio)
{
    size_t i;
    ktime_t start, target;
    s64 interval_ns = 1000000000LL / SAMPLE_RATE; // nanosegundos entre muestras

    start = ktime_get();
    for (i = 0; i < N_SAMPLES; i++) {
        int value;
        if (gpio == GPIO24 && desc24)
            value = gpiod_get_value(desc24);
        else if (gpio == GPIO25 && desc25)
            value = gpiod_get_value(desc25);
        else
            value = 0;
        sample_buffer[i] = value;
        if ((i%90 == 0) && (i<1000) ) // Solo los primeros algunos para no saturar el log
            printk(KERN_INFO "sample[%zu]=%d\n", i, sample_buffer[i]);
        // Espera activa para mantener la tasa de muestreo
        target = ktime_add_ns(start, interval_ns * (i + 1));
        while (ktime_compare(ktime_get(), target) < 0)
            cpu_relax();
    }
    return 0;
}

static ssize_t mi_cdd_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    ssize_t ret = 0;
    size_t i, len = 0;
    char *kbuf;

    mutex_lock(&signal_mutex);
    if (!acq_ready) {
        mutex_unlock(&signal_mutex);
        return 0; // No hay datos listos
    }
    kbuf = kmalloc(N_SAMPLES, GFP_KERNEL);
    if (!kbuf) {
        mutex_unlock(&signal_mutex);
        return -ENOMEM;
    }
    for (i = 0; i < N_SAMPLES; i++)
        kbuf[i] = sample_buffer[i] ? '1' : '0';
    len = N_SAMPLES;
    if (*ppos >= len) {
        kfree(kbuf);
        mutex_unlock(&signal_mutex);
        return 0;
    }
    if (count > len - *ppos)
        count = len - *ppos;
    if (copy_to_user(buf, kbuf + *ppos, count)) {
        kfree(kbuf);
        mutex_unlock(&signal_mutex);
        return -EFAULT;
    }
    *ppos += count;
    ret = count;
    kfree(kbuf);
    mutex_unlock(&signal_mutex);
    return ret;
}

static ssize_t mi_cdd_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char kbuf[4];
    int gpio;
    if (count > 3) return -EINVAL;
    if (copy_from_user(kbuf, buf, count)) return -EFAULT;
    kbuf[count] = '\0';
    mutex_lock(&signal_mutex);
    printk(KERN_INFO "mi_cdd_write: recibido %s\n", kbuf);
    if (kbuf[0] == '0') {
        selected_signal = 0;
        printk(KERN_INFO "mi_cdd_write: seleccionada señal 0 -> GPIO24\n");
    } else if (kbuf[0] == '1') {
        selected_signal = 1;
        printk(KERN_INFO "mi_cdd_write: seleccionada señal 1 -> GPIO25 \n");
    } else {
        printk(KERN_INFO "mi_cdd_write: valor inválido\n");
        mutex_unlock(&signal_mutex);
        return -EINVAL;
    }
    acq_ready = 0;
    printk(KERN_INFO "mi_cdd_write: iniciando adquisición\n");
    gpio = (selected_signal == 0) ? GPIO24 : GPIO25;
    acquire_signal(gpio);
    printk(KERN_INFO "mi_cdd_write: adquisición terminada\n");
    acq_ready = 1;
    mutex_unlock(&signal_mutex);
    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read  = mi_cdd_read,
    .write = mi_cdd_write,
};

static int __init mi_cdd_init(void) {
    if (!gpio_is_valid(GPIO24)) {
        printk(KERN_ERR "GPIO %d no es válido\n", GPIO24);
        return -EINVAL;
    }
    if (!gpio_is_valid(GPIO25)) {
        printk(KERN_ERR "GPIO %d no es válido\n", GPIO25);
        return -EINVAL;
    }
    gpio_direction_input(GPIO24);
    gpio_direction_input(GPIO25);
    printk(KERN_INFO "Configurados GPIO %d y %d como entradas\n", GPIO24, GPIO25);

    desc24 = gpio_to_desc(GPIO24);
    desc25 = gpio_to_desc(GPIO25);
    if (!desc24 || !desc25) {
        printk(KERN_ERR "No se pudo obtener descriptor GPIO\n");
        return -EINVAL;
    }

    sample_buffer = kmalloc_array(N_SAMPLES, sizeof(int), GFP_KERNEL);
    if (!sample_buffer) {
        return -ENOMEM;
    }

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        kfree(sample_buffer);
        printk(KERN_ERR "No se pudo registrar el device\n");
        return major;
    }
    mi_cdd_class = class_create(CLASS_NAME);
    if (IS_ERR(mi_cdd_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        kfree(sample_buffer);
        return PTR_ERR(mi_cdd_class);
    }
    mi_cdd_device = device_create(mi_cdd_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mi_cdd_device)) {
        class_destroy(mi_cdd_class);
        unregister_chrdev(major, DEVICE_NAME);
        kfree(sample_buffer);
        return PTR_ERR(mi_cdd_device);
    }
    mutex_init(&signal_mutex);
    printk(KERN_INFO "mi_cdd: driver cargado\n");
    return 0;
}

static void __exit mi_cdd_exit(void) {
    device_destroy(mi_cdd_class, MKDEV(major, 0));
    class_destroy(mi_cdd_class);
    unregister_chrdev(major, DEVICE_NAME);
    kfree(sample_buffer);
    printk(KERN_INFO "mi_cdd: driver descargado\n");
}

module_init(mi_cdd_init);
module_exit(mi_cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Franco, Juli y Pato");
MODULE_DESCRIPTION("CDD para sensar una señal seleccionada a alta velocidad durante 1 segundo");