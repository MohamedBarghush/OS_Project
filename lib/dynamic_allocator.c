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

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//initializing first metadata block
	struct BlockMetaData* init__blocK1= (struct BlockMetaData*)(daStart);
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	//=========================================
	//=========================================

	//setting size of next block = zero
	init__blocK1->size=initSizeOfAllocatedSpace;

	//setting metadata block status as free
	init__blocK1->is_free=1;

	//setting previous pointer to this block
	init__blocK1->prev_next_info.le_prev=init__blocK1;

	//setting pointer to null
	init__blocK1->prev_next_info.le_next=NULL;

	LIST_INIT(&memBlockList);

	LIST_INSERT_HEAD(&memBlockList, init__blocK1);


}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//empty size case
	if (size == 0)
	{
		return NULL;
	}

	struct BlockMetaData *myBlock = NULL;
	struct BlockMetaData *newBlock = NULL;

	LIST_FOREACH(myBlock, &memBlockList) {

		if (myBlock->is_free) {

			//saving the actual size of block in this variable
			int actualSize = myBlock->size - sizeOfMetaData();

			//case 1: if block can fit into free space easily
			if (actualSize > size + sizeOfMetaData()) {

				//initializing the new metadata block
				newBlock = (struct BlockMetaData *)((char *)myBlock + sizeOfMetaData() + size);
				//setting its size
				newBlock->size = myBlock->size - size - sizeOfMetaData();
				//setting free status
				newBlock->is_free = 1;

				//setting block status as not free
				myBlock->is_free = 0;
				//setting its size
				myBlock->size = size+sizeOfMetaData();
				if (newBlock) {

					LIST_INSERT_AFTER(&memBlockList, myBlock, newBlock);

				}


				return (void *)(myBlock + 1);

			}
			// case 2: if block exactly fits in free space
			else if (actualSize == size + sizeOfMetaData()) {

				//setting block status as not free
				myBlock->is_free = 0;

				return (void *)(myBlock + 1);

			}
			//case 3: if block doesn't fit in free space
			else {

				//case 3.1:if data block fits
				if (actualSize >= size) {


					myBlock->is_free=0;
					return (void *)(myBlock+1);


				}//if its the last block
				if (myBlock->prev_next_info.le_next == NULL) {
					// allocate new block
					struct BlockMetaData *new_block = (struct BlockMetaData *)sbrk(size + sizeOfMetaData());
					if ((void *)new_block == (void *)-1) {
						return NULL;
					}
					new_block->is_free = 0;
					new_block->size = ROUNDUP(size + sizeOfMetaData(), PAGE_SIZE);
					LIST_INSERT_TAIL(&memBlockList, new_block);
					free_block((void*)(new_block + 1));
					return alloc_block_FF(size);
				}
			}
		}
	}

	//law sbrk msh tmm w msh httrf3 tdeny msa7a hrga3 NULL
	struct BlockMetaData *new_block = (struct BlockMetaData *)sbrk(size + sizeOfMetaData());
	if ((void *)new_block == (void *)-1) {
		return NULL;
	}
	new_block->is_free = 0;
	new_block->size = ROUNDUP(size + sizeOfMetaData(), PAGE_SIZE);
	LIST_INSERT_TAIL(&memBlockList, new_block);
	free_block((void*)(new_block + 1));
	return alloc_block_FF(size);


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

//okay BF is another way to allocate memory but in this strategy we scan all memory and choose the
//smallest block which can fit the required size


//its like FF by 95% m3ada bas in block to allocate




	if (size == 0) {
	    return NULL;
	}

	if (!is_initialized)
	{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
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

	            }
	            else {

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
//	struct BlockMetaData* current = LIST_FIRST(&memBlockList);
//	while (current != NULL) {
//
//	    if (current->is_free == 1) {
//
//
//	        struct BlockMetaData* nextBlock = current->prev_next_info.le_next;
//	        while (nextBlock && nextBlock->is_free == 1)
//	           {
//
//						current->size += nextBlock->size;
//
//						nextBlock->size = 0;
//
//						nextBlock->is_free = 0;
//						// Adjust the next pointer before removing the current block
//						struct BlockMetaData* temp = nextBlock;
//
//						nextBlock = nextBlock->prev_next_info.le_next;
//
//						LIST_REMOVE(&memBlockList, temp);
//
//
//	        }
//	    }
//	    current = current->prev_next_info.le_next;
//	}
	struct BlockMetaData* intended_block = block->prev_next_info.le_next;
	if (intended_block) {
		if (intended_block->is_free == 1) {
			block->size += intended_block->size;
			intended_block->size = 0;
			intended_block->is_free = 0;
			LIST_REMOVE(&memBlockList, intended_block);
		}
	}
	intended_block = block->prev_next_info.le_prev;
	if (intended_block) {
		if (intended_block->is_free == 1) {
			intended_block->size += block->size;
			block->size = 0;
			block->is_free = 0;
			LIST_REMOVE(&memBlockList, block);
		}
	}
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");


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

   // bmsk el meta data ll block nfso mn el VA bta3o
	struct BlockMetaData *current = (struct BlockMetaData*)va - 1;

	// law ba3tly el size b zero keda 3ayzny a3ml free ll block da w return NULL
	if (new_size == 0) {

		free_block(va);
		return NULL;

	}

	// law el size kber l el block bta3y ana h3ml split ll block da w law el size kber 3n elmeta data
	//h3ml new block w a7ot feh meta data , return va


	if (current_block_size > new_size) {

		// Adjust the size of the existing block and possibly coalesce with the next free block

		current->size = new_size+sizeOfMetaData();

		//7tt elmeta data ahy law kant el space tkafeha w azyad haga tmm 8er keda
		//htt7sb internal fragmentation 3la el block bta3y


		int available = current_block_size - new_size;
		if (available >= sizeOfMetaData()) {


			struct BlockMetaData *new_meta_data = (struct BlockMetaData*)((char*)current + new_size +sizeOfMetaData());
			new_meta_data->size = available - sizeOfMetaData();
			new_meta_data->is_free = 1;


		}

		return va;
	}

	//in this case my block is smaller than the new requested size
	//so I will check the next block if free i will merge two blocks if not
	//i will free the current block and reallocate using FF strategy

	LIST_FOREACH(current, &memBlockList) {

		if (current->prev_next_info.le_next->is_free && current->prev_next_info.le_next->size >= new_size)
		{

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
