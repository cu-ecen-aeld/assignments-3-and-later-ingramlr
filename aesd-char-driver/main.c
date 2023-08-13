/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/string.h>
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Logan Ingram");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");

    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry;
    uint8_t index;
    
    dev = container_of(inode->i_cdev, struct aesd_dev, chardev);
    filp->private_data = dev;

    PDEBUG("Open current buffer content");
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buff, index) {
    	if (entry->buffptr)
    	{
    		PDEBUG("open: entry %s", entry->buffptr);
    	}
    }
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");

    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry;
    uint8_t index;
    dev = container_of(inode->i_cdev, struct aesd_dev, chardev);
  
    if (mutex_is_locked(&(dev->writeLock)))
    {
    	mutex_unlock(&(dev->writeLock));
    }
    
    PDEBUG("release: current buffer content");
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buff, index) {
    	if (entry->buffptr)
    	{
    		PDEBUG("Mutex release entry %s", entry->buffptr);
    	}
    }

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *) filp->private_data;
    struct aesd_buffer_entry *p_entry;
    size_t received_bytes_offset, bytes_to_copy;
    
    ssize_t retval = 0;
    PDEBUG("read %ld bytes with offset %lld",count,*f_pos);
    

    (void) mutex_lock_interruptible(&(dev->writeLock));
    p_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->buff), *f_pos, &received_bytes_offset);
    mutex_unlock(&(dev->writeLock));
    if (NULL == p_entry)
    {	
    	PDEBUG("read: no entry found");
    	return 0;
    }
    PDEBUG("String found %s at offset %ld", &p_entry->buffptr[received_bytes_offset], received_bytes_offset);
    PDEBUG("Full string string %s", p_entry->buffptr);
    bytes_to_copy = ((p_entry->size - received_bytes_offset) > count) ? count : (p_entry->size - received_bytes_offset);
    retval = bytes_to_copy - copy_to_user(buf, &p_entry->buffptr[received_bytes_offset], bytes_to_copy);
    *f_pos += retval;
    
    PDEBUG("Copies %ld bytes to user", retval);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *) filp->private_data;
    struct aesd_buffer_entry *entry = NULL;
    char *new_buf;

    ssize_t retval = -ENOMEM;
    PDEBUG("Write %ld bytes with offset %lld",count,*f_pos);

    new_buf = (char *) kmalloc(count + dev->partial_len, GFP_KERNEL);
    if (NULL != dev->partial_write)
    {	
    	strncpy(new_buf, dev->partial_write, dev->partial_len);
    }
    PDEBUG("write: new_buf before appending input %s", new_buf);
    retval = count - copy_from_user(&new_buf[dev->partial_len], buf, count);
    PDEBUG("write: new_buf after appending input %s", new_buf);
    if ('\n' == new_buf[count + dev->partial_len - 1])
    {
    	PDEBUG("writing %s", new_buf);
    	entry = (struct aesd_buffer_entry *) kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
    	entry->buffptr = new_buf;
    	entry->size = dev->partial_len + count;
    	(void) mutex_lock_interruptible(&(dev->writeLock));
    	aesd_circular_buffer_add_entry(&(dev->buff), entry);
    	mutex_unlock(&(dev->writeLock));
    	kfree(dev->partial_write);
    	dev->partial_write = NULL;
    	dev->partial_len = 0;
    }
    else
    {
    	PDEBUG("partial write of %s", new_buf);
    	kfree(dev->partial_write);	    
	dev->partial_write = new_buf;
	dev->partial_len += retval;
    }

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int error, deviceno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->chardev, &aesd_fops);
    dev->chardev.owner = THIS_MODULE;
    dev->chardev.ops = &aesd_fops;
    error = cdev_add (&dev->chardev, deviceno, 1);
    if (error) {
        printk(KERN_ERR "Error %d adding aesd cdev", error);
    }
    return error;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buff);
    aesd_device.partial_write = NULL;
    aesd_device.partial_len = 0;
    mutex_init(&(aesd_device.writeLock));

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t deviceno = MKDEV(aesd_major, aesd_minor);
    struct aesd_buffer_entry *entry;
    uint8_t index;

    cdev_del(&aesd_device.chardev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buff, index) {
    	if (entry->buffptr)
    	{
    		kfree(entry->buffptr);
    	}
    }

    unregister_chrdev_region(deviceno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
