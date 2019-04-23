/* 
 * File:   PN532Interface.h
 * Author: Brad McGarvey
 *
 * Created on February 25, 2015, 8:33 AM
 */

#ifndef PN532INTERFACE_H
#define	PN532INTERFACE_H

//Uncomment one interface
#define HSU_INTERFACE
//#define I2C_INTERFACE
//#define SPI_INTERFACE

#define _XTAL_FREQ      40000000L
#define PN532_ACK_TIMEOUT       150 // 150 * 100uS = 15ms


#if defined I2C_INTERFACE
//uController pin assignments for I2C
/*
 Pin assignmnets for I2C
 *  MSSP2
 * RD4 = P70_IRQ
 * RD5 = SDA
 * RD6 = SCL
 * RD7 = RSTPD_N
 *
 *  MSSP1
 * RC4 = SDA
 * RC3 = SCL
 */
#define PN532_RESET       LATDbits.LATD7
#define PN532_IRQ         PORTDbits.RD4

//Set tris bits for the above pins only - I2C pins are handled separately
#define PN532_TRIS()      TRISDbits.TRISD7 = 0;TRISDbits.TRISD4 = 1

#define MSSPx           2   //1 or 2 to select MSSP1 or MSSP2 for I2C
#define I2C_BAUD        400000L //set to 100000L or 400000L

#elif defined HSU_INTERFACE
//uController pin assignments for HSU
/*
 Pin assignmnets for HSU
 *  USART 1
 * RC7 = PN532 TX
 * RC6 = PN532 RX
 * RD7 = RSTPD_N
 *
 *  USART2
 * RG2 = PN532 TX
 * RG1 = PN532 RX
 * RD7 = RSTPD_N
 */
#define PN532_RESET       LATDbits.LATD7
//Set tris bits for the above pins only - USART pins are handled separately
#define PN532_TRIS()      TRISDbits.TRISD7 = 0

#define USARTx       2

#elif defined SPI_INTERFACE
//uController pin assignments for SPI
/*
 Pin assignmnets for SPI
 *  MSSP2
 * RD2 = NSS
 * RD3 = P70_IRQ
 * RD4 = MOSI
 * RD5 = MISO
 * RD6 = SCK
 * RD7 = RSTPD_N
 *
 *  MSSP1
 * RC5 = MOSI
 * RC4 = MISO
 * RC3 = SCK
 *
 */
#define PN532_RESET       LATDbits.LATD7
#define PN532_IRQ         PORTDbits.RD3
#define PN532_NSS         LATDbits.LATD2

//Set tris bits for the above pins only - SPI pins are handled separately
#define PN532_TRIS()      TRISDbits.TRISD7 = 0;TRISDbits.TRISD3 = 1;\
                          TRISDbits.TRISD2 = 0

#define MSSPx           2   //1 or 2 to select MSSP1 or MSSP2 for SPI
#else
#error No interface defined
#endif
#endif	/* PN532INTERFACE_H */

