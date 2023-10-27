/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d, current: %p, next: %p)\n", blk->size, blk->is_free, blk, blk->prev_next_info.le_next) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
    struct BlockMetaData* init__blocK1= (struct BlockMetaData*)(daStart);
    //=========================================
    //DON'T CHANGE THESE LINES=================
    if (initSizeOfAllocatedSpace == 0)
        return ;
    //=========================================
    //=========================================

    init__blocK1->size=initSizeOfAllocatedSpace;
    init__blocK1->is_free=1;
    init__blocK1->prev_next_info.le_prev=init__blocK1;
    init__blocK1->prev_next_info.le_next=NULL;

	LIST_INIT(&memBlockList);

	LIST_INSERT_HEAD(&memBlockList, init__blocK1);
//	print_blocks_list(memBlockList);
//	cprintf("Fuck you");

//	cprintf("The list is: %p, %p\n", init__blocK1, init__blocK1->prev_next_info.le_next);
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
    if (size == 0) {
		return NULL;
	}

//    cprintf("Trying to allocate size of: %d %p with size of %d \n", size, LIST_FIRST(&memBlockList), LIST_FIRST(&memBlockList)->size);

	struct BlockMetaData *myBlock = NULL;
	struct BlockMetaData *newBlock = NULL;

	LIST_FOREACH(myBlock, &memBlockList) {
		if (myBlock->is_free) {
			int actualSize = myBlock->size - sizeOfMetaData();
			if (actualSize > size + sizeOfMetaData()) {
				newBlock = (struct BlockMetaData *)((char *)myBlock + sizeOfMetaData() + size);
				newBlock->size = myBlock->size - size - sizeOfMetaData();
				newBlock->is_free = 1;
				myBlock->is_free = 0;
				myBlock->size = size+sizeOfMetaData();
				if (newBlock) {
					LIST_INSERT_AFTER(&memBlockList, myBlock, newBlock);
				}

				return (void *)(myBlock + 1);
			} else if (actualSize == size + sizeOfMetaData()) {
				myBlock->is_free = 0;
				return (void *)(myBlock + 1);
			} else {
				if (actualSize >= size) {
					myBlock->is_free=0;
					return (void *)(myBlock+1);
				}
				if (myBlock->prev_next_info.le_next == NULL) {
					sbrk(0);
					return NULL;
				}
			}
		}
	}

	if (sbrk(size + sizeOfMetaData()) == (void *)-1) {
		return NULL;
	}

//	myBlock = (struct BlockMetaData *)sbrk(0);
//	myBlock->size = size;
//	myBlock->is_free = 0;
//	myBlock->prev_next_info.le_next = NULL;
//
//	LIST_INSERT_AFTER(&memBlockList, LIST_LAST(&memBlockList), myBlock);

	return (void *)(myBlock + 1);
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
    //TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
//    panic("alloc_block_BF is not implemented yet");
//    return NULL;

	if (size == 0) {
	    return NULL;
	}

	struct BlockMetaData *myBlock = NULL;
	struct BlockMetaData *newBlock = NULL;

	struct BlockMetaData* smallestSize = NULL;

	LIST_FOREACH(myBlock, &memBlockList) {
	    if (myBlock->is_free) {
	        int actualSize = myBlock->size - sizeOfMetaData();
	        if (actualSize >= size) {
	            if (smallestSize != NULL) {
					if (actualSize < smallestSize->size-sizeOfMetaData()) {
						smallestSize = myBlock;
					}
	            } else {
	            	smallestSize = myBlock;
	            }
	        }
	    }
	}


	if (smallestSize == NULL) {
		sbrk(0);
		return NULL;
	} else {
		if ((smallestSize->size-sizeOfMetaData()) > size + sizeOfMetaData()) {
			newBlock = (struct BlockMetaData *)((char *)smallestSize + sizeOfMetaData() + size);
			newBlock->size = smallestSize->size - size - sizeOfMetaData();
			newBlock->is_free = 1;
			smallestSize->is_free = 0;
			smallestSize->size = size + sizeOfMetaData();
			LIST_INSERT_AFTER(&memBlockList, smallestSize, newBlock);
			return (void *)(smallestSize + 1);
		} else if ((smallestSize->size - sizeOfMetaData()) == size + sizeOfMetaData()) {
			smallestSize->is_free = 0;
			return (void*)(smallestSize+1);
		} else {
			if ((smallestSize->size-sizeOfMetaData()) >= size) {
				smallestSize->is_free = 0;
				return (void*)(smallestSize+1);
			} else {
				sbrk(0);
				return NULL;
			}
		}
	}

	if (sbrk(size + sizeOfMetaData()) == (void *)-1) {
		return NULL;
	}

	return (void *)(myBlock+1);
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
//	panic("free_block is not implemented yet");
	if (va == NULL) {
		return;
	}

	struct BlockMetaData* block = (struct BlockMetaData*)va - 1;
	if (block == NULL) {
		return;
	}
//	int previousSize = block->size;
	block->is_free = 1;

	// Coalesce consecutive free blocks
	struct BlockMetaData* current = LIST_FIRST(&memBlockList);
	while (current != NULL) {
	    if (current->is_free == 1) {
	        struct BlockMetaData* nextBlock = current->prev_next_info.le_next;
	        while (nextBlock && nextBlock->is_free == 1) {
	            current->size += nextBlock->size;
	            nextBlock->size = 0;
	            nextBlock->is_free = 0;
	            // Adjust the next pointer before removing the current block
	            struct BlockMetaData* temp = nextBlock;
	            nextBlock = nextBlock->prev_next_info.le_next;
	            LIST_REMOVE(&memBlockList, temp);
	        }
	    }
	    current = current->prev_next_info.le_next;
	}
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
//	panic("realloc_block_FF is not implemented yet");
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");
	// If va is NULL and new_size is 0, return NULL
	// If va is NULL and new_size is 0, return NULL
	if (va == NULL && new_size == 0) {
		return NULL;
	}

	// If va is NULL and new_size is not 0, allocate a new block
	if (va == NULL) {
		return alloc_block_FF(new_size);
	}

	// Get the current block size and metadata
	uint32 current_block_size = get_block_size(va);
//	current_block_size += sizeOfMetaData();
//	cprintf("old size: %d, new size: %d\n", current_block_size, new_size);
	struct BlockMetaData *current = (struct BlockMetaData*)va - 1;

	// If new_size is 0, free the current block and return NULL
	if (new_size == 0) {
		free_block(va);
		return NULL;
	}

	// If the current block size is already greater or equal to new_size, return va
	if (current_block_size >= new_size) {
		// Adjust the size of the existing block and possibly coalesce with the next free block
		current->size = new_size+sizeOfMetaData();

		// Check if there's enough space for a new free block after resizing
		int available = current_block_size - new_size;
		if (available >= sizeOfMetaData()) {
			struct BlockMetaData *new_meta_data = (struct BlockMetaData*)((char*)current + new_size);
			new_meta_data->size = available - sizeOfMetaData();
			new_meta_data->is_free = 1;
		}

		return va;
	}

	// to find a free block adjacent to the current block
	LIST_FOREACH(current, &memBlockList) {
		if (current->prev_next_info.le_next->is_free && current->prev_next_info.le_next->size >= new_size) {
			// Found a suitable free block, use it for realloc
			struct BlockMetaData *free_block = current->prev_next_info.le_next;
			int available = free_block->size - new_size;
			free_block->is_free = 0;
			free_block->size = 0;
			if (available >= sizeOfMetaData()) {
				struct BlockMetaData *new_meta_data = (struct BlockMetaData*)((char*)free_block + sizeOfMetaData());
				new_meta_data->size = available - sizeOfMetaData();
				new_meta_data->is_free = 1;
			}
			current->size = new_size + sizeOfMetaData();
			current->is_free = 0;
			return va;
		}
	}

	// If no suitable free block is found, use sbrk to create more space on the heap
	current->size = 0;
	current->is_free = 1;
	return alloc_block_FF(new_size);
}
