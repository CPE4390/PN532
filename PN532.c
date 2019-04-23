#include <xc.h>
#include "PN532.h"
#include "PN532Interface.h"
#include "string.h"

void PN532Reset(void) {
    PN532_RESET = 0;
    __delay_ms(10);
    PN532_RESET = 1;
    __delay_ms(10);
}

char PN532Command(char command, char *out, char outLen, char *in, char *inLen,
        char wantStatus) {
    char status;

    status = PN532SendCommandFrame(command, out, outLen);
    if (status) {
        return status;
    }
    status = PN532GetACK();
    if (status) {
        return status;
    }
    status = PN532GetResponse(in, inLen, wantStatus);
    return status;
}

char PN532GetFirmwareVersion(char *verStr) {
    char buffer[5];
    char len = 5;
    char status;

    status = PN532Command(GetFirmwareVersion, NULL, 0, buffer, &len, 0);
    if (status) {
        return status;
    }
    strcpy(verStr, "PN532 0.00");
    verStr[6] = buffer[1] + '0';
    verStr[8] = (buffer[2] / 10) + '0';
    verStr[9] = (buffer[2] % 10) + '0';
    return STATUS_OK;
}

char PN532PowerDown(void) {
    char buffer[2];
    char len = 2;
    char status;

    buffer[0] = 0b10110000;  //Wake on I2C, SPI or HSU
    buffer[1] = 1;
    status = PN532Command(PowerDown, buffer, 2, buffer, &len, 1);
    if (status) {
        return status;
    }
    __delay_ms(1);
    return STATUS_OK;
}

char PN532WriteRegister(unsigned int address, char value) {
    char buffer[3];
    char len = 3;

    buffer[0] = address >> 8;
    buffer[1] = address;
    buffer[2] = value;
    return PN532Command(WriteRegister, buffer, 3, 0, 0, 0);
}

char PN532ReadRegister(unsigned int address, char *value) {
    char buffer[2];
    char len = 2;
    char status;

    buffer[0] = address >> 8;
    buffer[1] = address;
    status = PN532Command(ReadRegister, buffer, 2, buffer, &len, 0);
    *value = buffer[0];
    return status;
}
