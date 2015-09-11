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


#define MYDEV_NAME "mycdrv"
#define ramdisk_size (size_t) (16 * PAGE_SIZE)

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

#define MODE1 1 // define mode here. There can be 2 modes
#define MODE2 2

static int Major;

struct e2_dev {
	struct cdev cdev;
	char *ramdisk;
	struct semaphore sem1, sem2;
	int count1, count2;
	int mode;
  wait_queue_head_t queue1, queue2;
};

struct e2_dev *devc;

	int e2_open(struct inode *inode, struct file *filp)
	{
		struct e2_dev *dev;
		dev = container_of(inode->i_cdev, struct e2_dev, cdev);
		filp->private_data = dev;
		down_interruptible(&dev->sem1);
		if (dev->mode == MODE1) {
			dev->count1++;
			up(&dev->sem1);
			down_interruptible(&dev->sem2);
			return 0;
		}
		else if (dev->mode == MODE2) {
			dev->count2++;
		}
		up(&dev->sem1);
		return 0;
	}

	int e2_release(struct inode *inode, struct file *filp)
	{
		struct e2_dev *dev;
		dev = container_of(inode->i_cdev, struct e2_dev, cdev);
		down_interruptible(&dev->sem1);
		if (dev->mode == MODE1) {
			dev->count1--;
			if (dev->count1 == 1)
				wake_up_interruptible(&(dev->queue1));
			up(&dev->sem2);
		}
		else if (dev->mode == MODE2) {
			dev->count2--;
			if (dev->count2 == 1)
				wake_up_interruptible(&(dev->queue2));
		}
		up(&dev->sem1);
		return 0;
	}



static ssize_t e2_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret;
	struct e2_dev *dev;
	dev = filp->private_data;
	down_interruptible(&dev->sem1);
	if (dev->mode == MODE1) {
		up(&dev->sem1);
		ret = 0; // just initializing ret. not required
	// read
	}
	else {
	// read
		ret = 0; // just initializing ret. not required
		up(&dev->sem1);
	}
	return ret;
}


static ssize_t e2_write (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret;
	struct e2_dev *dev;
	dev = filp->private_data;
	down_interruptible(&dev->sem1);
	if (dev->mode == MODE1) {
		up(&dev->sem1);
		ret = 0; // just initializing ret. not required
	// write
	}
	else {
	// write
			ret = 0; // just initializing ret. not required
		up(&dev->sem1);
	}
	return ret;
}

int e2_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	struct e2_dev *dev;
	dev = container_of(inode->i_cdev, struct e2_dev, cdev);
	
	if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC) {
		pr_info("Invalid magic number\n");
		return -ENOTTY;
	}
	if ( (_IOC_NR(cmd) != 1) | (_IOC_NR(cmd) != 2) ) {
		pr_info("Invalid cmd\n");
		return -ENOTTY;
	}
	
	switch(cmd) {
		case E2_IOCMODE2:
				down_interruptible(&(dev->sem1));
				if (dev->mode == MODE2) {
					up(&dev->sem1);
					break;
				}
				if (dev->count1 > 1) {
					while (dev->count1 > 1) {
						up(&dev->sem1);
						wait_event_interruptible(dev->queue1, (dev->count1 == 1));
						down_interruptible(&dev->sem1);
					}
				}
				dev->mode = MODE2;
				up(&dev->sem2);
				up(&dev->sem1);
				break;
				
		case E2_IOCMODE1:
				down_interruptible(&dev->sem1);
				if (dev->mode == MODE1) {
				up(&dev->sem1);
				break;
				}
				if (dev->count2 > 0) {
				while (dev->count2 > 0) {
				up(&dev->sem1);
				wait_event_interruptible(dev->queue2, (dev->count2 == 0));
				down_interruptible(&dev->sem1);
				}
				}
				dev->mode = MODE1;
				down(&dev->sem2);
				up(&dev->sem1);
				break;
				
				default :
					pr_info("Unrecognized ioctl command\n");
					return -1;
					break;	
	}
	return 0;
}



static const struct file_operations fops = { 
  .owner = THIS_MODULE,
	.read = e2_read,
	.write = e2_write,
	.open = e2_open,
	.release = e2_release,
	.unlocked_ioctl = e2_ioctl
};

static int __init my_init (void) {
   int ret = 0;
   dev_t dev_no, dev1;
	struct e2_dev *dev;
	dev = devc;
	 ret = alloc_chrdev_region(&dev_no, 0, 1,MYDEV_NAME);
	 if (ret<0) {
    	printk(KERN_ALERT "mycdrv: failed to allocate major number");
    	return ret;
    }
   Major = MAJOR(dev_no);
   dev = kmalloc(sizeof(struct e2_dev), GFP_KERNEL);
   dev1 = MKDEV(Major,0);
	 dev->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
   cdev_init(&(dev->cdev), &fops); 
   printk(KERN_INFO "init: starting driver\n");
   
   /*
For referrence   
  struct e2_dev {
	struct cdev cdev;
	char *ramdisk;
	struct semaphore sem1, sem2;
	int count1, count2;
	int mode;
  wait_queue_head_t queue1, queue2;
};   
   */
   dev->count1 = 0; 
   dev->count2 = 0;
   dev->mode = 1;
   sema_init(&dev->sem1, 1);
   sema_init(&dev->sem2, 1);
   ret = cdev_add(&dev->cdev, dev1, 1);
   if(ret < 0 ) {
      printk(KERN_INFO "Unable to allocate cdev");
      return ret;
   }
   return 0;
}

static void __exit my_exit(void) {
		struct e2_dev *dev;
		dev = devc;
   printk(KERN_INFO "cleanup: unloading driver\n");
	 cdev_del(&(dev->cdev));
	 kfree(dev->ramdisk);
	 kfree(dev);
   unregister_chrdev_region(Major, 1);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Assignment5");
MODULE_LICENSE("GPL v2");
