/*
 * scheduler.c
 *
 *  Created on: Nov 23, 2022
 *      Author: apart
 */
#include "stm32f7xx_hal.h"
#include "scheduler.h"
#include <stdio.h>


#define TASK_NUMBER_MAX   (16)




uint32_t __uCurrentTaskIdx = 0;
uint32_t __puTasksPSP[TASK_NUMBER_MAX] = {0};
uint8_t __canProcess = 0;
tSCHEDULER_INIT __schedInit;
uint32_t __main_stack;

// return PSP value stored in slot at __uCurrentTaskIdx index
uint32_t get_current_psp() {
  return __puTasksPSP[__uCurrentTaskIdx];
}

// save PSP value to the slot at __uCurrentTaskIdx index
void save_current_psp(uint32_t psp) {
  __puTasksPSP[__uCurrentTaskIdx] = psp;
}

void init_task(void (*handler)) {
  int i=0;

  // find an empty slot
  for(; i<TASK_NUMBER_MAX; i++) {
    if (__puTasksPSP[i] == 0) break;
  }

  if(i >= TASK_NUMBER_MAX) {
    printf("Can not register a new task anymore!\n");
    return;
  } else {
    printf("Register a task %p at slot %i\n", handler, i);
  }

  // calculate new PSP
  uint32_t* psp = (uint32_t*)(__schedInit.RAM_START + __schedInit.RAM_SIZE - (i+1)*__schedInit.TASK_STACK_SIZE);
  *(--psp) = 0x01000000u; // Dummy xPSR, just enable Thumb State bit;
      *(--psp) = (uint32_t) handler; // PC
      *(--psp) = 0xFFFFFFFDu; // LR with EXC_RETURN to return to Thread using PSP
      *(--psp) = 0x12121212u; // Dummy R12
      *(--psp) = 0x03030303u; // Dummy R3
      *(--psp) = 0x02020202u; // Dummy R2
      *(--psp) = 0x01010101u; // Dummy R1
      *(--psp) = 0x00000000u; // Dummy R0
      *(--psp) = 0x11111111u; // Dummy R11
      *(--psp) = 0x10101010u; // Dummy R10
      *(--psp) = 0x09090909u; // Dummy R9
      *(--psp) = 0x08080808u; // Dummy R8
      *(--psp) = 0x07070707u; // Dummy R7
      *(--psp) = 0x06060606u; // Dummy R6
      *(--psp) = 0x05050505u; // Dummy R5
      *(--psp) = 0x04040404u; // Dummy R4

      // save PSP
      __puTasksPSP[i] = (uint32_t)psp;
}

void idle()
{
	while(1)
	{
		__asm volatile("NOP");
	}
}
void init_scheduler(tSCHEDULER_INIT init_struct)
{
	__schedInit = init_struct;
	init_task(idle);
}
void start_scheduler() {
  printf("Start Scheduler!\n");

  // start with the first task
  __uCurrentTaskIdx = 0;

  // prepare PSP of the first task
  __asm volatile("BL get_current_psp"); // return PSP in R0
  __asm volatile("MSR PSP, R0");  // set PSP

  // change to use PSP
  __asm volatile("MRS R0, CONTROL");
  __asm volatile("ORR R0, R0, #2"); // set bit[1] SPSEL
  __asm volatile("MSR CONTROL, R0");

  // Move to Unprivileged level
  __asm volatile("MRS R0, CONTROL");
  __asm volatile("ORR R0, R0, #1"); // Set bit[0] nPRIV
  __asm volatile("MSR CONTROL, R0");
  // get the handler of the first task by tracing back from PSP which is at R4 slot
  void (*handler)() = (void (*))((uint32_t*)__puTasksPSP[__uCurrentTaskIdx])[14];

  __canProcess = 1;
  // execute the handler
  handler();

}

void select_next_task() {
    /* Round-Robin scheduler */
    __uCurrentTaskIdx++;
    // check if a task is register at current slot
    if (__uCurrentTaskIdx >= TASK_NUMBER_MAX || __puTasksPSP[__uCurrentTaskIdx] == 0) {
        __uCurrentTaskIdx=0;
    }
}

__attribute__ ((naked)) void SysTick_Handler() {
	if(__canProcess == 0)
		return;
  // save LR back to main, must do this firstly
  __asm volatile("PUSH {LR}");

  /* Save the context of current task */

  // get current PSP
  __asm volatile("MRS R0, PSP");
  // save R4 to R11 to PSP Frame Stack
  __asm volatile("STMDB R0!, {R4-R11}"); // R0 is updated after decrement
  // save current value of PSP
  __asm volatile("BL save_current_psp"); // R0 is first argument

  /* Do scheduling */

  // select next task
  __asm volatile("BL select_next_task");

  /* Retrieve the context of next task */

  // get its past PSP value
  __asm volatile("BL get_current_psp"); // return PSP is in R0
  // retrieve R4-R11 from PSP Fram Stack
  __asm volatile("LDMIA R0!, {R4-R11}"); // R0 is updated after increment
  // update PSP
  __asm volatile("MSR PSP, R0");

  // exit
  __asm volatile("POP {LR}");
  __asm volatile("BX LR");
}
