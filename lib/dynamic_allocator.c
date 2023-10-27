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

	struct BlockMetaData *myBlock = NULL;
	struct BlockMetaData *newBlock = NULL;

	LIST_FOREACH(myBlock, &memBlockList) {
		if (myBlock->is_free) {
			if (myBlock->size > size + sizeOfMetaData()) {
				newBlock = (struct BlockMetaData *)((char *)myBlock + sizeOfMetaData() + size);
				newBlock->size = myBlock->size - size - sizeOfMetaData();
				newBlock->is_free = 1;
				myBlock->is_free = 0;
				myBlock->size = size;
				if (newBlock) {
					LIST_INSERT_AFTER(&memBlockList, myBlock, newBlock);
				}

				return (void *)(myBlock + 1);
			} else if (myBlock->size == size + sizeOfMetaData()) {
				myBlock->is_free = 0;
				return (void *)(myBlock + 1);
			} else {
				if (myBlock->prev_next_info.le_next == NULL) {
					sbrk(0);
					return NULL;
				}
				if (myBlock->size >= size) {
					myBlock->is_free=0;
					return (void *)(myBlock+1);
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
//	panic("alloc_block_BF is not implemented yet");
//	return NULL;

	if (size == 0 || size > (DYN_ALLOC_MAX_SIZE - sizeOfMetaData())) {
		return NULL;
	}

	struct BlockMetaData* new_blocK;

	// Initialize fit_block to NULL
	struct BlockMetaData* fit_block = NULL;

	struct BlockMetaData* list_iterator;
	LIST_FOREACH(list_iterator, &memBlockList) {
		if (list_iterator->is_free == 1) {
			if (list_iterator->size < size) {
				continue;
			} else if (list_iterator->size == size) {
				fit_block = list_iterator;
				break;
			} else {
				if (( list_iterator->size)-size < (fit_block->size)-size) {
					fit_block = list_iterator;
					list_iterator=(list_iterator)->prev_next_info.le_next;
				}
			}
		}
	}

	if (sbrk(size + sizeOfMetaData()) == (void *)-1) {
		return NULL;
	}

	new_blocK = (struct BlockMetaData *)sbrk(0);
	new_blocK->size = size;
	new_blocK->is_free = 0;
	new_blocK->prev_next_info.le_next = NULL;

//	if (fit_block == NULL) {
//		return NULL;
//	}
//
//	new_blocK = fit_block;
//	new_blocK->size = size;
//	new_blocK->is_free = 0;

	// Double-check this part: Ensure the list is properly maintained.
	// Verify the insertion logic.
	LIST_INSERT_BEFORE(&memBlockList, list_iterator, new_blocK);

	return (void *)(new_blocK - 1);
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
	block->is_free = 1;

	// Coalesce consecutive free blocks
	struct BlockMetaData* current;
	struct BlockMetaData* toRemove = NULL;
//	bool shouldRemove = 0;

	for (current = LIST_FIRST(&memBlockList); current != NULL; current = current->prev_next_info.le_next) {
		if (current->prev_next_info.le_next == block) {
			if (current->is_free) {
				current->size += block->size+sizeOfMetaData();
				LIST_REMOVE(&memBlockList, block);
//				shouldRemove = 1;
//				toRemove = block;
//				block = current;
//				break;
			}
		}
		if (current->prev_next_info.le_prev == block) {
			if (current->is_free) {
				block->size += current->size+sizeOfMetaData();
				LIST_REMOVE(&memBlockList, current);
//				shouldRemove = 1;
//				toRemove = current;
//				block = current;
//				break;
			}
		}
	}

//	if (shouldRemove) {
//
//	}

	if (block->prev_next_info.le_next) {
		struct BlockMetaData* nextBlock = block->prev_next_info.le_next;
		if (nextBlock->is_free) {
			block->size += nextBlock->size;
			LIST_REMOVE(&memBlockList, nextBlock);
		}
	}
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	panic("realloc_block_FF is not implemented yet");
	return NULL;
}
