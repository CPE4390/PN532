/* 
 * File:   PCD.h
 * Author: Brad McGarvey
 *
 * Created on February 22, 2015, 8:16 PM
 */

#ifndef PCD_H
#define	PCD_H

//PICC Types
#define PICC_UNKNOWN        0x00
#define PICC_ISO_14443_4    0x01
#define PICC_MIFARE_MINI    0x02
#define PICC_MIFARE_1K      0x03
#define PICC_MIFARE_4K      0x04
#define PICC_MIFARE_UL      0x05
#define PICC_MIFARE_PLUS    0x06
#define PICC_ISO_18092      0x07
#define PICC_NOT_COMPLETE   0x80

//Mifare access bit constants
#define ACCESS_BITS_TRANSPORT       0b001
#define ACCESS_BITS_KEYB            0b011
#define BLOCK_TRANSPORT             0b000
#define BLOCK_READ_AB               0b010
#define BLOCK_READ_AB_WRITE_B       0b100
#define BLOCK_READ_B_WRITE_B        0b011
#define BLOCK_READ_B                0b101
#define BLOCK_NONE                  0b111
#define VALUE_DEC_AB_INC_B          0b110
#define VALUE_DEC_AB_NO_W_OR_INC    0b001

//Mifare classic key A or B
#define KEY_A   0
#define KEY_B   1

typedef struct {
    char Tg;
    char uid[10];
    char uidSize;
    char sak;
} TypeATarget;

//Mifare classic key
typedef char MifareKey[6];

//Mifare Ultralight Key
typedef char MifareULKey[8];

//Access bits for Mifare classic sector trailer
typedef struct {
    char B0;
    char B1;
    char B2;
    char B3;
} AccessBits;

//PCD functions
char PCDConfigureSAM(void);
char PCDGetPassiveTarget(TypeATarget *targets, char maxTargets, char *found,
        char retries);
char PCDSelect(char tg);
char PCDRelease(char tg);
char PCDDeselect(char tg);
char PCDAutoPoll(void);  //Needs some parameters!
char PCDGetTargetType(TypeATarget *target);

//Mifare classic functions
char MFReadBlock(TypeATarget *target, char sector, char block, char *data);
char MFWriteBlock(TypeATarget *target, char sector, char block, char *data);
char MFWriteValue(TypeATarget *target, char sector, char block, long value);
char MFReadValue(TypeATarget *target, char sector, char block, long *value);
char MFIncrement(TypeATarget *target, char sector, char block, long offset);
char MFDecrement(TypeATarget *target, char sector, char block, long offset);
char MFWriteSectorTrailer(TypeATarget *target, char sector, MifareKey keyA,
        MifareKey keyB, AccessBits *accessBits);
char MFReadSectorTrailer(TypeATarget *target, char sector, MifareKey keyB,
        AccessBits *accesBits);
char MFAuthenticate(TypeATarget *target, char sector, MifareKey key,
        char keyType);

//Mifare Ultralight and NTAG203 functions
char MFULReadBlock(TypeATarget *target, char block, char *data);
char MFULWriteBlock(TypeATarget *target, char block, char *data);
char MFULReadCounter(TypeATarget *target, unsigned int *value);
char MFULWriteCounter(TypeATarget *target, unsigned int value);
char MFULWriteOTP(TypeATarget *target, char *data);
char MFULSetLockBytes(TypeATarget *target, char b1, char b2, char b3, char b4);
char MFULGetLockBytes(TypeATarget *target, char *b1, char *b2, char *b3,
        char *b4);

//Mifare Ultralight functions
char MFULSetKey(TypeATarget *target, MifareULKey *key);
char MFULAuthenticate(TypeATarget *target, MifareULKey *key);

#endif	/* PCD_H */

