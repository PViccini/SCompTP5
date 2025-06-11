#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
// #include <linux/gpio/consumer.h> // Para GPIOs en dispositivos modernos


#define DEVICE_NAME "cdd_TP5"
#define CLASS_NAME  "cdd_TP5_class"
#define GPIO22 17   // quedo asi porque bueno pasason cosas
#define GPIO23 18
#define SAMPLE_RATE 20000 // 20kHz
#define ACQ_TIME_SEC 1
#define N_SAMPLES (SAMPLE_RATE * ACQ_TIME_SEC)

static int major;
static struct class*  mi_cdd_class  = NULL;
static struct device* mi_cdd_device = NULL;

static int selected_signal = 0; // 0 o 1
static int *sample_buffer = NULL;
// static size_t sample_count = 0;
static int acq_ready = 0;
static DEFINE_MUTEX(signal_mutex);

static int acquire_signal(int gpio)
{
    size_t i;
    ktime_t start, interval;
    s64 elapsed;
    interval = ktime_set(0, 1000000000 / SAMPLE_RATE); // nanosegundos entre muestras
    start = ktime_get();
    for (i = 0; i < N_SAMPLES; i++) {
        sample_buffer[i] = gpio_get_value(gpio);
        // Espera activa para mantener la tasa de muestreo
        while ((elapsed = ktime_to_ns(ktime_sub(ktime_get(), start))) < (i+1) * (1000000000 / SAMPLE_RATE)) cpu_relax();
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
    // Calcula el tamaño necesario para el buffer de salida
    kbuf = kmalloc(N_SAMPLES * 2, GFP_KERNEL); // "0\n" o "1\n" por muestra
    if (!kbuf) {
        mutex_unlock(&signal_mutex);
        return -ENOMEM;
    }
    for (i = 0; i < N_SAMPLES; i++)
        len += snprintf(kbuf + len, 2, "%d", sample_buffer[i]);
    // Devuelve los datos al usuario
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
    if (kbuf[0] == '0') {
        selected_signal = 0;
        printk(KERN_INFO "Configured Signal 0: PIN %d -> Sampled: 1 second\n", GPIO22);
    } else if (kbuf[0] == '1') {
        selected_signal = 1;
        printk(KERN_INFO "Configured Signal 1: PIN %d -> Sampled: 1 second\n", GPIO23);
    } else {
        mutex_unlock(&signal_mutex);
        return -EINVAL;
    }
    // Dispara adquisición
    acq_ready = 0;
    gpio = (selected_signal == 0) ? GPIO22 : GPIO23;
    acquire_signal(gpio);
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
    
    gpio_direction_input(GPIO22);
    gpio_direction_input(GPIO23);

    sample_buffer = kmalloc_array(N_SAMPLES, sizeof(int), GFP_KERNEL);
    if (!sample_buffer) {
        gpio_free(GPIO22);
        gpio_free(GPIO23);
        return -ENOMEM;
    }

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        kfree(sample_buffer);
        gpio_free(GPIO22);
        gpio_free(GPIO23);
        printk(KERN_ERR "No se pudo registrar el device\n");
        return major;
    }
    mi_cdd_class = class_create(CLASS_NAME);
    if (IS_ERR(mi_cdd_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        kfree(sample_buffer);
        gpio_free(GPIO22);
        gpio_free(GPIO23);
        return PTR_ERR(mi_cdd_class);
    }
    mi_cdd_device = device_create(mi_cdd_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mi_cdd_device)) {
        class_destroy(mi_cdd_class);
        unregister_chrdev(major, DEVICE_NAME);
        kfree(sample_buffer);
        gpio_free(GPIO22);
        gpio_free(GPIO23);
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
    gpio_free(GPIO22);
    gpio_free(GPIO23);
    printk(KERN_INFO "mi_cdd: driver descargado\n");
}

module_init(mi_cdd_init);
module_exit(mi_cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tu Nombre");
MODULE_DESCRIPTION("CDD para sensar una señal seleccionada a alta velocidad durante 1 segundo");