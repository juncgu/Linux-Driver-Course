/*
 ============================================================================
 Name        : char_device_driver.c
 Version     : 0.0
 ============================================================================
 */

/*
 * A simple character device driver
 */

#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */
#include <linux/moduleparam.h>
#include <linux/device.h>

#define NEWDEV_NAME "mycdrv"

#define ramdisk_size (size_t) (16 * PAGE_SIZE)
static char DEV_NAME[10];
static dev_t dev_num;
static dev_t dnum;
static int major, minor;
static struct class *cl;
struct Assignment3_mycdrv *cdrv_dev;

//NUM_DEVICES defaults to 3 unless specified during insmod
static int NUM_DEVICES = 3;
module_param(NUM_DEVICES, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

struct Assignment3_mycdrv {
	struct list_head list;
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
};

LIST_HEAD(devLinkList); //initializing linked list head

static int mycdrv_open(struct inode *inode, struct file *file) {
	struct Assignment3_mycdrv *cdrv = container_of(inode->i_cdev,
			struct Assignment3_mycdrv, dev);
    if(down_interruptible(&cdrv->sem) != 0) {
    	printk(KERN_ALERT "mycdrv%d: could not lock device during open",
    			cdrv->devNo);
    	return -1;
    }
    printk(KERN_INFO "mycdrv%d: Successfully Opened Device", cdrv->devNo);
    return 0;
}

static int mycdrv_release(struct inode *inode, struct file *file) {
	struct Assignment3_mycdrv *cdrv = container_of(inode->i_cdev,
			struct Assignment3_mycdrv, dev);
	up(&cdrv->sem);
	pr_info(" CLOSING device: mycdrv%d\n\n", cdrv->devNo);
	return 0;
}

static ssize_t
mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos) {
	int nbytes;
	struct Assignment3_mycdrv *cdrv = container_of(file->f_dentry->d_inode->i_cdev,
			struct Assignment3_mycdrv, dev);
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_to_user(buf, cdrv->ramdisk + *ppos, lbuf);
	*ppos += nbytes;
	pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static ssize_t
mycdrv_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos) {
	int nbytes;
	struct Assignment3_mycdrv *cdrv = container_of(file->f_dentry->d_inode->i_cdev,
			struct Assignment3_mycdrv, dev);
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_from_user(cdrv->ramdisk + *ppos, buf, lbuf);
	*ppos += nbytes;
	pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static const struct file_operations mycdrv_fops = {
	.owner = THIS_MODULE,
	.read = mycdrv_read,
	.write = mycdrv_write,
	.open = mycdrv_open,
	.release = mycdrv_release,
};


/* Initialization method for char driver*/
static int __init my_init(void) {
	unsigned int i = 0;
	unsigned int first = 0;

	int ret;
	ret = alloc_chrdev_region(&dnum, first, NUM_DEVICES, DEV_NAME);
    if (ret<0) {
    	printk(KERN_ALERT "mycdrv: failed to allocate major number");
    	return ret;
    }
    major = MAJOR(dnum);
    minor = MINOR(dnum);
    //creating class for device creation
	cl = class_create(THIS_MODULE, "mycdrv");

	/* Set up individual devices*/
	for (; i < NUM_DEVICES ; i++) {
		cdrv_dev = kmalloc(sizeof(struct Assignment3_mycdrv), GFP_KERNEL);
		dev_num = MKDEV(major, minor + i);
		sprintf(DEV_NAME, "%s%d", NEWDEV_NAME, i);
        printk(KERN_INFO "Device mycdrv%d allocated device num : %d %d", i,
        		major, minor + i);
		cdrv_dev->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
		cdev_init(&cdrv_dev->dev, &mycdrv_fops);
		ret = cdev_add(&cdrv_dev->dev, dev_num, 1);
        if(ret<0) {
        	printk(KERN_ALERT "mycdrv%d: failed to add cdev to kernel", i);
        	return ret;
        }
        cdrv_dev->devNo = i;
        sema_init(&cdrv_dev->sem, 1);
        INIT_LIST_HEAD(&cdrv_dev->list);
        list_add(&cdrv_dev->list, &devLinkList);
        pr_info("\n%s : Successfully registered character device %s\n", DEV_NAME, DEV_NAME);
		device_create(cl, NULL, dev_num, NULL, DEV_NAME);
        pr_info("\n%s : Successfully created character device %s\n", DEV_NAME, DEV_NAME);
	}
	return 0;
}

/* Exit method for char driver*/
static void __exit my_exit(void) {
	struct list_head *pos = NULL, *pos_next = NULL;
	struct Assignment3_mycdrv *temp;
	list_for_each_safe(pos, pos_next, &devLinkList) {
		temp = list_entry(pos, struct Assignment3_mycdrv, list);
		list_del(pos);
		dev_num = temp->dev.dev;
		cdev_del(&(temp->dev));
		pr_info("\nmycdrv%d unregistered\n", MINOR(dev_num));
		kfree(temp->ramdisk);
		device_destroy(cl, dev_num);
		kfree(temp);
	}
	unregister_chrdev_region(dnum, NUM_DEVICES);
	class_destroy(cl);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("TESTMODULE");
MODULE_LICENSE("GPL v2");
