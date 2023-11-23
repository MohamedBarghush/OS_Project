#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"


int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
//	panic("not implemented yet");
//	return 0;

	// Set up initial kernel heap memory region
	kinit.start = daStart;
	kinit.segment_break = daStart + initSizeToAllocate;
	kinit.hard_limit = daLimit;

	// Loop through the kernel heap memory region and allocate frames
	for (uint32 i = kinit.start; i < kinit.hard_limit; i += PAGE_SIZE) {
	    // Declare a pointer to a FrameInfo structure and initialize it to NULL
	    struct FrameInfo *pfi = NULL;

	    // Attempt to allocate a frame for the current address i
	    if (allocate_frame(&pfi) != 0) {
	        // If allocation fails, return the error code E_NO_MEM
	        return E_NO_MEM;
	    }

	    // Map the allocated frame to the specified address i in the page directory
	    map_frame(ptr_page_directory, pfi, i, PERM_WRITEABLE);
	}

	// Initialize the dynamic allocator for the kernel heap
	initialize_dynamic_allocator(daStart, initSizeToAllocate);

	// Return 0 to indicate successful initialization
	return 0;

}

void* sbrk(int increment)
{
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */

	// Save the current state of the segment break
	uint32 old_sbrk = kinit.segment_break;
	uint32 current_sbrk = kinit.segment_break;
	bool is_updated = 0;

	// Declare a pointer to a FrameInfo structure and an integer pointer
	struct FrameInfo *ptrNewFrame = NULL;
	uint32 *x = NULL; // Replace NULL with an appropriate initial value

	// If increment is 0, return the current segment break
	if (increment == 0) {
	    return (void *)old_sbrk;
	}

	// Check if increasing the segment break would exceed the hard limit
	if (current_sbrk + increment >= kinit.hard_limit) {
	    panic("cannot allocate memory, exceeded hard limit");
	}

	// Increase the segment break if increment is positive
	if (increment > 0) {
	    // Calculate the number of needed pages
	    int needed_pages = ROUNDUP(increment, PAGE_SIZE) / PAGE_SIZE;
	    current_sbrk += needed_pages * PAGE_SIZE;

	    // Allocate frames and map them to the corresponding addresses
	    for (int i = old_sbrk; i < current_sbrk; i += PAGE_SIZE) {
	        ptrNewFrame = NULL;
	        allocate_frame(&ptrNewFrame);
	        ptrNewFrame->va = i;
	        map_frame(ptr_page_directory, ptrNewFrame, i, PERM_WRITEABLE);
	    }

	    // Set the update flag to true
	    is_updated = 1;
	}
	// Decrease the segment break if increment is negative
	else if (increment < 0) {
	    if (-increment > PAGE_SIZE) {
	    	// Calculate the number of needed pages
			int needed_pages = ROUNDUP(-increment, PAGE_SIZE) / PAGE_SIZE;
			current_sbrk -= needed_pages * PAGE_SIZE;

			// Free frames and unmap them from the corresponding addresses
			for (int i = old_sbrk; i > current_sbrk; i -= PAGE_SIZE) {
				ptrNewFrame = get_frame_info(ptr_page_directory, i, &x);
				free_frame(ptrNewFrame);
				unmap_frame(ptr_page_directory, i);
			}
	    } else {
	    	current_sbrk -= increment;
	    }

	    // Set the update flag to true
	    is_updated = -1;
	}

	// Update the segment break if there was an update
	if (is_updated == 1) {
	    kinit.segment_break = current_sbrk;
	    return (void *)old_sbrk;
	} else if (is_updated == -1) {
		kinit.segment_break = current_sbrk;
		return (void *)current_sbrk;
	} else {
	    // Handle error appropriately (return NULL, set an error code, etc.)
	    panic("cannot allocate/deallocate memory");
	    return NULL;  // Added return NULL to suppress a warning
	}
}

struct kheap_data kData[NUM_OF_KHEAP_PAGES];
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	/*cprintf ("%d size to allocate \n", size);
	cprintf ("%p my hard limit \n", kinit.hard_limit);
	cprintf ("%p my start \n", kinit.start);
	cprintf ("%p my segment break \n", kinit.segment_break);*/

	uint32 start = 0;

	// Check if the kernel heap placement strategy is FIRSTFIT
	if (isKHeapPlacementStrategyFIRSTFIT() == 0) {
	    return NULL;  // Return NULL if not using FIRSTFIT strategy
	}

	// Check if the requested size exceeds the available space in the kernel heap
	if (size > kinit.hard_limit - kinit.start) {
	    return NULL;  // Return NULL if insufficient space in the kernel heap
	}

	// Check if the requested size is within the range that can be handled by the FIRSTFIT strategy
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
	    // If the size is within the range, allocate a block using the FIRSTFIT strategy
	    void* block = alloc_block_FF(size);
	    if (block != NULL) {
	        return block;  // Return the allocated block if successful
	    }
	}
	else {
	    // For larger allocations, use a ROUNDUP strategy to find a suitable contiguous block of memory
	    uint32 num_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	    uint32 total_size = num_pages * PAGE_SIZE;

	    uint32 addr = kinit.hard_limit + PAGE_SIZE;
	    int count = 0;

	    // Iterate through the kernel heap region starting from the next available address
	    // after the current segment break (kinit.hard_limit + PAGE_SIZE), incrementing by
	    // PAGE_SIZE each time, until reaching the maximum address (KERNEL_HEAP_MAX).
	    for (int i = kinit.hard_limit + PAGE_SIZE; i < KERNEL_HEAP_MAX; i += PAGE_SIZE) {
	        // Check if there is enough space in the current heap block
	        if (count == num_pages) {
	            break;  // Break the loop if enough space is found
	        }

	        uint32 *ptr_page = NULL;
	        uint32 table_ = get_page_table(ptr_page_directory, i, &ptr_page);
	        uint32 page_ = ptr_page[PTX(i)];

	        // Check if the page is present in the page table and has the present permission
	        if (page_ && (PERM_PRESENT)) {
	            count = 0;
	            addr = i + PAGE_SIZE;
//	            tlb_invalidate (ptr_page_directory, &addr);
	            continue;  // Reset count and address if the page is already allocated
	        }
	        else {
	            count++;
	        }
	    }

	    // If enough space is found, allocate and map the page frames
	    if (count == num_pages) {
	        start = addr;

	        for (int i = 0; i < num_pages; i++) {
	            // Allocate the page frame
	            struct FrameInfo *ptr_info = NULL;
	            int all_err = allocate_frame(&ptr_info);

	            if (all_err == 0) {
	                // Map the page frame to the virtual address
	            	ptr_info->va = addr + PAGE_SIZE + PAGE_SIZE*i;
	                map_frame(ptr_page_directory, ptr_info, addr, PERM_WRITEABLE);
	            }
	            addr += PAGE_SIZE;
	        }

//	        return (void*)start;  // Return the starting address of the allocated memory block
	    }
	}
	for (int i = 0; i < NUM_OF_KHEAP_PAGES; i++) {
		if (kData[i].start == (uint32*)0) {
			kData[i].start = (uint32*)start;
			kData[i].size = size;
			return (void*)start;
		}
	}
	return NULL;  // Return NULL if allocation is unsuccessful

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code


	// Convert the virtual address to an unsigned 32-bit integer
	uint32 va = (uint32)virtual_address;

	// Round down the virtual address to the nearest page boundary
	//uint32 va = ROUNDDOWN((uint32)positive_va, PAGE_SIZE);

	// Set the start of the physical memory range for kernel heap
	uint32 kernel_heap_start = (uint32)(kinit.hard_limit + PAGE_SIZE);

	// Check if the virtual address is within the kernel heap region
	if (va >= KERNEL_HEAP_START && va < kinit.hard_limit) {
//		cprintf("Freeing Block Allocator space \n");
	    // The virtual address is within the kernel heap region
	    free_block((uint32*)va); // Free the block associated with the virtual address
	} else if (va >=	 kernel_heap_start && va < KERNEL_HEAP_MAX) {
//		cprintf("This is my segment_brk: %p\n", kernel_heap_start);
	    // Get Table position from the directory table
		// Get the page table itself from the memory
		uint32* page_table_address = NULL;
		get_page_table(ptr_page_directory, va, &page_table_address);
//		cprintf("This is the page table pointer: %p\n", page_table_address);
		// Iterate over every single entry in the page table and:
		//		- Unmap the frame
		int index = 0;
		for (int i = 0; i < NUM_OF_KHEAP_PAGES; i++) {
			if (va == (uint32)kData[i].start) {
				index = i;
			}
		}

		for (int i = 0; i < kData[index].size; i++) {
//			cprintf("My Table is at: %p\n", i);
//			if (i == 0) {
//				cprintf("Exit NIGGA\n");
//				continue;
//			}
			uint32* temp = NULL;
			struct FrameInfo * ptr_new_frame = get_frame_info(ptr_page_directory, (uint32)kData[index].start+i, &temp);
//			cprintf("This is my frame %p - %p\n", positive_va, ptr_new_frame->va);
//			free_frame(ptr_new_frame);
			unmap_frame(ptr_page_directory, (uint32)kData[index].start+i);
		}
		// Unmap the frame of the page table itself
	} else {
//		cprintf("Invalid space \n");
	    // The virtual address is invalid
	    panic("ENTER INVALID ADDRESS !!\n");
	}

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
