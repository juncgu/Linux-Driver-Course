/* **************** LDD:2.0 s_04/sample_driver.c **************** */
/*
 * The code herein is: Copyright Jerry Cooperstein, 2012
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://www.coopj.com
 *     email:  coop@coopj.com
 *
 * The primary maintainer for this code is Jerry Cooperstein
 * The CONTRIBUTORS file (distributed with this
 * file) lists those known to have contributed to the source.
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/* 
Sample Character Driver 
@*/

#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */
#include <linux/device.h>	
#include <linux/semaphore.h>

#define MYDEV_NAME "mycdrv"

#define ramdisk_size (size_t) (16*PAGE_SIZE)

static int my_major = 0, my_minor = 0;
static int my_nr_devs = 3;
module_param(my_nr_devs, int,0);

struct asp_mycdrv {
	struct list_head list;
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
};
struct asp_mycdrv my_linked_list;
struct list_head *pos, *q;
struct asp_mycdrv *my_node;

static struct class *my_class;

static int mycdrv_open(struct inode *inode, struct file *file)
{
	struct asp_mycdrv *dev;
	dev = container_of(inode->i_cdev, struct asp_mycdrv, dev);
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	file->private_data = dev->ramdisk;
	pr_info(" OPENING device: %s%d:\n\n", MYDEV_NAME,dev->devNo);
	return 0;
}

static int mycdrv_release(struct inode *inode, struct file *file)
{
	struct asp_mycdrv *dev;
	dev = container_of(inode->i_cdev, struct asp_mycdrv, dev);
	up(&dev->sem);
	pr_info(" CLOSING device: %s%d:\n\n", MYDEV_NAME,dev->devNo);
	return 0;
}

static ssize_t
mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
	char *ramdisk = file->private_data;
	int nbytes;
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_to_user(buf, ramdisk + *ppos, lbuf);
	*ppos += nbytes;
	pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static ssize_t
mycdrv_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos)
{
	char *ramdisk = file->private_data;
	int nbytes;
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_from_user(ramdisk + *ppos, buf, lbuf);
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


static void my_setup_cdev(struct asp_mycdrv *dev, int index)
{
	int err, devno = MKDEV(my_major, my_minor + index);
    
	cdev_init(&dev->dev, &mycdrv_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &mycdrv_fops;
	device_create(my_class,NULL,devno,NULL,"%s%d",MYDEV_NAME,index);
	err = cdev_add (&dev->dev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding %s%d", err, MYDEV_NAME,index);
}


static void __exit my_exit(void)
{

	dev_t devno;

	list_for_each_safe(pos, q, &my_linked_list.list) {
		my_node = list_entry(pos, struct asp_mycdrv, list);
		devno = MKDEV(my_major, my_minor+my_node->devNo);
		device_destroy(my_class, devno);
		cdev_del(&(my_node->dev));
		list_del(pos);
		kfree(my_node->ramdisk);
		kfree(my_node);
	}
	
	class_destroy(my_class);

	/* cleanup_module is never called if registering failed */
	devno = MKDEV(my_major, my_minor);
	unregister_chrdev_region(devno, my_nr_devs);
	pr_info("\ndevice unregistered\n");
}

static int __init my_init(void)
{

	int result,i;
	dev_t dev = 0;
	INIT_LIST_HEAD(&my_linked_list.list);
	
 	if(my_major) {
		dev = MKDEV(my_major, my_minor);
		result = register_chrdev_region(dev,my_nr_devs,MYDEV_NAME);
	} else{
		result = alloc_chrdev_region(&dev, my_minor, my_nr_devs,MYDEV_NAME);
		my_major = MAJOR(dev);
	}
	if(result<0) {
		printk(KERN_WARNING "my:can't get major %d\n", my_major);
		return result;
	}

	my_class = class_create(THIS_MODULE,"my_class");
	for (i = 0; i < my_nr_devs; i++) {
		my_node = kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
		sema_init(&my_node->sem,1);
		my_node->devNo = i;
		my_node->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
		my_setup_cdev(my_node, i);
		INIT_LIST_HEAD(&my_node->list);
		list_add_tail(&(my_node->list),&(my_linked_list.list));
	}
	pr_info("\nSucceeded in registering character device %s\n", MYDEV_NAME);
	return 0;
}


module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Jerry Cooperstein");
MODULE_LICENSE("GPL v2");
