#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/ioctl.h>


#define MYDEV_NAME "a5"
#define ramdisk_size (size_t) (16 * PAGE_SIZE)

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

#define MODE1 1 // define mode here. There can be 2 modes
#define MODE2 2

static int majorNo = 700 , minorNo = 0;

static struct class *cl;
struct e2_dev {
	struct cdev cdev;
	char *ramdisk;
	struct semaphore sem1, sem2;
	int count1, count2;
	int mode;
    wait_queue_head_t queue1, queue2;
};

struct e2_dev *dev;


int e2_open(struct inode *inode, struct file *filp)
{
    struct e2_dev *devc = container_of(inode->i_cdev, struct e2_dev, cdev);
    filp->private_data = devc;
    pr_info("Open: just enter\n");
    down_interruptible(&devc->sem1);
    pr_info("      down sem1\n");
    if (devc->mode == MODE1) {
        pr_info("      in MODE1\n");
        pr_info("      before count1=%d\n", devc->count1);
        devc->count1++;
        pr_info("      after count1=%d\n", devc->count1);
        up(&devc->sem1);
        pr_info("      up sem1\n");
        down_interruptible(&devc->sem2);
        pr_info("      down sem2\n");
        pr_info("Open: just leave\n");
        return 0;
    }
    else if (devc->mode == MODE2) {
        pr_info("      in MODE2\n");
        pr_info("      before count2=%d\n", devc->count2);
        devc->count2++;
        pr_info("      after count2=%d\n", devc->count2);
    }
    up(&devc->sem1);
    pr_info("      up sem1\n");
    pr_info("Open: just leave\n");
    return 0;
}

int e2_release(struct inode *inode, struct file *filp)
{
    struct e2_dev *devc = container_of(inode->i_cdev, struct e2_dev, cdev);
    pr_info("Release: just enter\n");
    down_interruptible(&devc->sem1);
    pr_info("         down sem1\n");
    if (devc->mode == MODE1) {
        pr_info("         in MODE1\n");
        pr_info("         before count1=%d\n", devc->count1);
        devc->count1--;
        pr_info("         after count1=%d\n", devc->count1);
        if (devc->count1 == 1)
            wake_up_interruptible(&(devc->queue1));
			up(&devc->sem2);
    }
    else if (devc->mode == MODE2) {
        pr_info("         in MODE2\n");
        pr_info("         before count2=%d\n", devc->count2);
        devc->count2--;
        pr_info("         after count2=%d\n", devc->count2);
        if (devc->count2 == 1)
            wake_up_interruptible(&(devc->queue2));
    }
    up(&devc->sem1);
    pr_info("         up sem1\n");
    pr_info("Release: just leave\n");
    return 0;
}

static ssize_t e2_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
        struct e2_dev *devc = filp->private_data;
	ssize_t ret = 0;
        pr_info("Read: just enter\n");
	down_interruptible(&devc->sem1);
	if (devc->mode == MODE1) {
	   up(&devc->sem1);
           if (*f_pos + count > ramdisk_size) {
              printk("Trying to read past end of buffer!\n");
              pr_info("Read: just leave\n");
              return ret;
           }
	   ret = count - copy_to_user(buf, devc->ramdisk, count);
	}
	else {
          if (*f_pos + count > ramdisk_size) {
             printk("Trying to read past end of buffer!\n");
             up(&devc->sem1);
             pr_info("Read: just leave\n");
             return ret;
          }
          ret = count - copy_to_user(buf, devc->ramdisk, count);
	  up(&devc->sem1);
	}
        pr_info("Read: just leave\n");
	return ret;
}


static ssize_t e2_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct e2_dev *devc;
	ssize_t ret = 0;
	devc = filp->private_data;
        pr_info("Write: just enter\n");
	down_interruptible(&devc->sem1);
	if (devc->mode == MODE1) {
		up(&devc->sem1);
        if (*f_pos + count > ramdisk_size) {
            printk("Trying to read past end of buffer!\n");
            pr_info("Write: just leave\n");
            return ret;
        }
        ret = count - copy_from_user(devc->ramdisk, buf, count);
	}
	else {
        if (*f_pos + count > ramdisk_size) {
            printk("Trying to read past end of buffer!\n");
            up(&devc->sem1);
            pr_info("Write: just leave\n");
            return ret;
        }
        ret = count - copy_from_user(devc->ramdisk, buf, count);
		up(&devc->sem1);
	}
        pr_info("Write: just leave\n");
	return ret;
}

static long e2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	
       struct e2_dev *devc = filp->private_data;
	
	if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC) {
		pr_info("Invalid magic number\n");
		return -ENOTTY;
	}
	if ( (_IOC_NR(cmd) != 1) && (_IOC_NR(cmd) != 2) ) {
		pr_info("Invalid cmd\n");
		return -ENOTTY;
	}
        pr_info("IOCTL: just enter\n");
	switch(cmd) {
		case E2_IOCMODE2:
                                pr_info("       need switch to MODE2\n");
				down_interruptible(&(devc->sem1));
                                pr_info("       down sem1\n");
				if (devc->mode == MODE2) {
                                        pr_info("        in MODE2, no action\n");
					up(&devc->sem1);
                                        pr_info("        up sem1\n");
					break;
				}
				if (devc->count1 > 1) {
                                        pr_info("        in MODE1\n");
					while (devc->count1 > 1) {
                                            pr_info("        >1, count1=%d\n", devc->count1);
			          	    up(&devc->sem1);
                                            pr_info("        up sem1\n");
                                            pr_info("        put to queue1, wait...\n");
					    wait_event_interruptible(devc->queue1, (devc->count1 == 1));
                                            pr_info("        wake up from queue1\n");
		                            down_interruptible(&devc->sem1);
                                            pr_info("        down sem1\n");
					}
				}
                                pr_info("        before MODE1 count1=%d, count2=%d\n", devc->count1, devc->count2);
				devc->mode = MODE2;
                                devc->count1--;
                                devc->count2++;
                                pr_info("        after MODE2 count1=%d, count2=%d\n", devc->count1, devc->count2);
				up(&devc->sem2);
                                pr_info("        up sem2\n");
				up(&devc->sem1);
                                pr_info("        up sem1\n");
				break;
				
		case E2_IOCMODE1:
                                pr_info("       need switch to MODE1\n");
				down_interruptible(&devc->sem1);
                                pr_info("       down sem1\n");
				if (devc->mode == MODE1) {
                                   pr_info("        in MODE1, no action\n");
				   up(&devc->sem1);
                                   pr_info("        up sem1\n");
				   break;
				}
				if (devc->count2 > 1) {
                                   pr_info("        in MODE2\n");
				   while (devc->count2 > 1) {
                                       pr_info("        >1, count2=%d\n", devc->count2);
				       up(&devc->sem1);
                                       pr_info("        up sem1\n");
                                       pr_info("        put to queue2, wait...\n");
				       wait_event_interruptible(devc->queue2, (devc->count2 == 1));
                                       pr_info("        wake up from queue2\n");
				       down_interruptible(&devc->sem1);
                                       pr_info("        down sem1\n");
				   }
				}
                                pr_info("       before MODE2 count1=%d, count2=%d\n", devc->count1, devc->count2);
				devc->mode = MODE1;
                                devc->count2--;
                                devc->count1++;
                                pr_info("       after MODE1 count1=%d, count2=%d\n", devc->count1, devc->count2);
				down(&devc->sem2);
                                pr_info("       down sem2\n");
				up(&devc->sem1);
                                pr_info("       up sem1\n");
				break;
				
				default :
					pr_info("Unrecognized ioctl command\n");
					return -1;
					break;	
	}
        pr_info("IOCTL: just leave\n");
	return 0;
}

static const struct file_operations fops = { 
  .owner = THIS_MODULE,
	.read = e2_read,
	.write = e2_write,
	.open = e2_open,
	.release = e2_release,
	.unlocked_ioctl = e2_ioctl,
};

static int __init my_init (void) {

   int ret = 0;
   dev_t dev_no = MKDEV(majorNo, minorNo);
   ret = register_chrdev_region(dev_no, 1, MYDEV_NAME);
   if (ret<0) {
    	printk(KERN_ALERT "mycdrv: failed to reserve major number");
    	return ret;
   }
   cl = class_create(THIS_MODULE, MYDEV_NAME);
   dev = kmalloc(sizeof(struct e2_dev), GFP_KERNEL);
   dev->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
   memset(dev->ramdisk,0,ramdisk_size);
   cdev_init(&(dev->cdev), &fops); 
   dev->count1 = 0;
   dev->count2 = 0;
   dev->mode = MODE1;
   init_waitqueue_head(&dev->queue1);
   init_waitqueue_head(&dev->queue2);
   sema_init(&dev->sem1, 1);
   sema_init(&dev->sem2, 1);
   ret = cdev_add(&dev->cdev, dev_no, 1);
   if(ret < 0 ) {
      printk(KERN_INFO "Unable to register cdev");
      return ret;
   }
   device_create(cl, NULL, dev_no, NULL, MYDEV_NAME);
   return 0;
}

static void __exit my_exit(void) {

   dev_t devNo = MKDEV(majorNo, minorNo);  
   printk(KERN_INFO "cleanup: unloading driver\n");
   cdev_del(&(dev->cdev));
   kfree(dev->ramdisk);
   device_destroy(cl, devNo);
   kfree(dev);
   unregister_chrdev_region(devNo, 1);
   class_destroy(cl);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Assignment5");
MODULE_LICENSE("GPL v2");
