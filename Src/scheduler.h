/*
 * scheduler.h
 *
 *  Created on: Nov 23, 2022
 *      Author: apart
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

typedef struct
{
	uint32_t RAM_START;
	uint32_t RAM_SIZE;
	uint32_t TASK_STACK_SIZE;

}tSCHEDULER_INIT;

extern void init_task(void (*handler));
extern void init_scheduler();
extern void start_scheduler();

#endif /* SCHEDULER_H_ */
