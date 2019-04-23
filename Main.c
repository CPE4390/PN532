/* 
 * File:   Main.c
 * Author: bmcgarvey
 *
 * Created on January 16, 2015, 8:29 AM
 */

#include <xc.h>
#include "LCD.h"
#include "PN532.h"
#include "PCD.h"

char buffer[16];
TypeATarget targets[2];
MifareKey key = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
AccessBits ab = {BLOCK_TRANSPORT, VALUE_DEC_AB_INC_B, BLOCK_TRANSPORT, ACCESS_BITS_KEYB};
char sector = 13;
char block = 2;

void InitSystem(void);

void main(void) {
    char status;
    char i;
    char tgFound;
    long value=-1;
    InitSystem();
    LCDInit();

    PN532Init();
    PN532Reset();
    PN532WakeUp();
    PN532GetFirmwareVersion(buffer);
    lprintf(0, buffer);
    PCDConfigureSAM();
    for (i = 0; i < 16; ++i) {
        buffer[i] = 0;
    }
    while (1) {
        status = PCDGetPassiveTarget(targets, 1, &tgFound, 0xff);
        LCDClear();
        for (i = 0; i < tgFound; ++i) {
            lprintf(0, "%d %d %02x%02x%02x%02x", targets[i].Tg, targets[i].sak,
                    targets[i].uid[0], targets[i].uid[1], targets[i].uid[2],
                    targets[i].uid[3]);
            if (targets[i].uidSize > 4) {
                lprintf(1, "%02x%02x%02x", targets[i].uid[4], targets[i].uid[5],
                        targets[i].uid[6]);
            }
            while (PORTBbits.RB0 == 1);
            __delay_ms(15);
            while (PORTBbits.RB0 == 0);
            __delay_ms(15);
            status = MFAuthenticate(&targets[0], sector, key, KEY_A);
            //status = MFWriteBlock(&targets[0], sector, block, buffer);
            //status = MFWriteSectorTrailer(&targets[0], sector, key, key, &ab);
            //status = MFWriteValue(&targets[0], sector, block, 12345);
            status = MFReadBlock(&targets[0], sector, block, buffer);
            //status = MFDecrement(&targets[0], sector, block, 100);
            //status = MFReadValue(&targets[0], sector, block, &value);
            lprintf(1, "%02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3]);
            //lprintf(1, "%ld", value);
            while (PORTBbits.RB0 == 1);
            __delay_ms(15);
            while (PORTBbits.RB0 == 0);
            __delay_ms(15);
        }
    }
}

void InitSystem(void) {
    OSCTUNEbits.PLLEN = 1;
    TRISBbits.RB0 = 1;
}

void __interrupt(high_priority) HighISR(void) {

}

