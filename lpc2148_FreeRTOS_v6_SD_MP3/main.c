#include "./osHandles.h"                // Includes all OS Files
#include "./System/cpu.h"               // Initialize CPU Hardware
#include "./System/crash.h"             // Detect exception (Reset)
#include "./System/watchdog.h"
#include "./drivers/uart/uart0.h"       // Initialize UART
#include "./System/rprintf.h"			// Reduced printf.  STDIO uses lot of FLASH space & Stack space
#include "./general/userInterface.h"	// User interface functions to interact through UART
//Need these to read files
#include "./fat/diskio.h"
#include "./fat/ff.h"
#include "./drivers/sta013.h"
#include "./drivers/ssp_spi.h"
#include "./drivers/pcm1774.h"

//Need the functions from here for Buttons/LEDs
#include "./drivers/i2c.h"

//Standard includes
#include "string.h"
#include <stdbool.h>

//GLOBALS
int lastFileIndex;
char songArray[30][15];
char *nowPlaying;
int paused;
char command;
bool SDcardInserted;

//Additions: Gets rid of "magic" port expander register numbers
#define SWREG  0x01
#define LEDREG 0x02

#define button8 0x01
#define button7 0x02
#define button6 0x04
#define button5 0x08
#define button4 0x10
#define button3 0x20
#define button2 0x40
#define button1 0x80

int outputVol;
/*pcm1774_OutputVolume(outputVol, outputVol);
 pcm1774_DigitalVolume(vol, vo1);*/

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
void diskTimer() {
	for (;;) {
		vTaskDelay(10);
		disk_timerproc();
	}
}

char binary2hex(int n) {
	switch (n) {
	case 0:
		return 0x0;
		break;
	case 1:
		return 0x80;
		break;
	case 2:
		return 0xC0;
		break;
	case 3:
		return 0xE0;
		break;
	case 4:
		return 0xF0;
		break;
	case 5:
		return 0xF8;
		break;
	case 6:
		return 0xFC;
		break;
	case 7:
		return 0xFE;
		break;
	case 8:
		return 0xFF;
		break;
	}

}

void initPE(void) {
	vTaskDelay(50);
	i2cWriteDeviceRegister(0x40, 0x06, 0x00);
	i2cWriteDeviceRegister(0x40, 0x07, 0xFF);

	//rprintf("initPE complete!\n");
	i2cWriteDeviceRegister(0x40, LEDREG, 0x00);
}

void SDcardWatcher(void *pv) {

	while (1) {
		SDcardInserted = (IOPIN0 & (1 << 16)) ? 0 : 1;
		if (!SDcardInserted) { // SD card removed during playback.
			//	sta013PauseDecoder();
			//	if(paused==false) needToResume = true;

			i2cWriteDeviceRegister(0x40, LEDREG, 0xAA);
			vTaskDelay(100);
			i2cWriteDeviceRegister(0x40, LEDREG, 0x55);
			vTaskDelay(100);
			//rprintf("Card not inserted!\n");

		} else {
			//rprintf("Card inserted!\n");
			//sta013ResumeDecoder();
			vTaskDelay(1);
		}
	}
	vTaskDelay(200);
}

void mp3(void *pvParameters) {
	OSHANDLES *osHandles = (OSHANDLES*) pvParameters;

	char songname[15];
	paused = 0;
	bool broken = false;

	while (1) {

		if (xQueueReceive(osHandles->queue.songname, &songname[0], portMAX_DELAY)) {
			nowPlaying = songname;
			//rprintf("%c\n", songname[0]);
			//rprintf("%c\n", songname[1]);
			//rprintf("%c\n", songname[2]);
			//rprintf("%c\n", songname[3]);
			//rprintf("%c\n", songname[4]);
			//rprintf("%c\n\n", songname[5]);
			//rprintf("Received: %s\n", songname);
			strcat(songname, ".mp3");
			rprintf("Song to play: %s\n", songname);
			FIL fileHandle;
			BYTE buffer[2048];

			if (FR_OK == f_open(&fileHandle, songname, FA_OPEN_EXISTING
					| FA_READ)) {
				rprintf("File opened, about to play:\n");
				int numOfReadBytes = sizeof(buffer);
				while (numOfReadBytes == sizeof(buffer)) {
					if (FR_OK == f_read(&fileHandle, buffer, sizeof(buffer),
							&numOfReadBytes)) {
						if (xSemaphoreTake(osHandles->lock.SPI, 1000)) {
							//rprintf("SPI Semaphore taken!\n");
							SELECT_MP3_CS();
							int i = 0;
							while (i < numOfReadBytes) {

								if (STA013_NEEDS_DATA()) {

									rxTxByteSSPSPI(buffer[i++]);
								} else {
									vTaskDelay(1);
								}

								if (command != 0) {
									DESELECT_MP3_CS();
									xSemaphoreGive(osHandles->lock.SPI);
									rprintf(
											"Semaphore given back--interrupted with a command.\n");
									f_close(&fileHandle);
									command = 0;
								}
							}
							DESELECT_MP3_CS();
							xSemaphoreGive(osHandles->lock.SPI);
							//rprintf("Semaphore given back.\n");
						} else {
							rprintf(
									"Error taking SPI semaphore while playing mp3\n");
						}
					}
					/*if (broken)
					 {
					 broken = false;
					 break;
					 }*/
				}
				f_close(&fileHandle);
			} else {
				rprintf("Couldn't open file!\n");
			}
		}

	}
}

void popSongs(void *pvParameters) {
	OSHANDLES *osHandles = (OSHANDLES*) pvParameters;
	int current = 0;
	char fullName[15];
	char *file;
	int counter = 0;
	char *fileExtension;
	int i = 0;
	int j = 0;
	char mp3[4] = "mp3";

	// Error check if SPI Lock doesn't exist.
	if (0 == osHandles->lock.SPI) {
		rprintf("You did not create an SPI Mutex\n");
		while (1)
			;
	}

	initialize_SSPSPI();
	initialize_I2C0(100 * 1000);

	diskio_initializeSPIMutex(&(osHandles->lock.SPI));
	initialize_SdCardSignals();
	initialize_sta013();
	initialize_pcm1774();

	FATFS SDCard; // Takes ~550 Bytes
	if (FR_OK != f_mount(0, &SDCard)) { // Mount the Card on the File System
		rprintf("WARNING: SD CARD Could not be mounted!\n");
	}

	DIR Dir;
	FILINFO Finfo;
	FATFS *fs;
	FRESULT returnCode = FR_OK;

	unsigned int fileBytesTotal, numFiles, numDirs;
	fileBytesTotal = numFiles = numDirs = 0;
#if _USE_LFN
	char Lfname[512];
#endif

	char dirPath[] = "0:";
	if (RES_OK != (returnCode = f_opendir(&Dir, dirPath))) {
		rprintf("Invalid directory: |%s|\n", dirPath);
		return 0;
	}

	rprintf("Directory listing of: %s\n\n", dirPath);
	for (j = 0; j < 100; j++) {
#if _USE_LFN
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif

		char returnCode = f_readdir(&Dir, &Finfo);
		if ((FR_OK != returnCode) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR) {
			numDirs++;
		} else {
			numFiles++;
			fileBytesTotal += Finfo.fsize;
		}
		strcpy(fullName, Finfo.fname);
		//rprintf("File Name found: %s.\n", fullName);
		char *temp = fullName;
		file = strtok(temp, ".");
		fileExtension = strtok(NULL, NULL);

		while (file[i] != NULL) {
			//rprintf("Copying File[%d]: %c\n", i, file[i]);

			songArray[counter][i] = file[i];
			i++;
			//rprintf("Received songArray[%d][%d]: %c\n", counter, i, songArray[counter][i]);
		}

		songArray[counter][i + 1] = NULL;
		i = 0;
		//rprintf("Just populated song: %s\n", songArray[counter]);

		lastFileIndex = counter;
		counter++;

		//rprintf("lastFileIndex = %d\n", lastFileIndex);
		//don't overflow the song array!
		if (counter + 1 > 30)
			break;
	}
	for (i = 0; i <= lastFileIndex; i++) {
		rprintf("songArray[%d] = %s\n", i, songArray[i]);
	}
	rprintf("lastFileIndex = %d\n", lastFileIndex);
	xSemaphoreGive(osHandles->lock.SPI);
	//rprintf("Exiting popSongs.\n");

}

int getSong(char c, char *fileName, int current) {
	//rprintf("Received the following character: %c.\n", c);
	vTaskDelay(100);
	if (c == 'S') {
		if (current + 1 > lastFileIndex) {
			current = 0;
		} else {
			current++;
		}
		strcpy(fileName, songArray[current]);
	} else if (c == 'P') {
		if (current - 1 < 0) {
			current = lastFileIndex;
		} else {
			current--;
		}
		strcpy(fileName, songArray[current]);
	} else if (c == 'C') {
		strcpy(fileName, songArray[current]);
	} else if (c != NULL) {
		fileName = "ERROR!";
	}
	vTaskDelay(50);

	//rprintf("Sent Along: songArray[%d] = %s =? %s!\n", currentSong, songArray[currentSong], fileName);

	return current;
}

void debounce(char button) {
	while (i2cReadDeviceRegister(0x40, SWREG) == button) // debounce
	{
		vTaskDelay(1);

	}
}

void buttonHandler(void *pvParameters) {
	OSHANDLES *osHandles = (OSHANDLES*) pvParameters;

	int currentSong;
	char returnData;
	char songName[15];
	int i = 0;

	/*unsigned short bitrate;
	 unsigned char sampFreq;
	 unsigned char mode;*/

	popSongs(osHandles);
	currentSong = 0;

	initPE();
	vTaskDelay(50);
	outputVol = 4;

	command = 0;
	char bass = 4;
	char needToResume = 0;
	while (1) {
		vTaskDelay(100);
		SDcardInserted = (IOPIN0 & (1 << 16)) ? 0 : 1;
		if (!SDcardInserted) { // SD card removed during playback.
			//	sta013PauseDecoder();
			if (paused == false)
				needToResume = true;

			i2cWriteDeviceRegister(0x40, LEDREG, 0xAA);
			vTaskDelay(100);
			i2cWriteDeviceRegister(0x40, LEDREG, 0x55);
			vTaskDelay(100);
			//rprintf("Card not inserted!\n");

		} else {
			//rprintf("Card inserted!\n");
			//sta013ResumeDecoder();
			if (needToResume) {
				f_mount(0, 0);
				popSongs(osHandles);
				needToResume = 0;
			}
			i2cWriteDeviceRegister(0x40, LEDREG, binary2hex(outputVol));
			vTaskDelay(1);
		}

		returnData = i2cReadDeviceRegister(0x40, SWREG);
		//i2cWriteDeviceRegister(0x40, LEDREG, returnData);

		if (returnData != 0x00) {
			switch (returnData) {
			case button1:
				if (outputVol > 0) {
					outputVol--;
					pcm1774_OutputVolume(outputVol * 14, outputVol * 14);
					rprintf("Volume Down\n");
				}
				debounce(button1);
				break;
			case button2:
				if (outputVol < 8) {
					outputVol++;
					pcm1774_OutputVolume(outputVol * 14, outputVol * 14);
					rprintf("Volume Up\n");
				}
				debounce(button2);
				break;
			case button3:
				if (bass > 0)
					bass--;
				pcm1774_bassBoost(bass * 2);
				rprintf("bass = %d\n", bass);
				//void sta013SetTone(char bassEnh, unsigned short bassFreq, char trebleEnh, unsigned short trebleFreq)
				debounce(button3);
				break;
			case button4:
				//sta013GetMP3Info(bitrate, sampFreq, mode);
				//rprintf("Bitrate = %d\nSample Frequency = %d\nMode = %c");
				if (bass < 8)
					bass++;
				pcm1774_bassBoost(bass * 2);
				rprintf("bass = %d\n", bass);
				debounce(button4);
				break;
			case button5:
				if (paused) {
					paused = 0;
					sta013ResumeDecoder();
				}
				command = 'P';

				vTaskDelay(50);
				currentSong = getSong('P', songName, currentSong);
				while (songArray[currentSong][i] != NULL) {
					songName[i] = songArray[currentSong][i];
					i++;
				}
				songName[i] = 0;
				rprintf("Previous song is: %s.\n", songName);
				i = 0;
				if (xQueueSend(osHandles->queue.songname, &songName, 100)) {
					rprintf("Sent Previous song name: %s!\n", songName);
				} else {
					rprintf("Timeout during Send!!!\n");
				}
				debounce(button5);
				break;
			case button6:
				if (paused) {
					paused = 0;
					sta013ResumeDecoder();
				}
				command = 'C';
				vTaskDelay(50);
				currentSong = getSong(command, songName, currentSong);
				command = 0;
				if (xQueueSend(osHandles->queue.songname, &songName, 100)) {
					paused = false;
					rprintf("Sent current song name: %s!\n", songName);
				} else {
					rprintf("Timeout during Send!!!\n");
				}
				debounce(button6);
				break;

			case button7:
				if (paused) {
					paused = 0;
					sta013ResumeDecoder();
				} else if (paused == 0) {
					paused = 1;
					sta013PauseDecoder();
				}
				rprintf("Pause Button\n");
				while (i2cReadDeviceRegister(0x40, SWREG) == button7) // debounce
				{
					vTaskDelay(1);

				}
				break;

			case button8:
				if (paused) {
					paused = 0;
					sta013ResumeDecoder();
				}
				command = 'S';
				vTaskDelay(50);
				currentSong = getSong('S', songName, currentSong);
				while (songArray[currentSong][i] != NULL) {
					songName[i] = songArray[currentSong][i];
					i++;
				}
				songName[i] = 0;
				rprintf("Next song is: %s.\n", songName);
				i = 0;
				if (xQueueSend(osHandles->queue.songname, &songName, 100)) {
					rprintf("Sent Next song name: %s!\n", songName);
				} else {
					rprintf("Timeout during Send!!!\n");
				}
				debounce(button8);
				break;
			default:
				rprintf("One button at a time, please...\n");
				break;
			}
		}
	}
}

int main(void) {
	OSHANDLES System; // Should contain all OS Handles

	cpuSetupHardware(); // Setup PLL, enable MAM etc.
	watchdogDelayUs(2000 * 1000); // Some startup delay
	uart0Init(38400, 256); // 256 is size of Rx/Tx FIFO

	// Use polling version of uart0 to do printf/rprintf before starting FreeRTOS
	rprintf_devopen(uart0PutCharPolling);
	if (didSystemCrash()) {
		printCrashInfo();
		clearCrashInfo();
	}
	cpuPrintMemoryInfo();

	// Open interrupt-driven version of UART0 Rx/Tx
	rprintf_devopen(uart0PutChar);

	System.lock.SPI = xSemaphoreCreateMutex();
	System.queue.songname = xQueueCreate(1, 15);
	System.queue.command = xQueueCreate(1, 1);

	//SET THE INITIAL SONG?
	//currentSong = 0;

	// Use the WATERMARK command to determine optimal Stack size of each task (set to high, then slowly decrease)
	// Priorities should be set according to response required
	if (
	// User Interaction set to lowest priority.
	//pdPASS != xTaskCreate( uartUI, (signed char*)"Uart UI", STACK_BYTES(1024*6), &System, PRIORITY_LOW,  &System.task.userInterface )
	//||
	// diskTimer should always run, and it is a short function, so assign CRITIAL priority.
	pdPASS
			!= xTaskCreate( buttonHandler, (signed char*)"buttonHandler", STACK_BYTES(1024*4), &System, PRIORITY_LOW, &System.task.buttonHandler )
			|| pdPASS
					!= xTaskCreate( diskTimer, (signed char*)"Dtimer", STACK_BYTES(256), &System, PRIORITY_CRITICAL, &System.task.diskTimer )
			|| pdPASS
					!= xTaskCreate( mp3, (signed char*)"mp3", STACK_BYTES(1024*4), &System, PRIORITY_HIGH, &System.task.mp3 )

	) {
		rprintf(
				"ERROR:  OUT OF MEMORY: Check OS Stack Size or task stack size.\n");
	}

	rprintf("\n-- Starting FreeRTOS --\n");
	vTaskStartScheduler(); // This function will not return.

	rprintf_devopen(uart0PutCharPolling);
	rprintf("ERROR: Unexpected OS Exit!\n");
	return 0;
}

