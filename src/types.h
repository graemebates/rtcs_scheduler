/*
 * types.h
 *
 *  Created on: Mar 26, 2019
 *      Author: gbates
 */

#ifndef TYPES_H_
#define TYPES_H_

typedef enum {
	CREATE,
	DELETE,
	ACTIVE,
	OVERDUE
} msgType;

typedef enum {
	PERIODIC,
	APERIODIC
} taskType;

typedef struct {
	uint32_t deadline;
	uint32_t execution_time;
	taskType task_type;
	void (*func)();
	char *name;
} createTaskParams;

typedef struct {
	TaskHandle_t handle;
	char *name;
	uint32_t deadline;
	uint32_t execution_time;
	uint32_t task_type;
	uint32_t creation_time;
} taskProps;

typedef struct {
	xQueueHandle cb_queue;
	taskProps task_props;
	msgType msg_type;
} queueMsg;


typedef struct {
	TaskHandle_t t_handle;
	uint32_t deadline;
	uint32_t task_type;
	uint32_t creation_time;
	struct taskList *next_cell;
	struct taskList *previous_cell;
} taskList;

typedef struct {
	TaskHandle_t t_handle;
	uint32_t deadline;
	uint32_t task_type;
	uint32_t creation_time;
	struct overdueTasks *next_cell;
	struct overdueTasks *previous_cell;
} overdueTasks;



#endif /* TYPES_H_ */
