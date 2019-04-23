#include <xc.h>
#include "PN532.h"
#include "PCD.h"
#include "string.h"


#define PICC_MF_AUTH_A      0x60
#define PICC_MF_AUTH_B      0x61
#define PICC_MF_READ        0x30
#define PICC_MF_WRITE       0xa0
#define PICC_MF_DECREMENT   0xc0
#define PICC_MF_INCREMENT   0xc1
#define PICC_MF_RESTORE     0xc2
#define PICC_MF_TRANSFER    0xb0
#define PICC_MFUL_WRITE     0xa2

char PCDSetPassiveActivationRetries(char max);
char MFGetAccessBits(AccessBits *accessBits, char *buffer);
void MFSetAccessBits(AccessBits *accessBits, char *buffer);
char MFValidateAccessBits(char *buffer);
char MFTransfer(TypeATarget *target, char sector, char block);
char MFRestore(TypeATarget *target, char sector, char block);
char MFULWrite(TypeATarget *target, char block, char *data);

char PCDConfigureSAM(void) {
    char buffer[3];
    char len = 3;

    buffer[0] = 0x01; //Normal mode
    buffer[1] = 0x14; //Timeout in mode0x02 = 1s
    buffer[2] = 0x01; //Use P70_IRQ
    return PN532Command(SAMConfiguration, buffer, 3, buffer, &len, 0);
}

char PCDSetPassiveActivationRetries(char max) {
    char buffer[4];
    char len = 4;

    buffer[0] = 5;
    buffer[1] = 0xff;
    buffer[2] = 0x01;
    buffer[3] = max;
    return PN532Command(RFConfiguration, buffer, 4, buffer, &len, 0);
}

char PCDGetPassiveTarget(TypeATarget *targets, char maxTargets, char *found,
        char retries) {
    char buffer[32];
    char len = 32;
    char status;
    char i;
    char pos;
    char resLen;

    status = PCDSetPassiveActivationRetries(retries);
    if (status) {
        return status;
    }
    buffer[0] = maxTargets;
    buffer[1] = 0x00; //106 kbps cards
    status = PN532Command(InListPassiveTarget, buffer, 2, buffer, &len, 0);
    if (status) {
        return status;
    }
    *found = buffer[0];
    resLen = (len - 1) / *found;
    for (i = 0; i < *found; ++i) {
        pos = resLen * i + 1;
        targets[i].Tg = buffer[pos];
        targets[i].uidSize = buffer[pos + 4];
        memcpy(targets[i].uid, &buffer[pos + 5], buffer[pos + 4]);
        targets[i].sak = buffer[pos + 3];
    }
    return STATUS_OK;
}

char PCDSelect(char tg) {
    return PN532Command(InSelect, &tg, 1, 0, 0, 1);
}

char PCDRelease(char tg) {
    return PN532Command(InRelease, &tg, 1, 0, 0, 1);
}

char PCDDeselect(char tg) {
    return PN532Command(InDeselect, &tg, 1, 0, 0, 1);
}

char PCDAutoPoll(void); //Needs some parameters!

//Mifare classic functions

char MFReadBlock(TypeATarget *target, char sector, char block, char *data) {
    char buffer[3];
    char len = 16;
    char status;

    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_READ;
    buffer[2] = sector * 4 + block;
    status = PN532Command(InDataExchange, buffer, 3, data, &len, 1);
    return status;

}

char MFWrite(TypeATarget *target, char block, char *data) {
    char buffer[19];
    char status;

    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_WRITE;
    buffer[2] = block;
    memcpy(&buffer[3], data, 16);
    status = PN532Command(InDataExchange, buffer, 19, 0, 0, 1);
    return status;
}

char MFWriteBlock(TypeATarget *target, char sector, char block, char *data) {
    char address;

    address = sector * 4 + block;
    if (address % 4 == 3) {
        return STATUS_PROTOCOL_ERROR; //This is a sector trailer!
    }
    return MFWrite(target, address, data);
}

char MFWriteValue(TypeATarget *target, char sector, char block, long value) {
    char buffer[16];
    char address;

    address = sector * 4 + block;
    *((long *) buffer) = value;
    *((long *) (&buffer[8])) = value;
    *((long *) (&buffer[4])) = ~value;
    buffer[12] = buffer[14] = address;
    buffer[13] = buffer[15] = ~address;
    return MFWriteBlock(target, sector, block, buffer);
}

char MFReadValue(TypeATarget *target, char sector, char block, long *value) {
    char buffer[16];
    char status;

    status = MFReadBlock(target, sector, block, buffer);
    if (status) {
        return status;
    }
    if (*((long *) buffer) != *((long *) (&buffer[8]))) {
        return STATUS_PROTOCOL_ERROR;
    }
    if (*((long *) buffer) != ~(*((long *) (&buffer[4])))) {
        return STATUS_PROTOCOL_ERROR;
    }
    *value = *((long *) buffer);
    return STATUS_OK;
}

char MFIncrement(TypeATarget *target, char sector, char block, long offset) {
    char buffer[7];
    char status;
    char address;

    address = sector * 4 + block;
    if (address % 4 == 3) {
        return STATUS_PROTOCOL_ERROR;
    }
    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_INCREMENT;
    buffer[2] = address;
    *((long *) &buffer[3]) = offset;
    status = PN532Command(InDataExchange, buffer, 7, 0, 0, 1);
    if (status) {
        return status;
    }
    return MFTransfer(target, sector, block);
}

char MFDecrement(TypeATarget *target, char sector, char block, long offset) {
    char buffer[7];
    char status;
    char address;

    address = sector * 4 + block;
    if (address % 4 == 3) {
        return STATUS_PROTOCOL_ERROR;
    }
    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_DECREMENT;
    buffer[2] = address;
    *((long *) &buffer[3]) = offset;
    status = PN532Command(InDataExchange, buffer, 7, 0, 0, 1);
    if (status) {
        return status;
    }
    return MFTransfer(target, sector, block);
}

void MFSetAccessBits(AccessBits *accessBits, char *buffer) {
    char c1, c2, c3;

    c1 = ((accessBits->B0 & 4) << 2) | ((accessBits->B1 & 4) << 3) |
            ((accessBits->B2 & 4) << 4) | ((accessBits->B3 & 4) << 5);
    c2 = ((accessBits->B0 & 2) >> 1) | (accessBits->B1 & 2) |
            ((accessBits->B2 & 2) << 1) | ((accessBits->B3 & 2) << 2);
    c3 = ((accessBits->B0 & 1) << 4) | ((accessBits->B1 & 1) << 5) |
            ((accessBits->B2 & 1) << 6) | ((accessBits->B3 & 1) << 7);
    buffer[0] = ((~c2 & 0x0f) << 4) | ((~c1 & 0xf0) >> 4);
    buffer[1] = c1 | ((~c3 & 0xf0) >> 4);
    buffer[2] = (c3 & 0xf0) | (c2 & 0x0f);
}

char MFGetAccessBits(AccessBits *accessBits, char *buffer) {
    //First check format
    if (MFValidateAccessBits(buffer)) {
        return STATUS_PROTOCOL_ERROR;
    }
    accessBits->B0 = ((buffer[1] & 0b00010000) >> 2)
            | ((buffer[2] & 0b00000001) << 1) | ((buffer[2] & 0b00010000) >> 4);
    accessBits->B1 = ((buffer[1] & 0b00100000) >> 3)
            | (buffer[2] & 0b00000010) | ((buffer[2] & 0b00100000) >> 5);
    accessBits->B2 = ((buffer[1] & 0b01000000) >> 4)
            | ((buffer[2] & 0b00000100) >> 1) | ((buffer[2] & 0b01000000) >> 6);
    accessBits->B3 = ((buffer[1] & 0b10000000) >> 5)
            | ((buffer[2] & 0b00001000) >> 2) | ((buffer[2] & 0b10000000) >> 7);
    return STATUS_OK;
}

char MFValidateAccessBits(char *buffer) {
    if ((buffer[1] & 0x0f) & (buffer[2] >> 4)) {
        return STATUS_PROTOCOL_ERROR;
    }
    if ((buffer[0] >> 4) & (buffer[2] & 0x0f)) {
        return STATUS_PROTOCOL_ERROR;
    }
    if ((buffer[0] & 0x0f) & (buffer[1] >> 4)) {
        return STATUS_PROTOCOL_ERROR;
    }
    return STATUS_OK;
}

char MFWriteSectorTrailer(TypeATarget *target, char sector, MifareKey keyA,
        MifareKey keyB, AccessBits *accessBits) {
    char buffer[16];
    char address;

    address = sector * 4 + 3;
    memcpy(buffer, keyA, 6);
    memcpy(&buffer[10], keyB, 6);
    buffer[9] = 0;
    MFSetAccessBits(accessBits, &buffer[6]);
    if (MFValidateAccessBits(&buffer[6]) != STATUS_OK) {
        return STATUS_PROTOCOL_ERROR;
    }
    return MFWrite(target, address, buffer);
}

char MFReadSectorTrailer(TypeATarget *target, char sector, MifareKey keyB,
        AccessBits *accessBits) {
    char buffer[16];
    char status;

    status = MFReadBlock(target, sector, 3, buffer);
    if (status) {
        return status;
    }
    if (MFGetAccessBits(accessBits, &buffer[6]) != STATUS_OK) {
        return STATUS_PROTOCOL_ERROR;
    }
    memcpy(keyB, &buffer[10], 6);
    return STATUS_OK;
}

char MFAuthenticate(TypeATarget *target, char sector, MifareKey key,
        char keyType) {
    char buffer[13];
    char status;

    buffer[0] = target->Tg;
    if (keyType == KEY_A) {
        buffer[1] = PICC_MF_AUTH_A;
    } else if (keyType == KEY_B) {
        buffer[1] = PICC_MF_AUTH_B;
    } else {
        return STATUS_PROTOCOL_ERROR;
    }
    buffer[2] = sector * 4 + 3;
    memcpy(&buffer[3], key, 6);
    memcpy(&buffer[9], target->uid, 4);
    status = PN532Command(InDataExchange, buffer, 13, 0, 0, 1);
    return status;
}

char MFTransfer(TypeATarget *target, char sector, char block) {
    char buffer[3];
    char status;
    char address;

    address = sector * 4 + block;
    if (address % 4 == 3) {
        return STATUS_PROTOCOL_ERROR;
    }
    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_TRANSFER;
    buffer[2] = address;
    status = PN532Command(InDataExchange, buffer, 3, 0, 0, 1);
    return status;
}

char MFRestore(TypeATarget *target, char sector, char block) {
    char buffer[4];
    char status;
    char address;

    address = sector * 4 + block;
    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_RESTORE;
    buffer[2] = address;
    buffer[3] = 0;
    status = PN532Command(InDataExchange, buffer, 3, 0, 0, 1);
    return status;
}

//Mifare Ultralight and NTAG203 functions

char MFULWrite(TypeATarget *target, char block, char *data) {
    char buffer[7];
    char status;

    buffer[0] = target->Tg;
    buffer[1] = PICC_MFUL_WRITE;
    buffer[2] = block;
    memcpy(&buffer[3], data, 4);
    status = PN532Command(InDataExchange, buffer, 7, 0, 0, 1);
    return status;
}

char MFULReadBlock(TypeATarget *target, char block, char *data) {
    char buffer[16];
    char len = 16;
    char status;

    buffer[0] = target->Tg;
    buffer[1] = PICC_MF_READ;
    buffer[2] = block;
    status = PN532Command(InDataExchange, buffer, 3, buffer, &len, 1);
    if (status) {
        return status;
    }
    memcpy(data, buffer, 4);
    return STATUS_OK;
}

char MFULWriteBlock(TypeATarget *target, char block, char *data) {
    if (block < 4 || block > 0x27) {
        return STATUS_PROTOCOL_ERROR; //Not user memory
    }
    return MFULWrite(target, block, data);
}

char MFULReadCounter(TypeATarget *target, unsigned int *value) {
    char buffer[4];
    char status;

    status = MFULReadBlock(target, 0x29, buffer);
    if (status) {
        return status;
    }
    *value = *((unsigned int *) buffer);
    return STATUS_OK;
}

char MFULWriteCounter(TypeATarget *target, unsigned int value) {
    char buffer[4];

    *((unsigned int *) buffer) = value;
    buffer[2] = buffer[3] = 0;
    return MFULWrite(target, 0x29, buffer);
}

char MFULWriteOTP(TypeATarget *target, char *data) {
    return MFULWrite(target, 0x03, data);
}

char MFULSetLockBytes(TypeATarget *target, char b1, char b2, char b3, char b4) {
    char buffer[4];
    char status;

    status = MFULReadBlock(target, 2, buffer);
    if (status) {
        return status;
    }
    buffer[2] = b1;
    buffer[3] = b2;
    status = MFULWrite(target, 2, buffer);
    if (status) {
        return status;
    }
    buffer[0] = b3;
    buffer[1] = b4;
    buffer[2] = 0;
    buffer[3] = 0;
    status = MFULWrite(target, 0x28, buffer);
    if (status) {
        return status;
    }
    return STATUS_OK;
}

char MFULGetLockBytes(TypeATarget *target, char *b1, char *b2, char *b3,
        char *b4) {
    char buffer[4];
    char status;

    status = MFULReadBlock(target, 2, buffer);
    if (status) {
        return status;
    }
    *b1 = buffer[2];
    *b2 = buffer[3];
    status = MFULReadBlock(target, 0x28, buffer);
    if (status) {
        return status;
    }
    *b3 = buffer[0];
    *b4 = buffer[1];
    return STATUS_OK;
}

//Mifare Ultralight functions
char MFULSetKey(TypeATarget *target, MifareULKey *key);
char MFULAuthenticate(TypeATarget *target, MifareULKey *key);

//PICC functions

char PCDGetTargetType(TypeATarget *target) {
    if (target->sak & 0b00000100) {
        return PICC_NOT_COMPLETE;
    }
    switch (target->sak) {
        case 0x09: return PICC_MIFARE_MINI;
        case 0x08: return PICC_MIFARE_1K;
        case 0x18: return PICC_MIFARE_4K;
        case 0x00: return PICC_MIFARE_UL;
        case 0x10:
        case 0x11:return PICC_MIFARE_PLUS;
    }
    if (target->sak & 0b00100000) {
        return PICC_ISO_14443_4;
    }
    if (target->sak & 0b01000000) {
        return PICC_ISO_18092;
    }
    return PICC_UNKNOWN;
}