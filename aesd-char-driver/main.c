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

    struct aesd_dev *dev = NULL;                                        //Declare the device struct and set it to NULL so its not some random address
    struct aesd_buffer_entry *entry;
    uint8_t index;
    
    dev = container_of(inode->i_cdev, struct aesd_dev, chardev);        //Figure out how long each piece is based on the size of the struct
    filp->private_data = dev;

    PDEBUG("Open current buffer content");
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buff, index){
    	if (entry->buffptr){
            
    		PDEBUG("Opened at: %s", entry->buffptr);
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
  
    if (mutex_is_locked(&(dev->writeLock))){                     //Check if the mutex is already locked and if so, release it

    	mutex_unlock(&(dev->writeLock));
    }
    
    PDEBUG("Release current locked buffer");
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buff, index) {
    	if (entry->buffptr){                                        

    		PDEBUG("Mutex release entry %s", entry->buffptr);
    	}
    }

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *) filp->private_data;
    struct aesd_buffer_entry *circBuf;
    size_t received_bytes_offset, bytes_to_copy;
    
    ssize_t retval = 0;
    PDEBUG("read %ld bytes with offset %lld",count,*f_pos);

    (void) mutex_lock_interruptible(&(dev->writeLock));                 //Type cast the mutex lock code because we dont use it
    circBuf = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->buff), *f_pos, &received_bytes_offset);    //Save into
    mutex_unlock(&(dev->writeLock));                                    //Release the mutex

    if (circBuf == NULL){                                                       //Check to see if there are any entries inside the circular buffer

    	PDEBUG("No entry found to read");
    	return 0;
    }
    PDEBUG("String found %s at offset %ld", &circBuf->buffptr[received_bytes_offset], received_bytes_offset);
    PDEBUG("Full string: %s", circBuf->buffptr);
    bytes_to_copy = ((circBuf->size - received_bytes_offset) > count) ? count : (circBuf->size - received_bytes_offset);    //Figure out the number of characters inside of the buffer
    retval = bytes_to_copy - copy_to_user(buf, &circBuf->buffptr[received_bytes_offset], bytes_to_copy);        //Copy the characters to user space subtracting from the known number for error checking
    *f_pos += retval;                           //Save the current position based on the returned number of characters
    
    PDEBUG("Copied %ld bytes to user", retval);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *) filp->private_data;
    struct aesd_buffer_entry *entry = NULL;
    char *temp_buf;

    ssize_t retval = -ENOMEM;
    PDEBUG("Write %ld bytes with offset %lld",count,*f_pos);

    temp_buf = (char *) kmalloc(count + dev->partial_len, GFP_KERNEL);   //Ask the kernel for a space for a new temporary buffer
    if (dev->partial_write != NULL){                                    //If partial write isnt NULL then we know we append to our text

    	strncpy(temp_buf, dev->partial_write, dev->partial_len);         //String copy from the passed in file temp buffer, and set it to the temporary buffer before we add text
    }
    PDEBUG("Buffer before appending write %s", temp_buf);
    retval = count - copy_from_user(&temp_buf[dev->partial_len], buf, count);
    PDEBUG("Buffer after appending %s", temp_buf);

    if (temp_buf[count + dev->partial_len - 1] == '\n'){               //If we recieve a newline character then we know we got a full write text; Otherwise append

    	PDEBUG("Writing %s to buffer", temp_buf);
    	entry = (struct aesd_buffer_entry*) kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
    	entry->buffptr = temp_buf;
    	entry->size = dev->partial_len + count;
    	(void) mutex_lock_interruptible(&(dev->writeLock));     //Type cast the mutex lock check to void because we dont want to use it
    	aesd_circular_buffer_add_entry(&(dev->buff), entry);         //Add an entry to our circular buffer using the buffer
    	mutex_unlock(&(dev->writeLock));
    	kfree(dev->partial_write);                             //Free the kmalloc we did earlier
    	dev->partial_write = NULL;                                  //Set partial write to NULL so nothing is carried over
    	dev->partial_len = 0;                                       //Set partial length to 0 so nothing is carried
    }
    else{

        PDEBUG("Partial write of %s", temp_buf);
        kfree(dev->partial_write);                              //Free the kmalloc we did earlier
	    dev->partial_write = temp_buf;                               //Save the partial text in the struct
	    dev->partial_len += retval;                                 //Save the length of the partial text in the struct
    }

    return retval;
}
struct file_operations aesd_fops = {                       //File operations as given
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int error, deviceno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->chardev, &aesd_fops);                   //Initialize the character device
    dev->chardev.owner = THIS_MODULE;                       //Declare this module as the owner
    dev->chardev.ops = &aesd_fops;                          //Declare our file operations
    error = cdev_add(&dev->chardev, deviceno, 1);           //Ask the kernel for a character device
    if (error) {                                            //If an error is indicated print and return
        printk(KERN_ERR "Error %d adding aesd cdev", error);
    }
    return error;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");  //Allocate the device region
    aesd_major = MAJOR(dev);                                        //Ask for the device major and return as result
    if (result < 0) {                                               //Check if we have a major and error if not
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));   //Set the memory region to 0 so code wont be reused
    
    aesd_circular_buffer_init(&aesd_device.buff);  //Declare the circular buffer we wrote last time
    aesd_device.partial_write = NULL;       //Set the region to null so we dont potentially reuse old code
    aesd_device.partial_len = 0;            //Set the length to zero
    mutex_init(&(aesd_device.writeLock));   //Declare the mutex region to lock

    result = aesd_setup_cdev(&aesd_device);

    if(result) {                            //If result indicates an error unregister the device region
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t deviceno = MKDEV(aesd_major, aesd_minor);
    struct aesd_buffer_entry *entry;
    uint8_t index;

    cdev_del(&aesd_device.chardev);                //Delete the character device

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buff, index) {
    	if (entry->buffptr)                       //Check if there is a buffer allocated
    	{
    		kfree(entry->buffptr);         //Free the buffer we previously defined so it can be reused elsewhere
    	}
    }

    unregister_chrdev_region(deviceno, 1);      //Deregister the device region
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
