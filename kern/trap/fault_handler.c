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

// 2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
//  0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
// 2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE;
}
void setPageReplacmentAlgorithmCLOCK() { _PageRepAlgoType = PG_REP_CLOCK; }
void setPageReplacmentAlgorithmFIFO() { _PageRepAlgoType = PG_REP_FIFO; }
void setPageReplacmentAlgorithmModifiedCLOCK() { _PageRepAlgoType = PG_REP_MODIFIEDCLOCK; }
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal() { _PageRepAlgoType = PG_REP_DYNAMIC_LOCAL; }
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps)
{
	_PageRepAlgoType = PG_REP_NchanceCLOCK;
	page_WS_max_sweeps = PageWSMaxSweeps;
}

// 2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) { return _PageRepAlgoType == LRU_TYPE ? 1 : 0; }
uint32 isPageReplacmentAlgorithmCLOCK()
{
	if (_PageRepAlgoType == PG_REP_CLOCK)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmFIFO()
{
	if (_PageRepAlgoType == PG_REP_FIFO)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmModifiedCLOCK()
{
	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
		return 1;
	return 0;
}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal()
{
	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
		return 1;
	return 0;
}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK()
{
	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
		return 1;
	return 0;
}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt) { _EnableModifiedBuffer = enableIt; }
uint8 isModifiedBufferEnabled() { return _EnableModifiedBuffer; }

void enableBuffering(uint32 enableIt) { _EnableBuffering = enableIt; }
uint8 isBufferingEnabled() { return _EnableBuffering; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length; }
uint32 getModifiedBufferLength() { return _ModifiedBufferLength; }

//===============================
// FAULT HANDLERS
//===============================

// Handle the table fault
void table_fault_handler(struct Env *curenv, uint32 fault_va)
{
	// panic("table_fault_handler() is not implemented yet...!!");
	// Check if it's a stack page
	uint32 *ptr_table;
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

// Handle the page fault

void page_fault_handler(struct Env *curenv, uint32 fault_va)
{
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
	int iWS = curenv->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(curenv);
#endif

	fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
	if (isPageReplacmentAlgorithmFIFO())
	{
		// TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		//  Write your code here, remove the panic and write your code
		//		panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		// cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		// TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
		// Write your code here, remove the panic and write your code
		//		panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
		cprintf("\n Started placing page \n");
		struct FrameInfo *framer_info = NULL;
		//			placement(curenv, fault_va, &wsSize, framer_info);
		if (allocate_frame(&framer_info) != 0)
		{
			cprintf("This is my killer\n");
			sched_kill_env(curenv->env_id);
		}

		map_frame(curenv->env_page_directory, framer_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);

		int EPF = pf_read_env_page(curenv, (void *)fault_va);
		if (EPF == E_PAGE_NOT_EXIST_IN_PF)
		{
			if (!(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) && !(fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX))
			{
				//					cprintf("didn't pass the test\n");
				sched_kill_env(curenv->env_id);
			}
		}
		if(wsSize < (curenv->page_WS_max_size))
		{
		struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
		// newElement->virtual_address = fault_va;
		LIST_INSERT_TAIL(&curenv->page_WS_list, newElement);
		cprintf("This is the last element's data BEFORE: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
		wsSize += 1;
		// curenv->page_last_WS_index = (wsSize-1) % (curenv->page_WS_max_size-1); // (MS3 added, updated the last element index)
		if (wsSize == curenv->page_WS_max_size)
		{
			curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
		}
		cprintf("This is the last element's data AFTER: Virtual Address: %p, Index: %d\n", curenv->page_last_WS_element, curenv->page_last_WS_index);
		cprintf("\n Finished placing page \n");
	}
	else
	{
		// Replacement
		// TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		// Write your code here, remove the panic and write your code
		// panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

		struct FrameInfo* newFrame ;
		int ret = allocate_frame(&newFrame) ;
		map_frame(curenv->env_page_directory, newFrame, fault_va, PERM_WRITEABLE| PERM_USER | PERM_PRESENT) ;
		int read = pf_read_env_page((struct Env*)curenv, (void*)fault_va) ;
		if(read == E_PAGE_NOT_EXIST_IN_PF)
		{
			if(!(fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) && !(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM))
			{
				cprintf("FIFO Kill\n");
				sched_kill_env(curenv->env_id) ;
			}
		}

		// Get the page permissions of the last working set element in the current environment
		int tm = pt_get_page_permissions(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address);

		// Check if the page is modified
		if (tm & PERM_MODIFIED)
		{
			// If modified, obtain the page table and frame information
			uint32 *pageTa = NULL;
			// get_page_table(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address, &pageTa);
			struct FrameInfo *frame_info_ptr = get_frame_info(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address, &pageTa);

			// Update the page table and mark the page as not modified
			pf_update_env_page(curenv, curenv->page_last_WS_element->virtual_address, frame_info_ptr);
		}

		// Allocate a new frame for the faulting virtual address
		// struct FrameInfo* NF = NULL;
		// if(allocate_frame(&NF) != 0) {
		// 	sched_kill_env(curenv->env_id);
		// }

		// // Map the new frame to the faulting virtual address with specified permissions
		// map_frame(curenv->env_page_directory, NF, fault_va, PERM_PRESENT | PERM_WRITEABLE | PERM_USER);

		// // Read the contents of the page into memory
		// pf_read_env_page(curenv, (void*)fault_va);

		// // Create a new working set element for the faulting virtual address
		// struct WorkingSetElement* NewWS = env_page_ws_list_create_element(curenv, fault_va);
		// NewWS->virtual_address = fault_va;

		// // Insert the new working set element after the last one in the list
		// LIST_INSERT_AFTER(&(curenv->page_WS_list), curenv->page_last_WS_element, NewWS);

		// Unmap the frame associated with the previous last working set element
		unmap_frame(curenv->env_page_directory, curenv->page_last_WS_element->virtual_address);

		// Remove the previous last working set element from the list
		// LIST_REMOVE(&curenv->page_WS_list, curenv->page_last_WS_element);

		// Update the last working set element to the new one
		// curenv->page_last_WS_element = NewWS;

		if (curenv->page_last_WS_element == LIST_FIRST(&curenv->page_WS_list))
		{
			env_page_ws_invalidate(curenv, curenv->page_last_WS_element->virtual_address);
			struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
			LIST_INSERT_HEAD(&(curenv->page_WS_list), newElement);
		}
		else if (curenv->page_last_WS_element == LIST_LAST(&curenv->page_WS_list))
		{
			env_page_ws_invalidate(curenv, curenv->page_last_WS_element->virtual_address);
			struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
			LIST_INSERT_TAIL(&(curenv->page_WS_list), newElement);
			curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
		}
		else
		{
			struct WorkingSetElement *temp = curenv->page_last_WS_element->prev_next_info.le_prev;
			env_page_ws_invalidate(curenv, curenv->page_last_WS_element->virtual_address);
			struct WorkingSetElement *newElement = env_page_ws_list_create_element(curenv, fault_va);
			LIST_INSERT_AFTER(&(curenv->page_WS_list), temp, newElement);
		}
	}
}
else if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
{
	// cprintf("-----------------LRU part----------------\n");

	uint32 AWsize = LIST_SIZE(&(curenv->ActiveList));
	uint32 SWsize = LIST_SIZE(&(curenv->SecondList));
	int in_lru = 0;
	struct WorkingSetElement *Iterator = LIST_FIRST(&(curenv->SecondList));
	// Check if the faulting page is in the SecondList (LRU)
	LIST_FOREACH(Iterator, &(curenv->SecondList))
	{
		if (Iterator->virtual_address == fault_va)
		{
			// cprintf("Page found in LRU list\n");
			LIST_REMOVE(&(curenv->SecondList), Iterator);
			pt_set_page_permissions(curenv->env_page_directory, fault_va, PERM_PRESENT, 0);
			in_lru = 1;
			break;
		}
	}
	if (AWsize + SWsize < (curenv->page_WS_max_size))
	{

		if (!in_lru)
		{
			// cprintf("Page not in LRU list\n");

			struct FrameInfo *framer_info = NULL;
			if (allocate_frame(&framer_info) != 0)
			{
				// cprintf("Failed to allocate frame\n");
				sched_kill_env(curenv->env_id);
			}

			map_frame(curenv->env_page_directory, framer_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);

			int EPF = pf_read_env_page(curenv, (void *)fault_va);

			if (EPF == E_PAGE_NOT_EXIST_IN_PF)
			{
				if (!(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX))
				{
					// cprintf("Stack or heap page\n");
					sched_kill_env(curenv->env_id);
					return;
				}
			}

			struct WorkingSetElement *wse = env_page_ws_list_create_element(curenv, fault_va);
			wse->virtual_address = fault_va;

			if (AWsize < curenv->ActiveListSize)
			{

				LIST_INSERT_HEAD(&(curenv->ActiveList), wse);
				pt_set_page_permissions(curenv->env_page_directory, wse->virtual_address, PERM_PRESENT, 0);
			}
			else if (AWsize == curenv->ActiveListSize && SWsize < curenv->SecondListSize)
			{

				struct WorkingSetElement *victim = LIST_LAST(&(curenv->ActiveList));
				LIST_REMOVE(&(curenv->ActiveList), victim);
				LIST_INSERT_HEAD(&(curenv->SecondList), victim);
				pt_set_page_permissions(curenv->env_page_directory, victim->virtual_address, 0, PERM_PRESENT);
				LIST_INSERT_HEAD(&(curenv->ActiveList), wse);
				pt_set_page_permissions(curenv->env_page_directory, wse->virtual_address, PERM_PRESENT, 0);
			}
		}
		else
		{
			// Page is in SecondList (LRU)
			// if (AWsize == curenv->ActiveListSize) {
			// cprintf("Moving page from ActiveList to SecondList\n");
			struct WorkingSetElement *lastAWE = LIST_LAST(&(curenv->ActiveList));
			LIST_REMOVE(&(curenv->ActiveList), lastAWE);
			pt_set_page_permissions(curenv->env_page_directory, lastAWE->virtual_address, 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&(curenv->SecondList), lastAWE);
			LIST_INSERT_HEAD(&(curenv->ActiveList), Iterator);
			AWsize++;
			SWsize++;
			// }
		}
	}
	else if (AWsize + SWsize >= (curenv->page_WS_max_size))
	{

		// cprintf("\n\n Iam hereee\n");

		if (!in_lru)
		{
			struct FrameInfo *framer_info = NULL;
			if (allocate_frame(&framer_info) != 0)
			{
				// cprintf("Failed to allocate frame\n");
				sched_kill_env(curenv->env_id);
			}

			map_frame(curenv->env_page_directory, framer_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_PRESENT);
			framer_info->va = fault_va;

			int EPF = pf_read_env_page(curenv, (void *)fault_va);

			if (EPF == E_PAGE_NOT_EXIST_IN_PF)
			{
				if (!(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX))
				{
					// cprintf("not Stack or heap page\n");
					sched_kill_env(curenv->env_id);
					return;
				}
			}

			struct WorkingSetElement *victim = LIST_LAST(&(curenv->SecondList));
			uint32 lruVecPer = pt_get_page_permissions(curenv->env_page_directory, victim->virtual_address);
			if (lruVecPer & PERM_MODIFIED)
			{
				// cprintf("\nana fel modified\n");
				uint32 *ptr_page_table = NULL;
				struct FrameInfo *victim_frame_info = get_frame_info(curenv->env_page_directory, victim->virtual_address, &ptr_page_table);
				pf_update_env_page(curenv, victim->virtual_address, victim_frame_info);
			}
			env_page_ws_invalidate(curenv, victim->virtual_address);
			struct WorkingSetElement *ACvictim = LIST_LAST(&(curenv->ActiveList));
			LIST_REMOVE(&(curenv->ActiveList), ACvictim);
			pt_set_page_permissions(curenv->env_page_directory, ACvictim->virtual_address, 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&(curenv->SecondList), ACvictim);

			struct WorkingSetElement *wse = env_page_ws_list_create_element(curenv, fault_va);
			LIST_INSERT_HEAD(&(curenv->ActiveList), wse);

			SWsize++;
			AWsize++;
		}
		else
		{
			// cprintf("Moving page from ActiveList to SecondList\n");
			struct WorkingSetElement *lastAWE = LIST_LAST(&(curenv->ActiveList));
			LIST_REMOVE(&(curenv->ActiveList), lastAWE);
			pt_set_page_permissions(curenv->env_page_directory, lastAWE->virtual_address, 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&(curenv->SecondList), lastAWE);
			LIST_INSERT_HEAD(&(curenv->ActiveList), Iterator);
			//				SWsize++;
			//				AWsize++;
		}
	}
}
//	env_page_ws_print(curenv);
}

void __page_fault_handler_with_buffering(struct Env *curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}
