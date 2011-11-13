#ifndef OSHANDLES_H
#define OSHANDLES_H

#include "./FreeRTOS/FreeRTOS.h"	// Includes for RTOS & Other services provided by it:
#include "./FreeRTOS/task.h"
#include "./FreeRTOS/queue.h"
#include "./FreeRTOS/semphr.h"

// A pointer to OSHANDLES is all that is needed for any file/task to access
// Semaphores, task handles, queue handles etc...
// Pointer can be passed to task as pvParameter
typedef struct
{
	struct {
		xQueueHandle songname;
		xQueueHandle command;
	}queue;

	struct {
		xTaskHandle userInterface;
		xTaskHandle diskTimer;
		xTaskHandle sender;
		xTaskHandle receiver;
		xTaskHandle buttonHandler;
		xTaskHandle mp3player;
	}task;

	struct {
		xSemaphoreHandle SPI;
		xSemaphoreHandle i2c;
	}lock;
}OSHANDLES;

#endif /* COMMONDEFS_H_ */
