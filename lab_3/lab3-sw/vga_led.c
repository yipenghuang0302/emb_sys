/*
 * Device driver for the VGA LED Emulator
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * References:
 * http://www.linuxforu.com/tag/linux-device-drivers/
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#include "vga_led.h"
#include "socal/hps.h"
#include "hps_0.h"

/* Memory region visible through the lightweight HPS-FPGA bridge */
#define HW_REGS_BASE ALT_LWFPGASLVS_OFST
#define HW_REGS_SIZE 0x200000
#define HW_REGS_MASK (HW_REGS_SIZE - 1)

#define WRITE_BYTE(addr, val)  ( *(volatile unsigned char *)(addr) = (val))

static void __iomem *registers; /* Base of mapped memory */
static dev_t firstdev;
static struct class *cl;
static struct cdev c_dev;

static void *led_registers; /* Start of registers for LEDs */

/* Current segment state, since the hardware registers can't be read */
static unsigned char digits[VGA_LED_DIGITS];

/* Low-level write routine: digit assumed to be in range; remember the state */
static void write_digit(int digit, unsigned char segments) {
  WRITE_BYTE(led_registers + digit, segments);
  digits[digit] = segments;
}

static int my_open(struct inode *i, struct file *f) {
  return 0;
}

static int my_close(struct inode *i, struct file *f) {
  return 0;
}

/* Handle ioctls(): write to the display registers or read our state */
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
  vga_led_arg_t vla;

  switch (cmd) {
  case VGA_LED_WRITE_DIGIT:
    if (copy_from_user(&vla, (vga_led_arg_t *) arg, sizeof(vga_led_arg_t)))
      return -EACCES;
    if (vla.digit > 8) return -EINVAL;
    write_digit(vla.digit, vla.segments);
    break;
    
  case VGA_LED_READ_DIGIT:
    if (copy_from_user(&vla, (vga_led_arg_t *) arg, sizeof(vga_led_arg_t)))
      return -EACCES;
    if (vla.digit > 8) return -EINVAL;
    vla.segments = digits[vla.digit];
    if (copy_to_user((vga_led_arg_t *) arg, &vla, sizeof(vga_led_arg_t)))
      return -EACCES;
    break;

  default:
    return -EINVAL;
  }

  return 0;
}

static struct file_operations my_fops = {
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .unlocked_ioctl = my_ioctl
};

/* Initialize the driver: map the hardware registers, register the
 * device and our operations, and display a welcome message */
static int __init vga_led_init(void) {
  int i;
  static unsigned char welcome_message[VGA_LED_DIGITS] = {
    0x3E, 0x7D, 0x77, 0x08, 0x38, 0x79, 0x5E, 0x00};

  printk(KERN_INFO "vga_led: init\n");

  if ( (registers = ioremap(HW_REGS_BASE, HW_REGS_SIZE)) == NULL ) {
    printk(KERN_ERR "vga_led: Mapping hardware registers failed\n");
    return -1;
  }

  led_registers = registers +
    ((unsigned long) VGA_LED_0_BASE & (unsigned long) HW_REGS_MASK);

  if (alloc_chrdev_region(&firstdev, 0, 1, "vgaled") < 0) goto unmap;
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL) goto unregister;
  if (device_create(cl, NULL, firstdev, NULL, "vgaled") == NULL) goto del_class;
  cdev_init(&c_dev, &my_fops);
  if (cdev_add(&c_dev, firstdev, 1) == -1) goto del_device;

  /* Display a welcome message */
  for (i = 0 ; i < VGA_LED_DIGITS ; i++) write_digit(i, welcome_message[i]);
 
  return 0;

  /* Clean up if something went wrong */
 unmap:      iounmap(registers);
 del_device: device_destroy(cl, firstdev);
 del_class:  class_destroy(cl);
 unregister: unregister_chrdev_region(firstdev, 1);
  return -1;
}

/* Disable the driver; undo the effects of the initialization routine */
static void __exit vga_led_exit(void) {
  printk(KERN_INFO "vga_led: exit\n");

  cdev_del(&c_dev);
  device_destroy(cl, firstdev);
  class_destroy(cl);
  unregister_chrdev_region(firstdev, 1);
  iounmap(registers);
}

module_init(vga_led_init);
module_exit(vga_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen A. Edwards, Columbia University");
MODULE_DESCRIPTION("VGA 7-segment LED Emulator");
