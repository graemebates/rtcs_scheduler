/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
FreeRTOS is a market leading RTOS from Real Time Engineers Ltd. that supports
31 architectures and receives 77500 downloads a year. It is professionally
developed, strictly quality controlled, robust, supported, and free to use in
commercial products without any requirement to expose your proprietary source
code.

This simple FreeRTOS demo does not make use of any IO ports, so will execute on
any Cortex-M3 of Cortex-M4 hardware.  Look for TODO markers in the code for
locations that may require tailoring to, for example, include a manufacturer
specific header file.

This is a starter project, so only a subset of the RTOS features are
demonstrated.  Ample source comments are provided, along with web links to
relevant pages on the http://www.FreeRTOS.org site.

Here is a description of the project's functionality:

The main() Function:
main() creates the tasks and software timers described in this section, before
starting the scheduler.

The FreeRTOS RTOS tick hook (or callback) function:
The tick hook function executes in the context of the FreeRTOS tick interrupt.
The function 'gives' a semaphore every 500th time it executes.  The semaphore
is used to synchronise with the event semaphore task, which is described next.

The event semaphore task:
The event semaphore task uses the FreeRTOS xSemaphoreTake() API function to
wait for the semaphore that is given by the RTOS tick hook function.  The task
increments the ulCountOfReceivedSemaphores variable each time the semaphore is
received.  As the semaphore is given every 500ms (assuming a tick frequency of
1KHz), the value of ulCountOfReceivedSemaphores will increase by 2 each second.

The idle hook (or callback) function:
The idle hook function queries the amount of free FreeRTOS heap space available.
See vApplicationIdleHook().

The malloc failed and stack overflow hook (or callback) functions:
These two hook functions are provided as examples, but do not contain any
functionality.
*/

/* Standard includes. */
#include <stdint.h>

/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"

/* Custom includes */
#include "types.h"

/* Priorities at which the tasks are created.  The event semaphore task is
given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
soon as the semaphore is given. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainEVENT_SEMAPHORE_TASK_PRIORITY	( configMAX_PRIORITIES - 1 )
#define mainMIN_TASK_PRIORITY				( 0 )

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH					( 100 )

/*-----------------------------------------------------------*/

/*
 * TODO: Implement this function for any hardware specific clock configuration
 * that was not already performed before main() was called.
 */
static void prvSetupHardware( void );

/* DD-scheduler functions */
void dd_scheduler(void *pvParameters);
TaskHandle_t dd_tcreate(createTaskParams);
uint32_t dd_delete(TaskHandle_t);
uint32_t dd_return_active_list(taskList**);
uint32_t dd_return_overdue_list(overdueTasks**);

/*-----------------------------------------------------------*/

/* The queue used by the queue send and queue receive tasks. */
static xQueueHandle xQueue = NULL;
static taskList xActiveTasks = {};
static taskList *pActiveTasks = NULL;
static overdueTasks xOverdueTasks = {};

/* The semaphore (in this case binary) that is used by the FreeRTOS tick hook
 * function and the event semaphore task.
 */
static xSemaphoreHandle xEventSemaphore = NULL;

/*-----------------------------------------------------------*/

int main(void)
{
xTimerHandle xExampleSoftwareTimer = NULL;

	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();


	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */
	xQueue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
							sizeof( uint32_t ) );	/* The size of each item the queue holds. */
	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( xQueue, "MainQueue" );


	/* Create the semaphore used by the FreeRTOS tick hook function and the
	event semaphore task. */
	vSemaphoreCreateBinary( xEventSemaphore );
	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( xEventSemaphore, "xEventSemaphore" );

	/* Start the created timer.  A block time of zero is used as the timer
	command queue cannot possibly be full here (this is the first timer to
	be created, and it is not yet running).
	http://www.freertos.org/FreeRTOS-timers-xTimerStart.html */
	xTimerStart( xExampleSoftwareTimer, 0 );

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details.  http://www.freertos.org/a00111.html */
	for( ;; );
}
/*-----------------------------------------------------------*/



void dd_scheduler(void *pvParameters) {

	void insert(taskProps task) {
		// If there are no active tasks queued
		if (pActiveTasks == NULL) {
			pActiveTasks->t_handle		= task.handle;
			pActiveTasks->name			= task.name;
			pActiveTasks->deadline 		= task.deadline;
			pActiveTasks->task_type 	= task.task_type;
			pActiveTasks->creation_time = task.creation_time;
		} else {
		// Insert new task
			taskList *currTask = xActiveTasks;
			// Search for first task with deadline greater than inserted task
			while (currTask->next_cell != NULL) {
				currTask = currTask->next_cell;
				if (task.create_time + task.deadline < currTask->create_time + currTask->deadline) {
					break;
				}
			}

			// Create cell for task
			taskList task_cell = {
				.handle = task.handle,
				.name = task.name,
				.deadline = task.deadline,
				.task_type = task.task_type,
				.creation_time = task.creation_time,
				.next_cell = currTask,
				.prev_cell = currTask->prev_cell
			};

			// If currTask is on first task in list (prev cell doesn't exist)
			if (currTask->prev_cell != NULL) {
				currTask->prev_cell->next_cell = task_cell;
			}
			currTask->next_cell = task_cell
		}

		return;
	}

	void delete(TaskHandle_t handle) {
		if (xActiveTasks.t_handle == NULL) {
			// Something went wrong
		} else {
			while (currTask.t_handle != NULL) {
				if (currTask.t_handle == handle) {

				}
				currTask = currTask.next_cell;
			}
		}
		return;
	}

	queueMsg msg;
	while (1) {
		xQueueReceive( xQueue, &msg, portMAX_DELAY );

		switch(msg.msg_type) {
		case CREATE:
			insert(msg.task_props);
			xQueueSend(msg.cb_queue, "y", portMAX_DELAY);
			break;
		case DELETE:
			delete(msg.task_props.handle);
			xQueueSend(msg.cb_queue, "y", portMAX_DELAY);
			break;
		case ACTIVE:
			listActive();

			break;
		case OVERDUE:
			break;
		}
	}
}

TaskHandle_t dd_tcreate (createTaskParams create_task_params){

	TaskHandle_t xHandle = xTaskCreate(
							create_task_params.func,				/* The function that implements the task. */
							create_task_params.name, 				/* Text name for the task, just to help debugging. */
							configMINIMAL_STACK_SIZE, 		/* The size (in words) of the stack that should be created for the task. */
							NULL, 							/* A parameter that can be passed into the task. */
							mainMIN_TASK_PRIORITY,			/* The priority to assign to the task.  tskIDLE_PRIORITY (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1 is the highest priority. */
							NULL );							/* Used to obtain a handle to the created task.  Not used in this simple demo, so set to NULL. */

	// Create callback queue for backwards communication
	xQueueHandle cb_queue = xQueueCreate(1, sizeof(char));
	// Build message for global queue
	queueMsg msg = {
			.cb_queue = cb_queue,
			.task_props = {
					.handle = xHandle,
					.name = create_task_params.name,
					.deadline = create_task_params.deadline,
					.task_type = create_task_params.task_type,
					.creation_time = xTaskGetTickCount()
			},
			.msg_type = CREATE
	};

	// Put message on global queue
	xQueueSend(xQueue, &msg, 0);

	// Wait on receiver to call callback queue
	xQueueReceive(cb_queue, NULL, portMAX_DELAY);
	// Delete callback queue
	vQueueDelete(cb_queue);

	return xHandle;
}

uint32_t dd_delete(TaskHandle_t t_handle){

	// Create callback queue for backwards communication
	xQueueHandle cb_queue = xQueueCreate(1, sizeof(char));
	// Build message for global queue
	queueMsg msg = {
			.cb_queue = cb_queue,
			.task_props = {
					.handle = t_handle,
					.name = NULL,
					.deadline = NULL,
					.task_type = NULL,
					.creation_time = NULL
			},
			.msg_type = DELETE
	};

	// Put message on global queue
	xQueueSend(xQueue, &msg, 0);

	// Wait on receiver to call callback queue
	xQueueReceive(cb_queue, NULL, portMAX_DELAY);
	// Delete callback queue
	vQueueDelete(cb_queue);

	return 0;
}

uint32_t dd_return_active_list(taskList **list){
	return 0;
}

uint32_t dd_return_overdue_list(overdueTasks **list){
	return 0;
}


void vApplicationTickHook( void )
{
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
static uint32_t ulCount = 0;

	/* The RTOS tick hook function is enabled by setting configUSE_TICK_HOOK to
	1 in FreeRTOSConfig.h.

	"Give" the semaphore on every 500th tick interrupt. */
	ulCount++;
	if( ulCount >= 500UL )
	{
		/* This function is called from an interrupt context (the RTOS tick
		interrupt),	so only ISR safe API functions can be used (those that end
		in "FromISR()".

		xHigherPriorityTaskWoken was initialised to pdFALSE, and will be set to
		pdTRUE by xSemaphoreGiveFromISR() if giving the semaphore unblocked a
		task that has equal or higher priority than the interrupted task.
		http://www.freertos.org/a00124.html */
		xSemaphoreGiveFromISR( xEventSemaphore, &xHigherPriorityTaskWoken );
		ulCount = 0UL;
	}

	/* If xHigherPriorityTaskWoken is pdTRUE then a context switch should
	normally be performed before leaving the interrupt (because during the
	execution of the interrupt a task of equal or higher priority than the
	running task was unblocked).  The syntax required to context switch from
	an interrupt is port dependent, so check the documentation of the port you
	are using.  http://www.freertos.org/a00090.html

	In this case, the function is running in the context of the tick interrupt,
	which will automatically check for the higher priority task to run anyway,
	so no further action is required. */
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software 
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.

	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(queueMsg));

	/* TODO: Setup the clocks, etc. here, if they were not configured before
	main() was called. */
}
