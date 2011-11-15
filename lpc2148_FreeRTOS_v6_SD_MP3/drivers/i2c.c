#include "i2c.h"
#include <string.h>					// memset, memcpy
#include "../System/lpc214x.h"
#include "../System/sysConfig.h"	// Need PCLK to setup I2C Rate.

#include "../FreeRTOS/FreeRTOS.h"
#include "../FreeRTOS/Semphr.h"

typedef enum { 
	busy,
	readComplete,
	writeComplete
} i2cStateMachineStatus;

typedef enum {
	i2cWrite,
	i2cRead
} I2C_MODE;


#define I2C_FRAME_MAX_DATABYTES		4
typedef struct
{
	char slaveAddress;	// Slave Device Address
	char firstRegister;	// 1st Register to Read or Write
	char error;			// Error if any occured within I2C

	I2C_MODE mode;		// Tracks read vs. write mode

	char rwBuffer[I2C_FRAME_MAX_DATABYTES];	// Buffer of the I2C Read or Write
	int bytesToTransfer;					// # of bytes to transfer.
}I2CIO;
static I2CIO I2C_IOFRAME;

xSemaphoreHandle i2cReadCompleteSignal;
xSemaphoreHandle i2cLock;



i2cStateMachineStatus i2cStateMachine();
void i2cKickOffTransfer(I2C_MODE mode, char devAddr, char regStart, char* bytes, int len);
static void i2cISR() __attribute__ ((naked));




void initialize_I2C0(unsigned int i2cFrequency)
{
	// Connect I2C signals (SDA & SCL) to P0.2 and P0.3
	PINSEL0 &= ~0x000000F0;
	PINSEL0 |= 0x00000050;

	I2C_I2CONCLR = 0x6c;	// Clear all Flags
	if (i2cFrequency > 1000000)
		i2cFrequency = 400000;

	I2C_I2SCLH = (PCLK/(i2cFrequency + 1)) / 2;
	I2C_I2SCLL = (PCLK / i2cFrequency) / 2;

	I2C_I2ADR = 0x00;
	I2C_I2CONSET = 0x40;

	VICIntSelect &= ~(1<<9);	// Use as IRQ, not FIQ
	VICVectCntl4 = (1<<5) | 9;	// Select I2C0 and enable IRQ
	VICVectAddr4 = (unsigned int) &i2cISR;

	// Create I2C Semaphores
	vSemaphoreCreateBinary(i2cLock);
	vSemaphoreCreateBinary(i2cReadCompleteSignal);

	// Take the read complete signal once
	xSemaphoreTake(i2cReadCompleteSignal, 0);

	VICIntEnable = (1<<9);					// I2C0 Interrupt Enable
}

char i2cReadDeviceRegister(char deviceAddress, char registerAddress)
{
	char byte = 0;

	// Wait to get I2C Access
	xSemaphoreTake(i2cLock, portMAX_DELAY);
	i2cKickOffTransfer(i2cRead, deviceAddress, registerAddress, &byte, 1);
	
	// Wait for read to finish, and after it finishes, read the I2C_IOFRAME Buffer.
	xSemaphoreTake(i2cReadCompleteSignal, portMAX_DELAY);
	byte = I2C_IOFRAME.rwBuffer[0];

	// After read is done, give the i2c lock back.
	xSemaphoreGive(i2cLock);

	return byte;
}
char i2cWriteDeviceRegister(char deviceAddress, char registerAddress, char value)
{
	// Wait for I2C Access, and just return, I2C Lock will be given by ISR()
	xSemaphoreTake(i2cLock, portMAX_DELAY);
	i2cKickOffTransfer(i2cWrite, deviceAddress, registerAddress, &value, 1);
	return 0;
}

void i2cKickOffTransfer(I2C_MODE mode, char devAddr, char regStart, char* bytes, int len)
{
	if(len <= I2C_FRAME_MAX_DATABYTES)
	{
		I2C_IOFRAME.error			= 0;
		I2C_IOFRAME.mode			= mode;
		I2C_IOFRAME.slaveAddress	= devAddr;
		I2C_IOFRAME.firstRegister	= regStart;
		I2C_IOFRAME.bytesToTransfer	= len;

		// Copy data to write into I2C Buffer.
		if(i2cWrite == mode) {
			memcpy(I2C_IOFRAME.rwBuffer, bytes, len);
		}
		I2CONSET |= 0x20; // Send START, I2C State Machine will finish the rest.
	}
}

/*
 * I2CONSET
 * 0x04 AA
 * 0x08 SI
 * 0x10 STOP
 * 0x20 START
 * 0x40 ENABLE
 *
 * I2CONCLR
 * 0x04 AA
 * 0x08 SI
 * 0x20 START
 * 0x40 ENABLE
 */
i2cStateMachineStatus i2cStateMachine()
{
	enum I2CStatus{ busError=0, start=0x08, repeatStart=0x10, arbitrationLost=0x38,
			// Master Transmitter States:
			slaveAddressAcked=0x18, slaveAddressNacked=0x20, dataAckedBySlave=0x28, dataNackedBySlave=0x30,
			// Master Receiver States:
			readAckedBySlave=0x40, readModeNackedBySlave=0x48, dataAvailableAckSent=0x50, dataAvailableNackSent=0x58
	};

	i2cStateMachineStatus state = busy;
	static int bPointer = 0;
	/*
	 ***********************************************************************************************************
	 * Write-mode state transition :
	 * start --> slaveAddressAcked --> dataAckedBySlave --> ... (dataAckedBySlave) --> (stop)
	 *
	 * Read-mode state transition :
	 * start --> slaveAddressAcked --> dataAcked --> repeatStart --> readAckedBySlave
	 *  For 2+ bytes:  dataAvailableAckSent --> ... (dataAvailableAckSent) --> dataAvailableNackSent --> (stop)
	 *  For 1  byte :  dataAvailableNackSent --> (stop)
	 ***********************************************************************************************************
	 */

	#define clearSIFlag()		I2CONCLR = (1<<3)
	#define setSTARTFlag()		I2CONSET = (1<<5)
	#define clearSTARTFlag()	I2CONCLR = (1<<5)
	// busInUse is only set to 0 for write operation since read operation should set to 0 itself
	#define setStop()			clearSTARTFlag();						\
								I2CONSET = (1<<4);						\
								clearSIFlag();							\
								while((I2CONSET&(1<<4)));				\
								if(i2cRead == I2C_IOFRAME.mode) 		\
									state = readComplete; 				\
								else									\
									state = writeComplete;

	switch (I2STAT)
	{
		case start:
			bPointer = 0;
			I2DAT = I2C_IOFRAME.slaveAddress;
			clearSIFlag();
			break;
		case repeatStart:
			I2DAT = I2C_IOFRAME.slaveAddress | 0x01;
			clearSIFlag();
			break;

		case slaveAddressAcked:
			clearSTARTFlag();
			if(0 == I2C_IOFRAME.bytesToTransfer) {
				setStop();
			}
			else {
				I2DAT = I2C_IOFRAME.firstRegister;
				clearSIFlag();
			}
			break;
		case slaveAddressNacked:
			I2C_IOFRAME.error = I2STAT;
			setStop();
			break;

		case dataAckedBySlave:
			if (i2cRead == I2C_IOFRAME.mode) {
				setSTARTFlag(); 		// Send Repeat-start for read-mode
				clearSIFlag();
			}
			else {
				if(bPointer >= I2C_IOFRAME.bytesToTransfer) {
					setStop();
				}
				else {
					I2DAT = I2C_IOFRAME.rwBuffer[bPointer++];
					clearSIFlag();
				}
			}
			break;
		case dataNackedBySlave:
			I2C_IOFRAME.error = I2STAT;
			setStop();
			break;


		case readAckedBySlave:
			clearSTARTFlag();
			if(I2C_IOFRAME.bytesToTransfer > 1)
				I2CONSET = 0x04;	// Send ACK to receive a byte and transition to dataAvailableAckSent
			else
				I2CONCLR = 0x04;	// NACK next byte to go to dataAvailableNackSent for 1-byte read.
			clearSIFlag();
			break;
		case readModeNackedBySlave:
			I2C_IOFRAME.error = I2STAT;
			setStop();
			break;
		case dataAvailableAckSent:
			I2C_IOFRAME.rwBuffer[bPointer++] = I2DAT;
			if(bPointer >= (I2C_IOFRAME.bytesToTransfer-1)) {	// Only 1 more byte remaining
				I2CONCLR = 0x04; // NACK next byte --> Next state: dataAvailableNackSent
			}
			else {
				I2CONSET = 0x04; // ACK next byte --> Next state: dataAvailableAckSent(back to this state)
			}
			clearSIFlag();
			break;
		case dataAvailableNackSent:	// Read last-byte from Slave
			I2C_IOFRAME.rwBuffer[bPointer++] = I2DAT;
			setStop();
			break;


		case busError:
			I2C_IOFRAME.error = I2STAT;
			setStop();
			break;
		case arbitrationLost:
			// We should not issue stop() in this condition, but we still need to end our
			// i2c transaction.
			I2C_IOFRAME.error = I2STAT;
			bPointer = 0;
			break;
		default:
			I2C_IOFRAME.error = I2STAT;
			setStop();
	}

	return state;
}

static void i2cISR()
{
	portSAVE_CONTEXT();		// Save context of existing task

	long higherPriorityTaskWaiting = 0;
	i2cStateMachineStatus status = i2cStateMachine();

	/* For READ, do not release I2C Lock, instead give the i2cReadCompleteSignal
	 * so that the read() routine can read the data first, and then release the lock.
	 * For WRITE, release the i2c Lock so next task can access I2C
	 */
	if(status == readComplete)
		xSemaphoreGiveFromISR(i2cReadCompleteSignal, &higherPriorityTaskWaiting);
	else if(status == writeComplete)
		xSemaphoreGiveFromISR(i2cLock, &higherPriorityTaskWaiting);
	
	if(higherPriorityTaskWaiting) {
		portYIELD_FROM_ISR();
	}
	VICVectAddr = 0;

	portRESTORE_CONTEXT();	// Restore context to possibly a task waiting for I2C Read or I2C Mutex
}

