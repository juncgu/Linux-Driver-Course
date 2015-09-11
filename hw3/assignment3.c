/************************
 *EEL5934  ADV SYSTEM PROG
 *Assignment3
 *Juncheng Gu 5191-0572
 *Feb23, 2015
 ************************/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#define MYDEV_NAME "mycdrv"
#define DEV_NUM 3

/*Juncheng Gu
static char *ramdisk;
static size_t ramdisk_size = (16 * PAGE_SIZE);
static dev_t first;
static unsigned int count = 1;
static struct cdev *my_cdev;
*/

struct asp_mycdrv{
    struct list_head list;
    struct cdev dev;
    char *ramdisk;
    struct semaphore sem;
    int devNo;
};
static dev_t first;
static struct class *foo_class;
static size_t ramdisk_size = (16 * PAGE_SIZE);
static int mycdrv_major;
static int mycdrv_minor;
static struct asp_mycdrv *asp;

static int mycdrv_open(struct inode *inode, struct file *file)
{
	static int counter = 0;
        struct asp_mycdrv *tmp_dev = NULL;
        tmp_dev = container_of(inode->i_cdev, struct asp_mycdrv, dev);
        if (down_interruptible(&(tmp_dev->sem))){
            return -ERESTARTSYS; 
        }
        file->private_data = tmp_dev;
	pr_info(" attempting to open device: %s%d:\n", MYDEV_NAME, tmp_dev->devNo);
	pr_info(" MAJOR number = %d, MINOR number = %d\n",imajor(inode), iminor(inode));
	counter++;

	pr_info(" successfully open  device: %s%d:\n\n", MYDEV_NAME, tmp_dev->devNo);
	pr_info("I have been opened  %d times since being loaded\n", counter);
	pr_info("ref=%d\n", (int)module_refcount(THIS_MODULE));

	return 0;
}

static int mycdrv_release(struct inode *inode, struct file *file)
{
        struct asp_mycdrv *tmp_dev = NULL;
        tmp_dev = container_of(inode->i_cdev, struct asp_mycdrv, dev);
	pr_info(" CLOSING device: %s%d:\n\n", MYDEV_NAME, tmp_dev->devNo);
        up(&(tmp_dev->sem));
	return 0;
}

static ssize_t
mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
	int nbytes, maxbytes, bytes_to_do;
        struct asp_mycdrv *tmp_dev;
        tmp_dev = file->private_data;
	maxbytes = ramdisk_size - *ppos;
	bytes_to_do = maxbytes > lbuf ? lbuf : maxbytes;
	if (bytes_to_do == 0)
		pr_info("Reached end of the device on a read");
	nbytes = bytes_to_do - copy_to_user(buf, tmp_dev->ramdisk + *ppos, bytes_to_do);
	*ppos += nbytes;
	pr_info("\n Leaving the  READ function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static ssize_t
mycdrv_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos)
{
	int nbytes, maxbytes, bytes_to_do;
        struct asp_mycdrv *tmp_dev;
        tmp_dev = file->private_data;
	maxbytes = ramdisk_size - *ppos;
	bytes_to_do = maxbytes > lbuf ? lbuf : maxbytes;
	if (bytes_to_do == 0)
		pr_info("Reached end of the device on a write");
	nbytes =
	    bytes_to_do - copy_from_user(tmp_dev->ramdisk + *ppos, buf, bytes_to_do);
	*ppos += nbytes;
	pr_info("\n Leaving the   WRITE function, nbytes=%d, pos=%d\n",
		nbytes, (int)*ppos);
	return nbytes;
}

static loff_t mycdrv_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t testpos;
	switch (orig) {
	case SEEK_SET:
		testpos = offset;
		break;
	case SEEK_CUR:
		testpos = file->f_pos + offset;
		break;
	case SEEK_END:
		testpos = ramdisk_size + offset;
		break;
	default:
		return -EINVAL;
	}
	testpos = testpos < ramdisk_size ? testpos : ramdisk_size;
	testpos = testpos >= 0 ? testpos : 0;
	file->f_pos = testpos;
	pr_info("Seeking to pos=%ld\n", (long)testpos);
	return testpos;
}

static const struct file_operations mycdrv_fops = {
	.owner = THIS_MODULE,
	.read = mycdrv_read,
	.write = mycdrv_write,
	.open = mycdrv_open,
	.release = mycdrv_release,
	.llseek = mycdrv_lseek
};

//Juncheng Gu
static int asp_setup_cdev(struct asp_mycdrv *dev, int major, int minor, int index){
        int devno = MKDEV(major, minor);
        int err;
	cdev_init(&(dev->dev), &mycdrv_fops);
        dev->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
        dev->dev.owner = THIS_MODULE; 
        dev->dev.ops = &mycdrv_fops;
        dev->devNo = index;
        err=cdev_add(&(dev->dev), devno, 1);
	if (err < 0) {
                printk(KERN_NOTICE "Error %d adding asp_mycdrv%d", err, index);
		cdev_del(&(dev->dev));
                //first not sure where to go.
		unregister_chrdev_region(first, DEV_NUM);
		kfree(dev->ramdisk);
		return -1;
	}
	device_create(foo_class, NULL, devno, NULL, "mycdrv%d", index);
	pr_info("\nSucceeded in registering character device %s%d\n", MYDEV_NAME, index);
	pr_info("Major number = %d, Minor number = %d\n", major, minor);
        return 1;
}
static int __init my_init(void)
{
        int i = 0;
        struct asp_mycdrv *tmp_asp = NULL;
	if (alloc_chrdev_region(&first, 0, DEV_NUM, MYDEV_NAME) < 0) {
		pr_err("failed to allocate character device region\n");
		return -1;
	}
        mycdrv_major = MAJOR(first);
        mycdrv_minor = MINOR(first);
	foo_class = class_create(THIS_MODULE, "my_class");
        asp = kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
        INIT_LIST_HEAD(&(asp->list));
        sema_init(&(asp->sem), 1);
        if (asp_setup_cdev(asp, mycdrv_major, mycdrv_minor, i) < 0){
            return -1;
        }
        for (i=1;i<DEV_NUM;i++){
           tmp_asp = kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
           INIT_LIST_HEAD(&(tmp_asp->list));
           //init_MUTEX(&(tmp_asp->sem));
           sema_init(&(tmp_asp->sem), 1);
           if (asp_setup_cdev(tmp_asp, mycdrv_major, mycdrv_minor+i, i) < 0){
               return -1;
           } 
           list_add(&(tmp_asp->list), &(asp->list));
        }
	return 0;
}

static void __exit my_exit(void)
{
        int i = 0;
        int devno = 0;
        struct asp_mycdrv *tmp = NULL;
        struct list_head* pos = NULL;
        struct list_head* p = NULL;
        for (i=0; i<DEV_NUM;i++){
            devno = MKDEV(mycdrv_major, mycdrv_minor+i);
            device_destroy(foo_class, devno);
        }
	class_destroy(foo_class);
        
        list_for_each_safe(pos, p, &(asp->list)){
            tmp = list_entry(pos, struct asp_mycdrv, list);
            if (&(tmp->dev)){
                cdev_del(&(tmp->dev));
            }
            kfree(tmp->ramdisk); //may move;
            list_del(pos);
        }
	unregister_chrdev_region(first, DEV_NUM);
	pr_info("\ndevice unregistered\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Jerry Cooperstein");
MODULE_DESCRIPTION("LDD:2.0 s_04/lab5_dynamic_udev.c");
MODULE_LICENSE("GPL v2");
