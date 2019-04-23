/* 
 * File:   PN532.h
 * Author: bmcgarvey
 *
 * Created on February 20, 2015, 8:03 AM
 */

#ifndef PN532_H
#define	PN532_H

//Error codes - See also the error codes returned by some commands
#define STATUS_OK           0
#define STATUS_BUFFER_SIZE  0x30
#define STATUS_COMM_ERROR   0x31
#define STATUS_TIMEOUT      0x33
#define STATUS_PROTOCOL_ERROR  0x37

//Commands
#define Diagnose                0x00
#define GetFirmwareVersion      0x02
#define GetGeneralStatus        0x04
#define ReadRegister            0x06
#define WriteRegister           0x08
#define ReadGPIO                0x0c
#define WriteGPIO               0x0e
#define SetSerialBaudRate       0x10
#define SetParameters           0x12
#define SAMConfiguration        0x14
#define PowerDown               0x16

#define RFConfiguration         0x32
#define RFRegulationTest        0x58

#define InJumpForDEP            0x56
#define InJumpForPSL            0x46
#define InListPassiveTarget     0x4a
#define InATR                   0x50
#define InPSL                   0x4e
#define InDataExchange          0x40
#define InCommunicateThru       0x42
#define InDeselect              0x44
#define InRelease               0x52
#define InSelect                0x54
#define InAutoPoll              0x60

#define TgInitAsTarget          0x8c
#define TgSetGeneralBytes       0x92
#define TgGetData               0x86
#define TgSetData               0x8e
#define TgSetMetaData           0x94
#define TgGetInitiatorCommand   0x88
#define TgResponseToInitiator   0x90
#define TgGetTargetStatus       0x8a

//Used by dialog protocol
#define TFI_HOST            0xd4
#define TFI_PN532           0xd5

//Interface functions. Must be provided by all interface types
void PN532Init(void);
void PN532WakeUp(void);
char PN532GetResponse(char *data, char *len, char wantStatus);
char PN532SendCommandFrame(char command, char *data, char dataLen);
char PN532GetACK(void);

//Support functions
void PN532Reset(void);
char PN532Command(char command, char *out, char outLen, char *in, char *inLen, char wantStatus);
char PN532GetFirmwareVersion(char *verStr);
char PN532PowerDown(void);
char PN532WriteRegister(unsigned int address, char value);
char PN532ReadRegister(unsigned int address, char *value);
char PN532GetGeneralStatus(char *data);


#endif	/* PN532_H */

