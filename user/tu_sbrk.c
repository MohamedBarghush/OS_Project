
#include <inc/lib.h>

void _main(void)
{
	uint32 kilo = 1024;
	uint32 returned_break;
	int freeFrames;
	int usedDiskPages;
	int WsSize;
	uint32 expectedVAs[5] =
	{USER_HEAP_START, \
			USER_HEAP_START + PAGE_SIZE, \
			(USER_HEAP_START + (2*PAGE_SIZE) - 2*kilo), \
			((USER_HEAP_START + PAGE_SIZE) - kilo), \
			(USER_HEAP_START + 3*PAGE_SIZE), \
	};
	uint32 expectedWSPagesDeleted = {USER_HEAP_START};
	int old_eval = 0;
	int eval = 0;
	int found;

	uint32* int_arr2;
	bool is_correct = 1;

	cprintf("STEP A: checking increment with +ve value [increment from aligned segment break] ... \n");
	{
		freeFrames = (int)sys_calculate_free_frames();
		usedDiskPages = sys_pf_calculate_allocated_pages();
		WsSize = LIST_SIZE(&(myEnv->page_WS_list));
		uint32* int_arr = malloc(512 * sizeof(uint32));
		int_arr[0] = 500;
		if (((uint32)int_arr - sizeOfMetaData())!=expectedVAs[0])
		{ is_correct = 0; cprintf("Wrong returned break: Expected: %x, Actual: %x\n", expectedVAs[0], ((uint32)int_arr - sizeOfMetaData()));}

		if ((freeFrames - (int)sys_calculate_free_frames())!=2) /* Two frames were allocated for page table creation + first page for block allocator*/
		{ is_correct = 0; cprintf("Wrong free frames count: allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", 1, (freeFrames - (int)sys_calculate_free_frames()));}

		if (sys_pf_calculate_allocated_pages()!=usedDiskPages)
		{ is_correct = 0; cprintf("Wrong free page file pages count: allocated page file size incorrect. Expected Difference = %d, Actual = %d\n", 0, (usedDiskPages - sys_pf_calculate_allocated_pages()));}

		if ((LIST_SIZE(&(myEnv->page_WS_list)) - WsSize)!=1) /* Page fault were occurred when we try to initialize the first block in the block allocator */
		{ is_correct = 0; cprintf("Wrong size for WS size: new page(s) was/were added to the working set while it/they should NOT be written");}

		if (is_correct)
			cprintf("STEP A PASSED: checking increment with +ve value [increment from aligned segment break] works!\n\n\n");
		else
			cprintf("STEP A finished with problems\n");
		old_eval = eval;
	}
	if (is_correct)
	{
		eval += 20;
	}
	is_correct = 1;

	/*******************************************************************/
	/*******************************************************************/

	cprintf("STEP B: checking increment with zero value ... \n");
	{
		freeFrames = (int)sys_calculate_free_frames();
		usedDiskPages = sys_pf_calculate_allocated_pages();
		WsSize = LIST_SIZE(&(myEnv->page_WS_list));
		returned_break = (uint32)sys_sbrk(0);
		if (returned_break!=expectedVAs[1])
		{ is_correct = 0; cprintf("Wrong returned break: Expected: %x, Actual: %x\n", expectedVAs[1], returned_break);}

		if (is_correct)
			cprintf("STEP B PASSED: checking increment with zero value works!\n\n\n");
		else
			cprintf("STEP B finished with problems\n");
		old_eval = eval;
	}
	if (is_correct)
	{
		eval += 5;
	}
	is_correct = 1;

	/*******************************************************************/
	/*******************************************************************/

	cprintf("STEP C: checking increment with -ve value [No de-allocation case] ... \n");
	{
		int_arr2 = malloc(512 * sizeof(uint32));
		// Accessing to arr to make page fault and handle it
		int_arr2[32] = 300;
		freeFrames = (int)sys_calculate_free_frames();
		usedDiskPages = sys_pf_calculate_allocated_pages();
		WsSize = LIST_SIZE(&(myEnv->page_WS_list));
		sys_sbrk(-2*kilo);
		returned_break = (uint32)sys_sbrk(0);
		if (returned_break!=expectedVAs[2])
		{ is_correct = 0; cprintf("Wrong returned break: Expected: %x, Actual: %x\n", expectedVAs[2], returned_break);}

		if ((int)sys_calculate_free_frames()!=freeFrames)
		{ is_correct = 0; cprintf("Wrong free frames count: allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", 0, (freeFrames - (int)sys_calculate_free_frames()));}

		if (sys_pf_calculate_allocated_pages()!=usedDiskPages)
		{ is_correct = 0; cprintf("Wrong free page file pages count: allocated page file size incorrect. Expected Difference = %d, Actual = %d\n", 0, (usedDiskPages - sys_pf_calculate_allocated_pages()));}

		if (LIST_SIZE(&(myEnv->page_WS_list))!=WsSize)
		{ is_correct = 0; cprintf("Wrong size for WS size: new page(s) was/were added to the working set while it/they should NOT be written");}

		uint32 MentToFound [2] = {USER_HEAP_START, USER_HEAP_START + PAGE_SIZE};
		found = sys_check_WS_list(MentToFound, 2, 0, 2);
		if (!found)
		{ is_correct = 0; cprintf("Unexpected page were removed from working set\n");}

		if (is_correct)
			cprintf("STEP C PASSED: checking increment with -ve value [No de-allocation case] works!\n\n\n");
		else
			cprintf("STEP C finished with problems\n");
		old_eval = eval;
	}
	if (is_correct)
	{
		eval += 25;
	}
	is_correct = 1;

	/*******************************************************************/
	/*******************************************************************/

	cprintf("STEP D: checking increment with -ve value [De-allocation case] ... \n");
	{
		freeFrames = (int)sys_calculate_free_frames();
		free(int_arr2) ; //free the allocated 2KB first
		if (((int)sys_calculate_free_frames()-freeFrames)!=0) /* Free frames should not be affected since the free() doesn't remove from memory */
		{ is_correct = 0; cprintf("Wrong free frames count: allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", 1, ((int)sys_calculate_free_frames() - freeFrames));}

		freeFrames = (int)sys_calculate_free_frames();
		usedDiskPages = sys_pf_calculate_allocated_pages();
		WsSize = LIST_SIZE(&(myEnv->page_WS_list));
		sys_sbrk(-3*kilo);
		returned_break = (uint32)sys_sbrk(0);
		if (returned_break!=expectedVAs[3])
		{ is_correct = 0; cprintf("Wrong returned break: Expected: %x, Actual: %x\n", expectedVAs[3], returned_break);}

		if (((int)sys_calculate_free_frames()-freeFrames)!=1) /* Free frames increased by one due to crossing the page boundary */
		{ is_correct = 0; cprintf("Wrong free frames count: allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", 1, ((int)sys_calculate_free_frames() - freeFrames));}

		if (sys_pf_calculate_allocated_pages()!=usedDiskPages)
		{ is_correct = 0; cprintf("Wrong free page file pages count: allocated page file size incorrect. Expected Difference = %d, Actual = %d\n", 0, (usedDiskPages - sys_pf_calculate_allocated_pages()));}

		if (WsSize - LIST_SIZE(&(myEnv->page_WS_list))!=1)
		{ is_correct = 0; cprintf("Wrong size for WS size: removed page(s) was/were not removed from the working set");}

		uint32 MentToFound2 [1] = {USER_HEAP_START + PAGE_SIZE};
		found = sys_check_WS_list(MentToFound2, 1, 0, 2);
		if (found)
		{ is_correct = 0; cprintf("Unexpected page should be removed from the working set but it's NOT\n");}

		if (is_correct)
			cprintf("STEP D PASSED: checking increment with -ve value [De-allocation case] works!\n\n\n");
		else
			cprintf("STEP D finished with problems\n");
		old_eval = eval;
	}
	if (is_correct)
	{
		eval += 25;
	}
	is_correct = 1;

	/*******************************************************************/
	/*******************************************************************/
	cprintf("STEP E: checking increment with +ve value ... \n");
	{
		freeFrames = (int)sys_calculate_free_frames();
		usedDiskPages = sys_pf_calculate_allocated_pages();
		WsSize = LIST_SIZE(&(myEnv->page_WS_list));
		sys_sbrk(512);
		returned_break = (uint32)sys_sbrk(0);
		if (returned_break!=expectedVAs[1])
		{ is_correct = 0; cprintf("Wrong returned break: Expected: %x, Actual: %x\n", expectedVAs[1], returned_break);}

		if (((int)sys_calculate_free_frames()-freeFrames)!=0)
		{ is_correct = 0; cprintf("Wrong free frames count: allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", 0, ((int)sys_calculate_free_frames() - freeFrames));}

		if (sys_pf_calculate_allocated_pages()!=usedDiskPages)
		{ is_correct = 0; cprintf("Wrong free page file pages count: allocated page file size incorrect. Expected Difference = %d, Actual = %d\n", 0, (usedDiskPages - sys_pf_calculate_allocated_pages()));}

		if ((WsSize - LIST_SIZE(&(myEnv->page_WS_list)))!=0)
		{ is_correct = 0; cprintf("Wrong size for WS size: new page(s) was/were added to the working set while it/they should NOT be written");}

		uint32 MentToFound2 [1] = {USER_HEAP_START + PAGE_SIZE};

		found = sys_check_WS_list(MentToFound2, 1, 0, 2);
		if (found)
		{ is_correct = 0; cprintf("Unexpected page shouldn't be written into the working set but it has been written\n");}

		if (is_correct)
			cprintf("STEP E PASSED: checking increment with +ve value works!\n\n\n");
		else
			cprintf("STEP E finished with problems\n");
	}
	if (is_correct)
	{
		eval += 25;
	}
	is_correct = 1;

	/*******************************************************************/
	/*******************************************************************/

	//cprintf("test user heap sbrk is finished. Evaluation = %d%\n", eval);
	cprintf("[AUTO_GR@DING_PARTIAL]%d\n", eval);

	return;
}
