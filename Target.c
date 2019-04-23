#include <xc.h>
#include "PN532.h"
#include "Target.h"
#include <string.h>

#define PICC_MF_AUTH_A      0x60
#define PICC_MF_AUTH_B      0x61
#define PICC_MF_READ        0x30
#define PICC_MF_WRITE       0xa0
#define PICC_MF_DECREMENT   0xc0
#define PICC_MF_INCREMENT   0xc1
#define PICC_MF_RESTORE     0xc2
#define PICC_MF_TRANSFER    0xb0
#define PICC_MFUL_WRITE     0xa2

char InitTarget(char *nfcid, unsigned int sensRes, char selRes) {
    char buffer[37];
    char status;
    char len = 37;

    buffer[0] = 0b00000001;  //Passive only
    *((int *)&buffer[1]) = sensRes;
    memcpy(&buffer[3], nfcid, 3);
    buffer[6] = selRes;
    for (status = 7; status < 37; ++status) {
        buffer[status] = 0;
    }
    status = PN532Command(TgInitAsTarget, buffer, 37, buffer, &len, 0);
    if (status) {
        return status;
    }
    if ((buffer[0] & 0b00000011) != 0) {
        return STATUS_PROTOCOL_ERROR;
    }
    return status;
}

char card[41][4];

void InitCard(char *nfcid) {
    char i;
    char j;

    for (i = 0; i < 41; ++i) {
        for (j = 0; j < 4; ++j) {
            card[i][j] = i * 4 + j;
        }
    }
    
}

char Emulate(char *nfcid) {
    char status;
    char active = 1;
    char buffer[18];
    char len = 18;
    char block;

    InitCard(nfcid);
    status = InitTarget(nfcid, 0x0044, 0x40);
    if (status) {
        return status;
    }
    while (active) {
        status = PN532Command(TgGetData, buffer, 0, buffer, &len, 1);
        if (status) {
            break;
        }
        switch(buffer[0]) {
            case PICC_MF_READ:
                block = buffer[1];
                memcpy(buffer, &card[block], 16);
                status = PN532Command(TgSetData, buffer, 16, 0, 0, 1);
                break;
            default:
                break;
        }
    }
    return STATUS_OK;
}
