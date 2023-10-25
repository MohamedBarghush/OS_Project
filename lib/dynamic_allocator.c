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
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
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
    init__blocK1->prev_next_info.le_prev=NULL;
    init__blocK1->prev_next_info.le_next=NULL;

	LIST_INIT(&memBlockList);

	LIST_INSERT_HEAD(&memBlockList, init__blocK1);

}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	if (size == 0 || size > DYN_ALLOC_MAX_SIZE - sizeOfMetaData()) {
		return NULL;
	}

	struct BlockMetaData *myOldBlock;
	struct BlockMetaData *myBlock;

	LIST_FOREACH(myBlock, &memBlockList) {
		if (myBlock->is_free && myBlock->size >= size + sizeOfMetaData()) {
			if (myBlock->size > size + sizeOfMetaData() + DYN_ALLOC_MAX_BLOCK_SIZE) {
				struct BlockMetaData *newBlock = (struct BlockMetaData *)((char *)myBlock + sizeOfMetaData() + size);
				newBlock->size = myBlock->size - size - sizeOfMetaData();
				newBlock->is_free = 1;
				newBlock->prev_next_info.le_next = myBlock->prev_next_info.le_next;

				myBlock->size = size;
				myBlock->prev_next_info.le_next = newBlock;
			}

			myBlock->is_free = 0;
			return (void *)(myBlock + 1);
		}

		myOldBlock = myBlock;
	}

	// If no sufficiently large free block is found
	if (sbrk(size + sizeOfMetaData()) == (void *)-1) {
		return NULL;
	}

	myBlock = (struct BlockMetaData *)sbrk(0);
	myBlock->size = size;
	myBlock->is_free = 0;
	myBlock->prev_next_info.le_next = NULL;

	LIST_INSERT_AFTER(&memBlockList, myOldBlock, myBlock);
	cprintf("My thing is: %p and %p \n", myOldBlock, myBlock);
	return (void *)(myBlock + sizeOfMetaData());
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	panic("alloc_block_BF is not implemented yet");
	return NULL;
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

	// Coalesce with previous block
	struct BlockMetaData* current;
	LIST_FOREACH(current, &memBlockList) {
		if (current->prev_next_info.le_next == block) {
			if (current->is_free) {
				current->size += block->size + sizeOfMetaData();
				block = current;
				LIST_REMOVE(&memBlockList, block->prev_next_info.le_next);
			}
			break;
		}
	}

	// Coalesce with next block
	LIST_FOREACH(current, &memBlockList) {
		if (current == block->prev_next_info.le_next) {
			if (current->is_free) {
				block->size += current->size + sizeOfMetaData();
				LIST_REMOVE(&memBlockList, current);
			}
			break;
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
