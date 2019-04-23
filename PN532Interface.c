#include <xc.h>
#include "PN532Interface.h"
#include "PN532.h"

//TODO Modify all interface types to support extended frames > 255 bytes

#ifdef I2C_INTERFACE

#define I2C_ADDRESS_W     0x48
#define I2C_ADDRESS_R     0x49
#define I2C_ACK             0
#define I2C_NACK            1

#if MSSPx == 1
#define SSPxADD         SSPADD
#define SSPxCON1bits    SSPCON1bits
#define SSPxCON2bits    SSPCON2bits
#define SSPxSTATbits    SSPSTATbits
#define SSPxBUF         SSPBUF
#define I2C_TRIS()      (TRISC |= 0b00011000)  //RC4=SDA, RC3=SCL
//#define I2C_TRIS()    (TRISB |= 0b00000011)  //RB0=SDA, RB1=SCL
#elif MSSPx == 2
#define SSPxADD         SSP2ADD
#define SSPxCON1bits    SSP2CON1bits
#define SSPxCON2bits    SSP2CON2bits
#define SSPxSTATbits    SSP2STATbits
#define SSPxBUF         SSP2BUF
#define I2C_TRIS()      (TRISD |= 0b01100000)  //RD5=SDA, RD6=SCL
#else
#error Invalid MSSPx selection
#endif

char I2CRead(char ack) {
    char byte;

    SSPxCON2bits.RCEN = 1;
    while (!SSPxSTATbits.BF);
    byte = SSPxBUF;
    SSPxCON2bits.ACKDT = ack;
    SSPxCON2bits.ACKEN = 1;
    while (SSPxCON2bits.ACKEN != 0);
    return byte;
}

void PN532Init(void) {
    PN532_RESET = 1;
    PN532_TRIS();
    I2C_TRIS();
    SSPxADD = (_XTAL_FREQ / (4 * I2C_BAUD)) - 1; //BAUD = FOSC/(4 * (SSPADD + 1))
#if I2C_BAUD > 100000L
    SSPxSTATbits.SMP = 0;
#else
    SSPxSTATbits.SMP = 1;
#endif
    SSPxCON1bits.SSPM = 0b1000; //I2C Master mode
    SSPxCON1bits.SSPEN = 1; //Enable MSSP
    __delay_ms(10);
}

void PN532WakeUp(void) {
    SSPxCON2bits.SEN = 1;
    while (SSPxCON2bits.SEN == 1);
    __delay_ms(1);
    SSPxBUF = I2C_ADDRESS_W;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxCON2bits.PEN = 1;
    while (SSPxCON2bits.PEN == 1);
}

char PN532GetResponse(char *data, char *len, char wantStatus) {
    char status;
    char i;
    char rxLen;
    char checkSum;

    SSPxCON1bits.WCOL = 0;
    SSPxCON1bits.SSPOV = 0;
    while (PN532_IRQ == 1);
    SSPxCON2bits.SEN = 1;
    while (SSPxCON2bits.SEN == 1);
    SSPxBUF = I2C_ADDRESS_R;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    status = I2CRead(I2C_ACK);
    status &= 1;
    if (!status) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return STATUS_COMM_ERROR;
    }
    I2CRead(I2C_ACK);
    status = I2CRead(I2C_ACK);
    i = I2CRead(I2C_ACK);
    if (status != 0x00 || i != 0xff) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return STATUS_COMM_ERROR;
    }
    rxLen = I2CRead(I2C_ACK);
    checkSum = rxLen + I2CRead(I2C_ACK);
    if (checkSum != 0) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return STATUS_COMM_ERROR;
    }
    checkSum = I2CRead(I2C_ACK);
    if (checkSum != TFI_PN532) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return STATUS_COMM_ERROR;
    }
    status = I2CRead(I2C_ACK); //this byte is Command+1 - discard it
    checkSum += status;
    rxLen -= 2;
    if (wantStatus) {
        status = I2CRead(I2C_ACK);
        checkSum += status;
        --rxLen; //Don't count status as data if we want it returned
    } else {
        status = STATUS_OK;
    }
    if (rxLen > 0 && (!data || !len || (*len < rxLen))) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        if (len) {
            *len = rxLen;
        }
        return STATUS_BUFFER_SIZE;
    }
    if (len) {
        *len = rxLen;
    }
    for (i = 0; i < rxLen; ++i) {
        data[i] = I2CRead(I2C_ACK);
        checkSum += data[i];
    }
    checkSum += I2CRead(I2C_ACK);
    if (checkSum != 0) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return STATUS_COMM_ERROR;
    }
    I2CRead(I2C_NACK);
    SSPxCON2bits.PEN = 1;
    while (SSPxCON2bits.PEN == 1);
    return status;
}

char PN532SendCommandFrame(char command, char *data, char dataLen) {
    char i;
    char checkSum = TFI_HOST;


    SSPxCON2bits.SEN = 1;
    while (SSPxCON2bits.SEN == 1);
    SSPxBUF = I2C_ADDRESS_W;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    if (SSPxCON2bits.ACKSTAT == 1) //NACK
    {
        return STATUS_COMM_ERROR;
    }
    SSPxBUF = 0x00;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = 0x00;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = 0xff;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = dataLen + 2;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = -(dataLen + 2);
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = TFI_HOST;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = command;
    checkSum += command;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    for (i = 0; i < dataLen; ++i) {
        SSPxBUF = data[i];
        checkSum += data[i];
        while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    }
    SSPxBUF = -checkSum;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxBUF = 0x00;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    SSPxCON2bits.PEN = 1;
    while (SSPxCON2bits.PEN == 1);
    return STATUS_OK;
}

char PN532GetACK(void) {
    char status;
    char i;
    char byte;

    SSPxCON1bits.WCOL = 0;
    SSPxCON1bits.SSPOV = 0;
    i = PN532_ACK_TIMEOUT;
    while (i > 0) {
        if (PN532_IRQ == 0) {
            break;
        }
        __delay_us(100);
        --i;
    }
    if (i == 0) {
        return STATUS_TIMEOUT;
    }
    SSPxCON2bits.SEN = 1;
    while (SSPxCON2bits.SEN == 1);
    SSPxBUF = I2C_ADDRESS_R;
    while (SSPxSTATbits.BF || SSPxSTATbits.R_W);
    status = I2CRead(I2C_ACK);
    status &= 1;
    if (!status) {
        SSPxCON2bits.PEN = 1;
        while (SSPxCON2bits.PEN == 1);
        return 0;
    }
    status = STATUS_OK;
    for (i = 0; i < 5; ++i) {
        byte = I2CRead(I2C_ACK);
        if (i == 2 || i == 4) {
            if (byte != 0xff) {
                status = STATUS_COMM_ERROR;
            }
        } else if (byte != 0) {
            status = STATUS_COMM_ERROR;
        }
    }
    I2CRead(I2C_NACK);
    SSPxCON2bits.PEN = 1;
    while (SSPxCON2bits.PEN == 1);
    return status;
}
#endif

#ifdef HSU_INTERFACE

#if USARTx == 1
#define CRENx   RCSTAbits.CREN
#define RCxIF   RCIF
#define RCREGx  RCREG
#elif USARTx == 2
#define CRENx   RCSTA2bits.CREN2
#define RCxIF   RC2IF
#define RCREGx  RCREG2
#else
#error Invalid USART selection
#endif

void TXByte(char b) {
#if USARTx == 1
    while (TX1IF == 0);
    TXREG1 = b;
#elif USARTx == 2
    while (TX2IF == 0);
    TXREG2 = b;
#endif
}

#define BAUD    ((unsigned int)((_XTAL_FREQ / (4.0 * 115200L)) - 0.5))

void PN532Init(void) {
    PN532_TRIS();
#if USARTx == 1
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;
    //Configure the USART for 115,200 baud asynchronous transmission
    SPBRG1 = 86;//BAUD;
    SPBRGH1 = 0;//BAUD >> 8;
    TXSTA1bits.BRGH = 1;
    BAUDCON1bits.BRG16 = 1;
    TXSTA1bits.SYNC = 0;
    RCSTA1bits.SPEN = 1;
    TXSTA1bits.TXEN = 1;
#elif USARTx == 2
    TRISGbits.TRISG1 = 0;
    TRISGbits.TRISG2 = 1;
    //Configure the USART for 115,200 baud asynchronous transmission
    SPBRG2 = BAUD;
    SPBRGH2 = BAUD >> 8;
    TXSTA2bits.BRGH = 1;
    BAUDCON2bits.BRG16 = 1;
    TXSTA2bits.SYNC = 0;
    RCSTA2bits.SPEN = 1;
    TXSTA2bits.TXEN = 1;
#endif
}

void PN532WakeUp(void) {
    TXByte(0x55);
    __delay_ms(10);
    TXByte(0x55);
    __delay_ms(1);
}

char PN532GetResponse(char *data, char *len, char wantStatus) {
    char rx;
    char i = 0x55;
    char rxLen;
    char checkSum;
    char status;

    CRENx = 1;
    while (1) {
        if (RCxIF == 1) {
            rx = RCREGx;
            if (rx == 0xff && i == 0x00) {
                break;
            } else {
                i = rx;
            }
        }
    }
    while (RCxIF == 0);
    rxLen = RCREGx;
    while (RCxIF == 0);
    checkSum = rxLen + RCREGx;
    if (checkSum != 0) {
        CRENx = 0;
        return STATUS_COMM_ERROR;
    }
    while (RCxIF == 0);
    checkSum = RCREGx;
    if (checkSum != TFI_PN532) {
        CRENx = 0;
        return STATUS_COMM_ERROR;
    }
    while (RCxIF == 0);
    rx = RCREGx; //Discard command+1
    checkSum += rx;
    rxLen -= 2;
    if (wantStatus) {
        while (RCxIF == 0);
        status = RCREGx;
        checkSum += status;
        --rxLen;
    } else {
        status = STATUS_OK;
    }
    if (rxLen > 0 && (!data || !len || (*len < rxLen))) {
        CRENx = 0;
        if (len) {
            *len = rxLen;
        }
        return STATUS_BUFFER_SIZE;
    }
    if (len) {
        *len = rxLen;
    }
    for (i = 0; i < rxLen; ++i) {
        while (RCxIF == 0);
        data[i] = RCREGx;
        checkSum += data[i];
    }
    while (RCxIF == 0);
    checkSum += RCREGx;
    if (checkSum != 0) {
        CRENx = 0;
        return STATUS_COMM_ERROR;
    }
    while (RCxIF == 0);
    rx = RCREGx;
    CRENx = 0;
    return status;
}

char PN532SendCommandFrame(char command, char *data, char dataLen) {
    char i;
    char checkSum = TFI_HOST;

    TXByte(0x00);
    TXByte(0x00);
    TXByte(0xff);
    TXByte(dataLen + 2);
    TXByte(-(dataLen + 2));
    TXByte(TFI_HOST);
    TXByte(command);
    checkSum += command;
    for (i = 0; i < dataLen; ++i) {
        TXByte(data[i]);
        checkSum += data[i];
    }
    TXByte(-checkSum);
    TXByte(0x00);
    return STATUS_OK;
}

char PN532GetACK(void) {
    char last = 0x55;
    char rx;
    char i;

    CRENx = 1;
    i = PN532_ACK_TIMEOUT;
    while (i > 0) {
        if (RCxIF == 1) {
            rx = RCREGx;
            if (rx == 0xff && last == 0x00) {
                break;
            } else {
                last = rx;
            }
        } else {
            --i;
            __delay_us(100);
        }
    }
    if (i == 0) {
        CRENx = 0;
        return STATUS_TIMEOUT;
    }
    while (RCxIF == 0);
    rx = RCREGx;
    if (rx != 0x00) {
        CRENx = 0;
        return STATUS_COMM_ERROR;
    }
    while (RCxIF == 0);
    rx = RCREGx;
    if (rx != 0xff) {
        CRENx = 0;
        return STATUS_COMM_ERROR;
    }
    CRENx = 0;
    return STATUS_OK;
}
#endif

#ifdef SPI_INTERFACE
#define PN532_DW        0b00000001
#define PN532_DR        0b00000011
#define PN532_SR        0b00000010

#if MSSPx == 1
#define SSPxCON1bits    SSPCON1bits
#define SSPxSTATbits    SSPSTATbits
#define SSPxBUF         SSPBUF
    //SDO=RC5, SDI=RC4, SCL=RC3
#define SPI_TRIS()      TRISCbits.TRISC3=0;TRISCbits.TRISC4=1;TRISCbits.TRISC5=0
#define SSPxIF          SSPIF
#elif MSSPx == 2
#define SSPxCON1bits    SSP2CON1bits
#define SSPxSTATbits    SSP2STATbits
#define SSPxBUF         SSP2BUF
    //SDO=RD4, SDI=RD5, SCL=RD6
#define SPI_TRIS()      TRISDbits.TRISD4=0;TRISDbits.TRISD5=1;TRISDbits.TRISD6=0
#define SSPxIF          SSP2IF
#else
#error Invalid MSSPx selection
#endif

char SPIRead(void) {
    char b;
    SSPxIF = 0;
    SSPxBUF = 0;
    while (SSPxIF == 0);
    b = SSPxBUF;
    b = ((b >> 1) & 0x55) | ((b & 0x55) << 1);
    b = ((b >> 2) & 0x33) | ((b & 0x33) << 2);
    b = ((b >> 4) & 0x0F) | ((b & 0x0F) << 4);
    return b;
}

void SPIWrite(char b) {
    b = ((b >> 1) & 0x55) | ((b & 0x55) << 1);
    b = ((b >> 2) & 0x33) | ((b & 0x33) << 2);
    b = ((b >> 4) & 0x0F) | ((b & 0x0F) << 4);
    SSPxIF = 0;
    SSPxBUF = b;
    while (SSPxIF == 0);
}

void PN532Init(void) {
    PN532_NSS = 1;
    PN532_RESET = 1;
    PN532_TRIS();
    SPI_TRIS();
    SSPxCON1bits.CKP = 0;
    SSPxSTATbits.CKE = 1;
    SSPxSTATbits.SMP = 0;
    SSPxCON1bits.SSPM = 0b0001;
    SSPxCON1bits.SSPEN = 1; //Enable MSSP
    __delay_ms(10);
}

void PN532WakeUp(void) {
    PN532_NSS = 0;
    __delay_ms(10);
    PN532_NSS = 1;
}

char PN532GetResponse(char *data, char *len, char wantStatus) {
    char status;
    char i;
    char rxLen;
    char checkSum;

    while (PN532_IRQ == 1);
    PN532_NSS = 0;
    SPIWrite(PN532_DR);
    SPIRead();
    status = SPIRead();
    i = SPIRead();
    if (status != 0x00 || i != 0xff) {
        PN532_NSS = 1;
        return STATUS_COMM_ERROR;
    }
    rxLen = SPIRead();
    checkSum = rxLen + SPIRead();
    if (checkSum != 0) {
        PN532_NSS = 1;
        return STATUS_COMM_ERROR;
    }
    checkSum = SPIRead();
    if (checkSum != TFI_PN532) {
        PN532_NSS = 1;
        return STATUS_COMM_ERROR;
    }
    checkSum += SPIRead(); //Discard command+1
    rxLen -= 2;
    if (wantStatus) {
        status = SPIRead();
        checkSum += status;
        --rxLen;
    } else {
        status = STATUS_OK;
    }
    if (rxLen > 0 && (!data || !len || (*len < rxLen))) {
        PN532_NSS = 1;
        if (len) {
            *len = rxLen;
        }
        return STATUS_BUFFER_SIZE;
    }
    if (len) {
        *len = rxLen;
    }
    for (i = 0; i < rxLen; ++i) {
        data[i] = SPIRead();
        checkSum += data[i];
    }
    checkSum += SPIRead();
    if (checkSum != 0) {
        PN532_NSS = 1;
        return STATUS_COMM_ERROR;
    }
    SPIRead();
    PN532_NSS = 1;
    return status;
}

char PN532SendCommandFrame(char command, char *data, char dataLen) {
    char i;
    char checkSum = TFI_HOST;

    PN532_NSS = 0;
    __delay_us(100);
    SPIWrite(PN532_DW);
    SPIWrite(0x00);
    SPIWrite(0x00);
    SPIWrite(0xff);
    SPIWrite(dataLen + 2);
    SPIWrite(-(dataLen + 2));
    SPIWrite(TFI_HOST);
    SPIWrite(command);
    for (i = 0; i < dataLen; ++i) {
        SPIWrite(data[i]);
        checkSum += data[i];
    }
    SPIWrite(-checkSum);
    SPIWrite(0x00);
    PN532_NSS = 1;
    return STATUS_OK;
}

char PN532GetACK(void) {
    char status;
    char i;
    char byte;

    i = PN532_ACK_TIMEOUT;
    while (i > 0) {
        if (PN532_IRQ == 0) {
            break;
        }
        __delay_us(100);
        --i;
    }
    if (i == 0) {
        return STATUS_TIMEOUT;
    }
    PN532_NSS = 0;
    SPIWrite(PN532_DR);
    status = STATUS_OK;
    for (i = 0; i < 6; ++i) {
        byte = SPIRead();
        if (i == 2 || i == 4) {
            if (byte != 0xff) {
                status = STATUS_COMM_ERROR;
            }
        } else if (byte != 0) {
            status = STATUS_COMM_ERROR;
        }
    }
    PN532_NSS = 1;
    return status;
}
#endif