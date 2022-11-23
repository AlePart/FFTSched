/*
 * scheduler.h
 *
 *  Created on: Nov 23, 2022
 *      Author: apart
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

typedef struct{
	void* function;
	struct FUNCTION_NODE *parent;
	struct FUNCTION_NODE *l;
	struct FUNCTION_NODE *r;
}FUNCTION_NODE;

extern void init_task(void (*handler));
extern void init_scheduler();
extern void start_scheduler();

#endif /* SCHEDULER_H_ */
