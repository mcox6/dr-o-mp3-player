
#include "./osHandles.h"                // Includes all OS Files
#include "./System/cpu.h"               // Initialize CPU Hardware

#include "./System/crash.h"             // Detect exception (Reset)
#include "./System/watchdog.h"
#include "./drivers/uart/uart0.h"       // Initialize UART
#include "./System/rprintf.h"			// Reduced printf.  STDIO uses lot of FLASH space & Stack space
#include "./general/userInterface.h"	// User interface functions to interact through UART

//#include "./drivers/i2c.h"
#include <stdbool.h>

#include "drivers/pcm1774.h"



//GLOBALS

/* INTERRUPT VECTORS:
 * 0:    OS Timer Tick
 * 1:    Not Used
 * 2:    UART Interrupt
 * 3:    Not Used
 * 4:    I2C
 * 5-16: Not Used
 */

/* Resources Used
 * 1.  TIMER0 FOR OS Interrupt
 * 2.  TIMER1 FOR Run-time Stats (can be disabled at FreeRTOSConfig.h)
 * 3.  Watchdog for timed delay functions (No CPU Reset or timer)
 */
#include "./fat/diskio.h"
#include "./fat/ff.h"
#include "./drivers/sta013.h"
#include "./drivers/ssp_spi.h"

void buttonReceiver(void *p);
void buttonHandler(void *pvParameters);

//Additions: Gets rid of "magic" port expander register numbers
#define SWREG  0x01
#define LEDREG 0x02

int paused = 0; // controls pausing

#define allowVolCntrl 0

#if allowVolCntrls
int outputVol = 50;
pcm1774_OutputVolume(outputVol, outputVol);
pcm1774_DigitalVolume(vol, vo1);
#endif

void diskTimer()
{
	for(;;)
	{
		vTaskDelay(10);
		disk_timerproc();
	}
}

#if 0
void sender(void *p)
{
	OSHANDLES *osHandles = (OSHANDLES*)p;
	char songName[15] = "ESpark.mp3";
	for(;;)
	{
		rprintf("    %s(): Sending song-name to Queue ...\n", __FUNCTION__);
		if(xQueueSend(osHandles->queue.songname, &songName[0], 100)) {
			rprintf("    Song Sent!\n");
		}
		else {
			rprintf("Timeout during Send!!!\n");
		}
		vTaskDelay(OS_MS(5000));
	}
}
#endif

void mp3player(void *pvParameters)
{
	rprintf("mp3player()");
	OSHANDLES *osHandles = (OSHANDLES*)pvParameters;

	char songname[15];
	char command;
	while(1)
	{
		if(xQueueReceive(osHandles->queue.songname, &songname[0], portMAX_DELAY))
		{
			rprintf("SONG RECEIVED: %s\n", songname);
			FIL fileHandle;
			BYTE buffer[2048];

			if(FR_OK == f_open(&fileHandle, songname, FA_OPEN_EXISTING | FA_READ))
			{
				rprintf("File opened, about to play %s\n", songname);
				int numOfReadBytes = sizeof(buffer);
				while(numOfReadBytes == sizeof(buffer))
				{
					if(FR_OK == f_read(&fileHandle, buffer, sizeof(buffer), &numOfReadBytes))
					{
						rprintf("successful read\n");
						if(xSemaphoreTake(osHandles->lock.SPI, 1000))
						{
							rprintf("***** Playing mp3\ *****\n");
							SELECT_MP3_CS();

							int i = 0;
							while( i < numOfReadBytes)
							{
								//if( STA013_NEEDS_DATA() && !paused)
								if( STA013_NEEDS_DATA())
								{

									rxTxByteSSPSPI(buffer[i++]);

								}
								else
								{
									vTaskDelay(1);
								}
							}
							DESELECT_MP3_CS();
							xSemaphoreGive(osHandles->lock.SPI);
						}
						else
						{
							rprintf("Error taking SPI semaphore while playing mp3\n");
						}

					}
				}
				f_close(&fileHandle);
			}
			else
			{
				rprintf("Couldn't open file!\n");

			}
#if 0
			if(xQueueReceive(osHandles->queue.command, &command, portMAX_DELAY))
			{
				f_close(&fileHandle);
			}
#endif
			//	readFile(&myFile, buff, 4096, &bytesRead);
			//	int i = 0;

		}
	}
}

#if 0
void buttonTask()
{
	while(1)
	{
		if(button1IsPressed)
		{
			pcm1774_DigitalVolume(vol, vo1);
		}
		else if(button2)
		{
			vol--;
			pcm1774_DigitalVolume(vol, vol);
		}
		vTaskDelay(200);
	}
}
#endif

int main (void)
{
	OSHANDLES System;            // Should contain all OS Handles

	cpuSetupHardware();          // Setup PLL, enable MAM etc.
	watchdogDelayUs(2000*1000);  // Some startup delay
	uart0Init(38400, 256);       // 256 is size of Rx/Tx FIFO

	// Use polling version of uart0 to do printf/rprintf before starting FreeRTOS
	rprintf_devopen(uart0PutCharPolling);
	if(didSystemCrash())
	{
		rprintf("SYSTEM CRASH!\n");
		printCrashInfo();
		clearCrashInfo();
	}
	cpuPrintMemoryInfo();

	// Open interrupt-driven version of UART0 Rx/Tx
	rprintf_devopen(uart0PutChar);

	System.lock.SPI = xSemaphoreCreateMutex();
	//System.lock.i2c = xSemaphoreCreateMutex();
	System.queue.songname = xQueueCreate(1,15);
	System.queue.command = xQueueCreate(1,1);

	//xTaskCreate( sender,   (signed char*) "sender", 1024, &System, PRIORITY_HIGH, &System.task.sender );
	rprintf("made it here\n");
	// Use the WATERMARK command to determine optimal Stack size of each task (set to high, then slowly decrease)
	// Priorities should be set according to response required
	if (
			// User Interaction set to lowest priority.
			pdPASS != xTaskCreate( uartUI, (signed char*)"Uart UI", STACK_BYTES(1024*6), &System, PRIORITY_LOW,  &System.task.userInterface )
			||
			// diskTimer should always run, and it is a short function, so assign CRITIAL priority.
			pdPASS != xTaskCreate( diskTimer, (signed char*)"Dtimer", STACK_BYTES(256), &System, PRIORITY_CRITICAL,  &System.task.diskTimer )
			||
			pdPASS != xTaskCreate( mp3player, (signed char*)"mp3player", STACK_BYTES(4*1024), &System, PRIORITY_HIGH,  &System.task.mp3player )
			||
			pdPASS != xTaskCreate( buttonHandler, (signed char*)"buttonHandler", STACK_BYTES(1024*2), &System, PRIORITY_LOW,  &System.task.buttonHandler )
			//		||
			//		pdPASS != xTaskCreate( buttonReceiver, (signed char*)"buttonReceiver", STACK_BYTES(1024*2), &System, PRIORITY_LOW,  &System.task.receiver )

	)
	{
		rprintf("ERROR:  OUT OF MEMORY: Check OS Stack Size or task stack size.\n");
	}

	rprintf("\n-- Starting FreeRTOS --\n");
	vTaskStartScheduler();	// This function will not return.

	rprintf_devopen(uart0PutCharPolling);
	rprintf("ERROR: Unexpected OS Exit!\n");
	return 0;
}

// button presses are detected and handled here.
void buttonHandler(void *pvParameters)
{
	OSHANDLES *osHandles = (OSHANDLES*)pvParameters;

	char returnData;
	char *songName;
	int index = 0;
	char send = 0;


	//Additions
	//i2cInit(handles);
	//initPE(handles);
	//initialize_I2C0(400000);
	char testSong[15] = "ESpark.mp3";

#define button8 0x01
#define button7 0x02
#define button6 0x04
#define button5 0x08
#define button4 0x10
#define button3 0x20
#define button2 0x40
#define button1 0x80

	while(1)
	{
		vTaskDelay(200);
		returnData = i2cReadDeviceRegister(0x40, SWREG);
	//	i2cWriteDeviceRegister(0x40, LEDREG, returnData);
		if (returnData != 0x00)
		{
			switch(returnData)
			{
			case button8:
				//VOLUME DOWN
				rprintf("button 8 pressed\n");
#if allowVolCntrls
				if(outputVol>0)
				{
					outputVol--;
					pcm1774_OutputVolume(outputVol, outputVol);
					//pcm1774_DigitalVolume(vol, vo1);

				}
#endif
				break;
			case button7:
				rprintf("button 7 pressed\n");
				//VOLUME UP
#if allowVolCntrls
				if(outputVol<100)
				{
					pcm1774_OutputVolume(outputVol, outputVol);
					//pcm1774_DigitalVolume(vol, vo1);
				}
#endif
				break;


			case button6:	// PLAY

				rprintf("button 6 pressed\n");

				break;

			case button5: // PAUSE
				rprintf("button 5 pressed\n");

				break;
			case button4:
				rprintf("button4\n");
				// something here causes a crash
				/*send = 'S';
						if(xQueueSend(osHandles->queue.command, &send, 100))
						{
							rprintf("Sent Skip!\n");
						}
						else
						{
							rprintf("Timeout during Send!!!\n");
						}
						vTaskDelay(50);
						retGlobals(send, songName);
						if(xQueueSend(osHandles->queue.songname, &songName, 100))
						{
							rprintf("Sent next songname!\n");
						}
						else
						{
							rprintf("Timeout during Send!!!\n");
						}*/
				break;
			case button3:
				rprintf("button3\n");
				//Pause here
				paused = paused ^ 1;
				rprintf("paused = %d\n", paused);
				break;
			case button2:
				rprintf("button 2\n");
				//Play here
				rprintf("    %s(): Sending song-name to Queue ...\n", __FUNCTION__);
				if(xQueueSend(osHandles->queue.songname, &testSong[0], 100)) {
					rprintf("    Song Sent!\n");
				}
				break;
			case button1:
				rprintf("button 1\n");

				send = 'P';

				if(xQueueSend(osHandles->queue.command, &send, 100))
				{
					rprintf("Sent Previous!\n");
				}
				else
				{
					rprintf("Timeout during Send!!!\n");
				}

				vTaskDelay(50);
				retGlobals(send, songName);

				if(xQueueSend(osHandles->queue.songname, &songName, 100))
				{
					rprintf("Sent Previous songname!\n");
				}
				else
				{
					rprintf("Timeout during Send!!!\n");
				}
				break;
			default:
				rprintf("One button at a time, please...\n");
				break;
			}
		}
	}
}

#if 0
void buttonReceiver(void *p)
{
	// Cast the Handle from void pointer to OS_HANDLES pointer
	OSHANDLES *osHandles = (OSHANDLES*)p;


	char operation;
	for(;;)
	{
		vTaskDelay(200);
		if(xQueueReceive(handles->queue.pushButton, &pushButtonValue, portMAX_DELAY))
		{
			printf("%s(): Received push button value as: %i\n", MYNAME, pushButtonValue);
		}
		printf("------------------------------------------------------\n");
	}
}
#endif


