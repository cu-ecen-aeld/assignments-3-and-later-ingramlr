/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

#define indexing(index) (index%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
	size_t fpos_off=0;
	uint8_t start=buffer->out_offs;
	uint8_t end=buffer->in_offs;
	uint8_t total=AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

	if(buffer->full==false){
		if(start<end){
			total=end-start;
		}
		else if(start>end){
			total=total-start;
			total=total+end;
		}
		else{
			total=0;
		}
	}

	while(total){
		
		struct aesd_buffer_entry *entryptr=&((buffer)->entry[start]);
		size_t size=entryptr->size;
		fpos_off=fpos_off+size;
		if(fpos_off>char_offset){
			*entry_offset_byte_rtn=char_offset-fpos_off+size;
			return entryptr;
		}
		start=start+1;
		if(start==AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			start=0;
		}
		total--;
	}


	return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if(buffer->in_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)){
        
        memcpy(&(buffer->entry[buffer->in_offs]), add_entry, sizeof(struct aesd_buffer_entry));
        if(buffer->in_offs == buffer->out_offs)
        	buffer->out_offs = 0;
        buffer->in_offs = 0;
        buffer->full = true;
    }
    
    else if(buffer->full == true){

    	if(buffer->in_offs == buffer->out_offs) buffer->out_offs++;

    	memcpy(&(buffer->entry[buffer->in_offs++]), add_entry, sizeof(struct aesd_buffer_entry));
    }
    else memcpy(&(buffer->entry[buffer->in_offs++]), add_entry, sizeof(struct aesd_buffer_entry));
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
