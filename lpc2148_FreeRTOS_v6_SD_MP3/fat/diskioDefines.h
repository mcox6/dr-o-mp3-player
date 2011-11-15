#ifndef DISIODEFINES_H_
#define DISIODEFINES_H_

#include "../drivers/ssp_spi.h"

#define SPI_SD_CS 			(1 << 10)
#define SELECT()        	IOCLR0 = SPI_SD_CS
#define	DESELECT()      	IOSET0 = SPI_SD_CS
#define SET_CSPIN_OUT() 	PINSEL0 &= ~(3 << 20);	\
							IODIR0 |= SPI_SD_CS;	\
							IOSET0 = SPI_SD_CS

#define SOCKPORT        	IOPIN0                          /* Socket contact port	*/
#define SOCKWP				(1 << 21)
#define SOCKINS				(1 << 16)
#define CARD_SIGNAL_INIT()	PINSEL1 &= ~(3 << 0);		\
							PINSEL1 &= ~(3 << 10);		\
							IODIR0  &= ~SOCKWP;			\
							IODIR0  &= ~SOCKINS;

#define	FCLK_SLOW()			setSSPSPI_Divider(200)		/* Set slow SPI clock (100k-400k) */
#define	FCLK_FAST()			setSSPSPI_Divider(4)		/* Set fast SPI clock (depends on the CSD) */

#define xmit_spi(dat)		rxTxByteSSPSPI(dat)
#define rcvr_spi()			rxTxByteSSPSPI(0xff)


#endif /* DISIODEFINES_H_ */
