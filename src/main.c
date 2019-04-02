
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
/* Kernel includes. */
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
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
taskNames *dd_return_active_list();
taskNames *dd_return_overdue_list();
void purgeAndRun(void);

/*-----------------------------------------------------------*/

/* The queue used by the queue send and queue receive tasks. */
static xQueueHandle xQueue = NULL;
static taskList *pActiveTasks = NULL;
static overdueTasks *pOverdueTasks = NULL;
static xTimerHandle xExpirationTimer = NULL;
/* The semaphore (in this case binary) that is used by the FreeRTOS tick hook
 * function and the event semaphore task.
 */
static xSemaphoreHandle xEventSemaphore = NULL;

/*-----------------------------------------------------------*/

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the portTICK_RATE_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS		( 1000 / portTICK_RATE_MS )


#define amber  	0
#define green  	1
#define red  	2
#define blue  	3

#define amber_led	LED3
#define green_led	LED4
#define red_led		LED5
#define blue_led	LED6

#define TASK1_EXEC 195
#define TASK1_PERIOD 1500
#define TASK2_EXEC 150
#define TASK2_PERIOD 1500
#define TASK3_EXEC 250
#define TASK3_PERIOD 1750

//void green_light(){
//	STM_EVAL_LEDOn(green_led);
//	uint32_t start_time = xTaskGetTickCount();
//	while (xTaskGetTickCount() < start_time+10000){}
//	STM_EVAL_LEDOff(green_led);
//
//	TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
//	dd_delete(currentTaskHandle);
//}
//
//void red_light(){
//	STM_EVAL_LEDOn(red_led);
//	uint32_t start_time = xTaskGetTickCount();
//	while (xTaskGetTickCount() < start_time+1000){}
//	STM_EVAL_LEDOff(red_led);
//
//	TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
//	dd_delete(currentTaskHandle);
//}

void task1() {
	STM_EVAL_LEDOn(red_led);

	uint32_t start_time = xTaskGetTickCount();
	while ( xTaskGetTickCount() < start_time+TASK1_EXEC ) {}

	STM_EVAL_LEDOff(red_led);

	TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
	dd_delete(currentTaskHandle);
}

void task2() {
	STM_EVAL_LEDOn(green_led);

	uint32_t start_time = xTaskGetTickCount();
	while ( xTaskGetTickCount() < start_time+TASK2_EXEC ) {}

	STM_EVAL_LEDOff(green_led);

	TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
	dd_delete(currentTaskHandle);
}

void task3() {
	STM_EVAL_LEDOn(blue_led);

	uint32_t start_time = xTaskGetTickCount();
	while ( xTaskGetTickCount() < start_time+TASK3_EXEC ) {}

	STM_EVAL_LEDOff(blue_led);

	TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
	dd_delete(currentTaskHandle);
}

void task1timer() {
	createTaskParams taskParams = {
		.name = "task1",
		.deadline = TASK1_PERIOD,
		.task_type = PERIODIC,
		.func = &task1
	};
	dd_tcreate(taskParams);
}

void task2timer() {
	createTaskParams taskParams = {
		.name = "task2",
		.deadline = TASK2_PERIOD,
		.task_type = PERIODIC,
		.func = &task2
	};
	dd_tcreate(taskParams);
}

void task3timer() {
	createTaskParams taskParams = {
		.name = "task3",
		.deadline = TASK3_PERIOD,
		.task_type = PERIODIC,
		.func = &task3
	};
	dd_tcreate(taskParams);
}

void periodicGenerator() {
	xTimerHandle xPeriodicGenTimer1 = NULL;
	xTimerHandle xPeriodicGenTimer2 = NULL;
	xTimerHandle xPeriodicGenTimer3 = NULL;

	/* Test Bench #1 */
	xPeriodicGenTimer1 = xTimerCreate("Task 1", TASK1_PERIOD, pdTRUE, ( void * ) 0, task1timer);
	xPeriodicGenTimer2 = xTimerCreate("Task 2", TASK2_PERIOD, pdTRUE, ( void * ) 0, task2timer);
	xPeriodicGenTimer3 = xTimerCreate("Task 3", TASK3_PERIOD, pdTRUE, ( void * ) 0, task3timer);

	task1timer();
	task2timer();
	task3timer();

	xTimerStart(xPeriodicGenTimer1, 0);
	xTimerStart(xPeriodicGenTimer2, 0);
	xTimerStart(xPeriodicGenTimer3, 0);

	vTaskDelete( NULL );
}

//void gen(){
//	createTaskParams taskParams = {
//				.name = "greenLight",
//				.deadline = 1000,
//				.task_type = APERIODIC,
//				.func = &green_light
//		};
//
//	dd_tcreate(taskParams);
//	dd_return_active_list();
//
////	createTaskParams taskParams2 = {
////				.name = "redLight",
////				.deadline = 20,
////				.task_type = APERIODIC,
////				.func = &red_light
////		};
////	dd_tcreate(taskParams2);
//	taskNames *task_names = dd_return_overdue_list();
//
//	vTaskDelete( NULL );
//}

int main(void)
{
	// Init leds
	STM_EVAL_LEDInit(amber_led);
	STM_EVAL_LEDInit(green_led);
	STM_EVAL_LEDInit(red_led);
	STM_EVAL_LEDInit(blue_led);
	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();

	xExpirationTimer = xTimerCreate("Expiration Timer", /* A text name, purely to help debugging. */
		8192,											/* The timer period, in this case 1000ms (1s). */
		pdFALSE,										/* This is not a periodic timer, so xAutoReload is set to pdTRUE. */
		( void * ) 0,									/* The ID is not used, so can be set to anything. */
		purgeAndRun										/* The callback function that changes the running task. */
	);

	xTimerStart( xExpirationTimer, 0 );


	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */
	xQueue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
							sizeof( queueMsg ) );	/* The size of each item the queue holds. */
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
//	xTimerStart( xExampleSoftwareTimer, 0 );
	BaseType_t xReturned = xTaskCreate(
							dd_scheduler,				/* The function that implements the task. */
							"dd_scheduler", 				/* Text name for the task, just to help debugging. */
							configMINIMAL_STACK_SIZE, 		/* The size (in words) of the stack that should be created for the task. */
							NULL, 							/* A parameter that can be passed into the task. */
							4,			/* The priority to assign to the task.  tskIDLE_PRIORITY (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1 is the highest priority. */
							NULL );							/* Used to obtain a handle to the created task.  Not used in this simple demo, so set to NULL. */


	/* Start the tasks and timer running. */

//	xReturned = xTaskCreate(
//							gen,				/* The function that implements the task. */
//							"gen", 				/* Text name for the task, just to help debugging. */
//							configMINIMAL_STACK_SIZE, 		/* The size (in words) of the stack that should be created for the task. */
//							NULL, 							/* A parameter that can be passed into the task. */
//							3,			/* The priority to assign to the task.  tskIDLE_PRIORITY (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1 is the highest priority. */
//							NULL );							/* Used to obtain a handle to the created task.  Not used in this simple demo, so set to NULL. */

	xReturned = xTaskCreate(
							periodicGenerator,				/* The function that implements the task. */
							"periodicGenerator", 				/* Text name for the task, just to help debugging. */
							configMINIMAL_STACK_SIZE, 		/* The size (in words) of the stack that should be created for the task. */
							NULL, 							/* A parameter that can be passed into the task. */
							3,			/* The priority to assign to the task.  tskIDLE_PRIORITY (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1 is the highest priority. */
							NULL );							/* Used to obtain a handle to the created task.  Not used in this simple demo, so set to NULL. */


	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details.  http://www.freertos.org/a00111.html */
	for( ;; );
}
/*-----------------------------------------------------------*/


void insert(taskProps task) {
	// Create cell for task
	taskList task_cell = {
		.handle = task.handle,
		.name = task.name,
		.deadline = task.deadline,
		.task_type = task.task_type,
		.creation_time = task.creation_time,
		.next_cell = NULL,
		.previous_cell = NULL
	};

	// Create space in memory for task
	taskList *pTask = (taskList*)pvPortMalloc(sizeof(taskList));
	*pTask = task_cell;

	// If there are no active tasks queued
	if (pActiveTasks == NULL) {
		pActiveTasks = pTask;
	} else {
	// Insert new task
		taskList* currTask = pActiveTasks;
		// Search for first task with deadline greater than inserted task
		eTaskState taskState;
		while (currTask->next_cell != NULL) {
			taskState = eTaskGetState(currTask->handle);
			// If task has been deleted, ignore
			if (taskState == eDeleted) {

			// If task should be inserted before current task
			} else if (task.creation_time + task.deadline < currTask->creation_time + currTask->deadline) {
				break;
			}

			// Otherwise continue iteration
			currTask = currTask->next_cell;
		}

		// If inserting task should go after currTask (when only one task is in list)
		if (task.creation_time + task.deadline >= currTask->creation_time + currTask->deadline) {
			pTask->next_cell = currTask->next_cell;
			pTask->previous_cell = currTask;
			currTask->next_cell = pTask;
		// Otherwise task should go in front
		} else {

			// If currTask is on first task in list (prev cell doesn't exist)
			if (currTask->previous_cell != NULL) {
				pTask->previous_cell = currTask->previous_cell;
				currTask->previous_cell->next_cell = pTask;
			}

			pTask->next_cell = currTask;
			currTask->previous_cell = pTask;
			pActiveTasks = pTask;
		}
	}

	return;
}

void delete(TaskHandle_t handle) {
	if (pActiveTasks == NULL) {
		// Something went wrong
	} else {
		taskList* currTask = pActiveTasks;
		// Search for task using handle
		while (currTask != NULL) {
			if (currTask->handle == handle) {
				// Delete the task
				vTaskDelete(handle);

				// If it is the first/only task
				if (currTask->previous_cell == NULL) {
					// Move active tasks pointer to next one
					pActiveTasks = currTask->next_cell;
				// If there is a previous task
				} else {
					currTask->previous_cell->next_cell = currTask->next_cell;
				}

				// If there is a next task
				if (currTask->next_cell != NULL) {
					currTask->next_cell->previous_cell = currTask->previous_cell;
				}

				break;
			}

			currTask = currTask->next_cell;
		}
	}
	return;
}

void purgeAndRun() {
	taskList* currTask = pActiveTasks;
	TickType_t time;

	eTaskState taskState = 0;


	while (currTask != NULL) {
		// If it has run
		taskState = eTaskGetState(currTask->handle);
		if (taskState == eDeleted) {
			vPortFree(currTask);

		// If it is overdue
		time = xTaskGetTickCount();
		} else if (currTask->creation_time + currTask->deadline <= xTaskGetTickCount()) {
			overdueTasks *overdueTask = pvPortMalloc(sizeof(overdueTasks));
			overdueTasks odtask = {
					.handle = currTask->handle,
					.name = currTask->name,
					.deadline = currTask->deadline,
					.task_type = currTask->task_type,
					.creation_time = currTask->creation_time,
					.next_cell = pOverdueTasks,
					.previous_cell = NULL
			};
			*overdueTask = odtask;
			if (pOverdueTasks != NULL) {
				pOverdueTasks->previous_cell = overdueTask;
			};
			pOverdueTasks = overdueTask;

			vTaskDelete(currTask->handle);
			vPortFree(currTask);
		} else {
			// Not overdue, not deleted. This is a valid task
			currTask->previous_cell = NULL;
			pActiveTasks = currTask;
			break;
		}
		currTask = currTask->next_cell;
	}
	taskList* countTask = pActiveTasks;
	int num_tasks = 0;
	while (countTask != NULL){
		countTask = countTask->next_cell;
		num_tasks += 1;
	}

	for (int i = 0; i < num_tasks; i++) {
		vTaskPrioritySet(currTask->handle, num_tasks-i+1);
		currTask = currTask->next_cell;
	}

	if (pActiveTasks != NULL) {
		xTimerChangePeriod(
				xExpirationTimer,
				pActiveTasks->deadline + pActiveTasks->creation_time - xTaskGetTickCount(),
				0);
	}
}

void dd_scheduler(void *pvParameters) {
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
			//listActive();

			break;
		case OVERDUE:
			break;
		}
		purgeAndRun();
	}
}


TaskHandle_t dd_tcreate (createTaskParams create_task_params){
	TaskHandle_t xHandle = NULL;
	BaseType_t xReturned = xTaskCreate(
							create_task_params.func,				/* The function that implements the task. */
							create_task_params.name, 				/* Text name for the task, just to help debugging. */
							configMINIMAL_STACK_SIZE, 		/* The size (in words) of the stack that should be created for the task. */
							NULL, 							/* A parameter that can be passed into the task. */
							mainMIN_TASK_PRIORITY,			/* The priority to assign to the task.  tskIDLE_PRIORITY (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1 is the highest priority. */
							&xHandle );							/* Used to obtain a handle to the created task.  Not used in this simple demo, so set to NULL. */

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
	xQueueSend(xQueue, &msg, portMAX_DELAY);

	// Wait on receiver to call callback queue
	char res = 0;
	xQueueReceive(cb_queue, &res, portMAX_DELAY);

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
	char res = 0;
	xQueueReceive(cb_queue, &res, portMAX_DELAY);
	// Delete callback queue
	vQueueDelete(cb_queue);

	// Delete task
	vTaskDelete(t_handle);

	return 0;
}

taskNames *dd_return_active_list(){
	if (pActiveTasks == NULL) {
		return NULL;
	}

	taskNames *head_task_names;
	taskNames *prev_task_name;
	taskList *curr_task = pActiveTasks;

	taskNames temp_task_name = {
		.name = curr_task->name,
		.next_cell = NULL
	};

	taskNames *task_name = (taskNames*)pvPortMalloc(sizeof(taskNames));
	*task_name = temp_task_name;
	head_task_names = task_name;
	prev_task_name = task_name;
	curr_task = curr_task->next_cell;

	while (curr_task != NULL) {
		taskNames temp_task_name = {
			.name = curr_task->name,
			.next_cell = NULL
		};

		taskNames *task_name = (taskNames*)pvPortMalloc(sizeof(taskNames));
		*task_name = temp_task_name;

		prev_task_name->next_cell = task_name;
		prev_task_name = task_name;
		curr_task = curr_task->next_cell;
	}

	return head_task_names;
}

taskNames *dd_return_overdue_list(){
	if (pOverdueTasks == NULL) {
		return NULL;
	}

	taskNames *head_task_names;
	taskNames *prev_task_name;
	taskList *curr_task = pOverdueTasks;

	taskNames temp_task_name = {
		.name = curr_task->name,
		.next_cell = NULL
	};

	taskNames *task_name = (taskNames*)pvPortMalloc(sizeof(taskNames));
	*task_name = temp_task_name;
	head_task_names = task_name;
	prev_task_name = task_name;
	curr_task = curr_task->next_cell;

	while (curr_task != NULL) {
		taskNames temp_task_name = {
			.name = curr_task->name,
			.next_cell = NULL
		};

		taskNames *task_name = (taskNames*)pvPortMalloc(sizeof(taskNames));
		*task_name = temp_task_name;

		prev_task_name->next_cell = task_name;
		prev_task_name = task_name;
		curr_task = curr_task->next_cell;
	}

	pOverdueTasks = NULL;

	return head_task_names;
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
//	xFreeStackSpace = xPortGetFreeHeapSize();
//
//	if( xFreeStackSpace > 100 )
//	{
//		/* By now, the kernel has allocated everything it is going to, so
//		if there is a lot of heap remaining unallocated then
//		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
//		reduced accordingly. */
//	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	/* TODO: Setup the clocks, etc. here, if they were not configured before
	main() was called. */
}
