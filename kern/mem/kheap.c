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

	kinit.start = daStart;
	kinit.segment_break = daStart + initSizeToAllocate;
	kinit.hard_limit = daLimit;

	uint32 *x = NULL;
	for (uint32 i = kinit.start; i < kinit.segment_break; i += PAGE_SIZE) {
		struct FrameInfo *pfi = get_frame_info(ptr_page_directory, i, &x);
		if (pfi == NULL) {
			//fr1 = allocate_frame(pfi);
			allocate_frame(&pfi);
		}
		if (pfi != NULL) {
			return 0;
		}
		map_frame(ptr_page_directory, pfi, i, PERM_PRESENT | PERM_WRITEABLE);
	}


	initialize_dynamic_allocator(daStart, initSizeToAllocate);
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

	uint32 old_sbrk = kinit.segment_break;
	uint32 current_sbrk = kinit.segment_break;
	bool is_updated = 0;

	struct FrameInfo *ptrNewFrame = NULL;
	uint32 *x = NULL; // Replace some_initial_value with an appropriate initial value

	if (increment == 0) {
		return (void *)old_sbrk;
	}
	if(current_sbrk + increment  >=  (kinit.hard_limit)){
		panic("can not allocate memory");
	}

	if (increment > 0 && current_sbrk + increment  <=  (kinit.hard_limit)) {
		if (increment * 1024 < 4 * 1024) {
			current_sbrk += 4 * 1024;
			// only map one frame
			ptrNewFrame = get_frame_info(ptr_page_directory, current_sbrk, &x);
			allocate_frame(&ptrNewFrame);
			map_frame(ptr_page_directory, ptrNewFrame, current_sbrk, PERM_PRESENT | PERM_WRITEABLE);

			is_updated = 1;
		} else {
			int needed_pages = increment / (4 * 1024) + 1;
			current_sbrk += needed_pages * 4 * 1024;
			// map chunks of frames
			for (int i = old_sbrk; i < current_sbrk; i += PAGE_SIZE) {
				ptrNewFrame = get_frame_info(ptr_page_directory, i, &x);
				allocate_frame(&ptrNewFrame);
				map_frame(ptr_page_directory, ptrNewFrame, i, PERM_PRESENT | PERM_WRITEABLE);
			}
			is_updated = 1;
		}

	}
	else if (increment < 0) {
		if (-increment  >= 4 * 1024) {
			float needed_pages = -increment / (4 * 1024) ;
			old_sbrk=current_sbrk -= ROUNDDOWN(needed_pages * 4 * 1024,(int )-increment / (4 * 1024)+1);
//            current_sbrk -= ;
			// unmap chunks of frames
			for (int i = old_sbrk; i > current_sbrk; i -= PAGE_SIZE) {
				ptrNewFrame = get_frame_info(ptr_page_directory, i, &x);
				// No need to check if frame_was_mapped_at_address since it's not in your code
				free_frame(ptrNewFrame);
				unmap_frame(ptr_page_directory, i);
			}
			is_updated = 1;
		} else if (-increment  < 4 * 1024 && current_sbrk - 4 * 1024 >= kinit.start) {
			// unmap one frame
			// No need to check if frame_was_mapped_at_address since it's not in your code
			ptrNewFrame = get_frame_info(ptr_page_directory, current_sbrk, &x);
			free_frame(ptrNewFrame);
			unmap_frame(ptr_page_directory, current_sbrk);
			old_sbrk=current_sbrk -= 4 * 1024;
			is_updated = 1;
		}
	}

	if (is_updated) {
		return (void *)old_sbrk;
	} else {
		// Handle error appropriately (return NULL, set an error code, etc.)
		return (void *)-1;
		panic("can not allocate memory");

	}
}


void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	cprintf ("%d size to allocate \n", size);
	cprintf ("%p my hard limit \n", kinit.hard_limit);
	cprintf ("%p my start \n", kinit.start);
	cprintf ("%p my segment break \n", kinit.segment_break);

	if (isKHeapPlacementStrategyFIRSTFIT() == 0) {
		return NULL;
	}

	if (size > kinit.hard_limit) {
		return NULL;
	}

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		void* block = alloc_block_FF(size);
		if (block != NULL) {
			return block;
		}
	}
	else
	{
		uint32 num_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
		uint32 total_size = num_pages * PAGE_SIZE;

		if (kinit.segment_break + total_size > kinit.hard_limit) {
            return NULL;  // Not enough space in the kernel heap
        }

		uint32 start_k = KERNEL_HEAP_START;
		for (int i = 0; i < num_pages; i++) {
			uint32* ptr_page_table = NULL;
			get_page_table(ptr_page_directory, start_k + i * PAGE_SIZE, &ptr_page_table);
			if (ptr_page_table == NULL || (ptr_page_table[PTX(start_k + i * PAGE_SIZE)] & ~PERM_PRESENT)) {
				struct FrameInfo* ptr_info;
				int alloc_err = allocate_frame(&ptr_info);
				if (alloc_err == 0) {
					int map_err = map_frame(ptr_page_directory, ptr_info, start_k + i * PAGE_SIZE, PERM_WRITEABLE);
					if (map_err != 0) {
						return NULL;
					}
				}
			}
		}

		kinit.segment_break += total_size;

		return (void*)(kinit.segment_break - total_size);
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
// 	uint32 va=(uint32 )virtual_address;
// 	uint32 viradd=ROUNDDOWN((uint32 )virtual_address,PAGE_SIZE);
// 	uint32 pstrt = (uint32)(kinit.hard_limit + PAGE_SIZE);
// 	if(va>=KERNEL_HEAP_START && va<=kinit.hard_limit){
// //		struct FrameInfo *ff =get_frame_info(ptr_page_directory, viradd, &PT);
// //        unmap_frame(ptr_page_directory,viradd);
// //        tlb_invalidate(ptr_page_directory, (uint32*)viradd);
// //        free_block((uint32*)viradd);
// //        free_frame(ff);
// 		  free_block((uint32*)viradd);

// 	} else if (viradd>=pstrt&&viradd <=KERNEL_HEAP_MAX){
// 		uint32 *PT;
// 		struct FrameInfo *ff =get_frame_info(ptr_page_directory, viradd, &PT);
// 	    unmap_frame(ptr_page_directory,viradd);
// 	    //tlb_invalidate(ptr_page_directory, (uint32*)viradd);
// 	    free_frame(ff);
// 	}else{
// 		panic("ENTER INVALIDE ADDRESS !!\n");
// 	}

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
