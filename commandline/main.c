/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id: main.c 787 2010-05-30 20:54:25Z cs $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "usbcalls.h"

#define IDENT_VENDOR_NUM        0x16c0
#define IDENT_VENDOR_STRING     "obdev.at"
#define IDENT_PRODUCT_NUM       1503
#define IDENT_PRODUCT_STRING    "HIDBoot"

// extra delays before more USB requests for tiny85 compatibility
#define TINY85_POSTWRITE_DELAY 8000
#define TINY85_FIRSTWRITE_DELAY 500000

/* ------------------------------------------------------------------------- */

static char dataBuffer[65536 + 256];    /* buffer for file data */
static int  startAddress, endAddress;
static char leaveBootLoader = 0;

/* ------------------------------------------------------------------------- */

static int  parseUntilColon(FILE *fp)
{
int c;

    do{
        c = getc(fp);
    }while(c != ':' && c != EOF);
    return c;
}

static int  parseHex(FILE *fp, int numDigits)
{
int     i;
char    temp[9];

    for(i = 0; i < numDigits; i++)
        temp[i] = getc(fp);
    temp[i] = 0;
    return strtol(temp, NULL, 16);
}

/* ------------------------------------------------------------------------- */

static int  parseIntelHex(char *hexfile, char buffer[65536 + 256], int *startAddr, int *endAddr)
{
int     address, base, d, segment, i, lineLen, sum;
FILE    *input;

    input = fopen(hexfile, "r");
    if(input == NULL){
        fprintf(stderr, "error opening %s: %s\n", hexfile, strerror(errno));
        return 1;
    }
    while(parseUntilColon(input) == ':'){
        sum = 0;
        sum += lineLen = parseHex(input, 2);
        base = address = parseHex(input, 4);
        sum += address >> 8;
        sum += address;
        sum += segment = parseHex(input, 2);  /* segment value? */
        if(segment != 0)    /* ignore lines where this byte is not 0 */
            continue;
        for(i = 0; i < lineLen ; i++){
            d = parseHex(input, 2);
            buffer[address++] = d;
            sum += d;
        }
        sum += parseHex(input, 2);
        if((sum & 0xff) != 0){
            fprintf(stderr, "Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
        }
        if(*startAddr > base)
            *startAddr = base;
        if(*endAddr < address)
            *endAddr = address;
    }
    fclose(input);
    return 0;
}

/* ------------------------------------------------------------------------- */

char    *usbErrorMessage(int errCode)
{
static char buffer[80];

    switch(errCode){
        case USB_ERROR_ACCESS:      return "Access to device denied";
        case USB_ERROR_NOTFOUND:    return "The specified device was not found";
        case USB_ERROR_BUSY:        return "The device is used by another application";
        case USB_ERROR_IO:          return "Communication error with device";
        default:
            sprintf(buffer, "Unknown USB error %d", errCode);
            return buffer;
    }
    return NULL;    /* not reached */
}

static int  getUsbInt(char *buffer, int numBytes)
{
int shift = 0, value = 0, i;

    for(i = 0; i < numBytes; i++){
        value |= ((int)*buffer & 0xff) << shift;
        shift += 8;
        buffer++;
    }
    return value;
}

static void setUsbInt(char *buffer, int value, int numBytes)
{
int i;

    for(i = 0; i < numBytes; i++){
        *buffer++ = value;
        value >>= 8;
    }
}

/* ------------------------------------------------------------------------- */

typedef struct deviceInfo{
    char    reportId;
    char    pageSize[2]; // TODO: change this to one byte?
    char    flashSize[4]; // TODO: change this to two bytes?
}deviceInfo_t;

typedef struct deviceData{
    char    reportId;
    char    address[3];
    char    data[128];
}deviceData_t;

static int uploadData(char *dataBuffer, int startAddr, int endAddr)
{
usbDevice_t *dev = NULL;
int         err = 0, len, mask, pageSize, deviceSize;
union{
    char            bytes[1];
    deviceInfo_t    info;
    deviceData_t    data;
}           buffer;
unsigned char firstWrite = 1; // track first page write request, accept extra delay

    if((err = usbOpenDevice(&dev, IDENT_VENDOR_NUM, IDENT_VENDOR_STRING, IDENT_PRODUCT_NUM, IDENT_PRODUCT_STRING, 1)) != 0){
        fprintf(stderr, "Error opening HIDBoot device: %s\n", usbErrorMessage(err));
        goto errorOccurred;
    }
    len = sizeof(buffer);
    if(endAddr > startAddr){    // we need to upload data
        if((err = usbGetReport(dev, USB_HID_REPORT_TYPE_FEATURE, 1, buffer.bytes, &len)) != 0){
            fprintf(stderr, "Error reading page size: %s\n", usbErrorMessage(err));
            goto errorOccurred;
        }
        if(len < sizeof(buffer.info)){
            fprintf(stderr, "Not enough bytes in device info report (%d instead of %d)\n", len, (int)sizeof(buffer.info));
            err = -1;
            goto errorOccurred;
        }
        pageSize = getUsbInt(buffer.info.pageSize, 2);
        deviceSize = getUsbInt(buffer.info.flashSize, 4);
        printf("Page size   = %d (0x%x)\n", pageSize, pageSize);
        printf("Device size = %d (0x%x)\n", deviceSize, deviceSize);
        if(endAddr > deviceSize){
            fprintf(stderr, "Data (%d bytes) exceeds remaining flash size!\n", endAddr);
            err = -1;
            goto errorOccurred;
        }
        if(pageSize < 128){
            mask = 127;
        }else{
            mask = pageSize - 1;
        }
        startAddr &= ~mask;                  /* round down */
        endAddr = (endAddr + mask) & ~mask;  /* round up */
        printf("Uploading %d (0x%x) bytes starting at %d (0x%x)\n", endAddr - startAddr, endAddr - startAddr, startAddr, startAddr);
        while(startAddr < endAddr){
            buffer.data.reportId = 2;
            memcpy(buffer.data.data, dataBuffer + startAddr, 128);
            setUsbInt(buffer.data.address, startAddr, 3);
            printf("\r0x%05x ... 0x%05x", startAddr, startAddr + (int)sizeof(buffer.data.data));
            fflush(stdout);
            if((err = usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.data))) != 0){
                fprintf(stderr, "Error uploading data block: %s\n", usbErrorMessage(err));
                goto errorOccurred;
            }
            startAddr += sizeof(buffer.data.data);
            
            // special tiny85 chillout session - chip freezes after write, so we
            // need to make sure we don't send it any requests while it's busy
            // erasing or writing
            if (firstWrite) usleep(TINY85_FIRSTWRITE_DELAY); // progmem erase extra time
            usleep(TINY85_POSTWRITE_DELAY); // regular page write duration
            firstWrite = 0;
        }
        printf("\n");
    }
    if(leaveBootLoader){
        /* and now leave boot loader: */
        buffer.info.reportId = 1;
        usbSetReport(dev, USB_HID_REPORT_TYPE_FEATURE, buffer.bytes, sizeof(buffer.info));
        /* Ignore errors here. If the device reboots before we poll the response,
         * this request fails.
         */
    }
errorOccurred:
    if(dev != NULL)
        usbCloseDevice(dev);
    return err;
}

/* ------------------------------------------------------------------------- */

static void printUsage(char *pname)
{
    fprintf(stderr, "usage: %s [-r] [<intel-hexfile>]\n", pname);
}

int main(int argc, char **argv)
{
char    *file = NULL;

    if(argc < 2){
        printUsage(argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
        printUsage(argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "-r") == 0){
        leaveBootLoader = 1;
        if(argc >= 3){
            file = argv[2];
        }
    }else{
        file = argv[1];
    }
    startAddress = sizeof(dataBuffer);
    endAddress = 0;
    if(file != NULL){   // an upload file was given, load the data
        memset(dataBuffer, -1, sizeof(dataBuffer));
        if(parseIntelHex(file, dataBuffer, &startAddress, &endAddress))
            return 1;
        if(startAddress >= endAddress){
            fprintf(stderr, "No data in input file, exiting.\n");
            return 0;
        }
    }
    // if no file was given, endAddress is less than startAddress and no data is uploaded
    if(uploadData(dataBuffer, startAddress, endAddress))
        return 1;
    return 0;
}

/* ------------------------------------------------------------------------- */


