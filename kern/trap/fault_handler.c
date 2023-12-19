/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
	//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
	//refer to the project presentation and documentation for details
	if(isPageReplacmentAlgorithmFIFO())
	{
		//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		// Write your code here, remove the panic and write your code
//		panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code
	//		panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
			cprintf("\n Started placing page \n");
			struct FrameInfo* framer_info = NULL;
//			placement(curenv, fault_va, &wsSize, framer_info);
			if (allocate_frame(&framer_info) != 0) {
				cprintf("This is my killer\n");
				sched_kill_env(curenv->env_id);
			}

			map_frame(curenv->env_page_directory, framer_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);

			int EPF = pf_read_env_page(curenv, (void*)fault_va);
			if (EPF == E_PAGE_NOT_EXIST_IN_PF) {
				if ((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)) {
					struct WorkingSetElement* wse = env_page_ws_list_create_element(curenv, fault_va);
					wse->virtual_address = fault_va;
					LIST_INSERT_HEAD(&(curenv->page_WS_list), wse);
					pf_read_env_page(curenv, (void*) fault_va);
					cprintf("This is the last element's data BEFORE: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
					wsSize  += 1;
					curenv->page_last_WS_index = (wsSize-1) % (curenv->page_WS_max_size-1); // (MS3 added, updated the last element index)
					if (wsSize >= curenv->page_WS_max_size) {
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
					}
					cprintf("This is the last element's data AFTER: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
					cprintf("\n Finished placing page \n");
					return;
				} else {
		//					cprintf("didn't pass the test\n");
					sched_kill_env(curenv->env_id);
				}
			} else {
				struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
				newElement->virtual_address = fault_va;
				LIST_INSERT_HEAD(&curenv->page_WS_list, newElement);
				cprintf("This is the last element's data BEFORE: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
				wsSize += 1;
				curenv->page_last_WS_index = (wsSize-1) % (curenv->page_WS_max_size-1); // (MS3 added, updated the last element index)
				if (wsSize >= curenv->page_WS_max_size) {
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				cprintf("This is the last element's data AFTER: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
			}
			cprintf("\n Finished placing page \n");
		}
		else
		{
			// Replacement
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

			// Get the page permissions of the last working set element in the current environment
			int tm = pt_get_page_permissions(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address);

			// Check if the page is modified
			if (tm & PERM_MODIFIED)
			{
			    // If modified, obtain the page table and frame information
			    uint32* pageTa = NULL;
			    get_page_table(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address, &pageTa);
			    struct FrameInfo* frame_info_ptr = get_frame_info(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address, &pageTa);

			    // Update the page table and mark the page as not modified
			    pf_update_env_page(curenv, curenv->page_last_WS_element->virtual_address, frame_info_ptr);
			}

			// Allocate a new frame for the faulting virtual address
			struct FrameInfo* NF = NULL;
			allocate_frame(&NF);

			// Map the new frame to the faulting virtual address with specified permissions
			map_frame(curenv->env_page_directory, NF, fault_va, PERM_PRESENT | PERM_WRITEABLE | PERM_USER);

			// Read the contents of the page into memory
			pf_read_env_page(curenv, (void*)fault_va);

			// Create a new working set element for the faulting virtual address
			struct WorkingSetElement* NewWS = env_page_ws_list_create_element(curenv, fault_va);
			NewWS->virtual_address = fault_va;

			// Insert the new working set element after the last one in the list
			LIST_INSERT_AFTER(&(curenv->page_WS_list), curenv->page_last_WS_element, NewWS);

			// Unmap the frame associated with the previous last working set element
			unmap_frame(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address);

			// Remove the previous last working set element from the list
			LIST_REMOVE(&curenv->page_WS_list, curenv->page_last_WS_element);

			// Update the last working set element to the new one
			curenv->page_last_WS_element = NewWS;

			// Adjust the last working set element based on its position in the list
			if (curenv->page_last_WS_element->prev_next_info.le_next == NULL)
			{
			    curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
			}
			else
			{
			    curenv->page_last_WS_element = NewWS->prev_next_info.le_next;
			}



		}
	}
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		// Write your code here, remove the panic and write your code
//		panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
		if (curenv->ActiveList.size + curenv->SecondList.size < curenv->ActiveListSize + curenv->SecondListSize) {

		} else {

		}

		//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
	}
}

void placement (struct Env * curenv, uint32 fault_va, uint32* wsSize, struct FrameInfo* framer_info) {
	if (allocate_frame(&framer_info) != 0) {
		cprintf("This is my killer\n");
		sched_kill_env(curenv->env_id);
	}

	map_frame(curenv->env_page_directory, framer_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);

	int EPF = pf_read_env_page(curenv, (void*)fault_va);
	if (EPF == E_PAGE_NOT_EXIST_IN_PF) {
		if ((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)) {
			struct WorkingSetElement* wse = env_page_ws_list_create_element(curenv, fault_va);
			LIST_INSERT_TAIL(&(curenv->page_WS_list), wse);
			pf_read_env_page(curenv, (void*) fault_va);
			cprintf("This is the last element's data BEFORE: Virtual Address: %p, Index: %d", curenv->page_last_WS_element, curenv->page_last_WS_index);
			*wsSize  += 1;
			curenv->page_last_WS_index = (*wsSize-1) % (curenv->page_WS_max_size-1); // (MS3 added, updated the last element index)
			if (*wsSize >= curenv->page_WS_max_size) {
				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			}
			cprintf("This is the last element's data AFTER: Virtual Address: %p, Index: %d", curenv->page_last_WS_element, curenv->page_last_WS_index);
			return;
		} else {
//					cprintf("didn't pass the test\n");
			sched_kill_env(curenv->env_id);
		}
	} else {
		struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
		LIST_INSERT_TAIL(&curenv->page_WS_list, newElement);
		cprintf("This is the last element's data BEFORE: Virtual Address: %p, Index: %d", curenv->page_last_WS_element, curenv->page_last_WS_index);
		*wsSize += 1;
		curenv->page_last_WS_index = (*wsSize-1) % (curenv->page_WS_max_size-1); // (MS3 added, updated the last element index)
		if (*wsSize >= curenv->page_WS_max_size) {
			curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
		}
		cprintf("This is the last element's data AFTER: Virtual Address: %p, Index: %d", curenv->page_last_WS_element, curenv->page_last_WS_index);
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



