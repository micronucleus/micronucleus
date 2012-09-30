/*
	Created: September 2012
	by ihsan Kehribar <ihsan@kehribar.me>
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.	
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "micronucleus_lib.h"
#include "littleWire_util.h"

/******************************************************************************
* Global definitions 
******************************************************************************/
unsigned char		dataBuffer[65536 + 256];    /* buffer for file data */
/*****************************************************************************/

/******************************************************************************
* Function prototypes
******************************************************************************/
static int parseIntelHex(char *hexfile, char* buffer, int *startAddr, int *endAddr); /* taken from bootloadHID example from obdev */
static int parseUntilColon(FILE *fp); /* taken from bootloadHID example from obdev */
static int parseHex(FILE *fp, int numDigits); /* taken from bootloadHID example from obdev */
/*****************************************************************************/

/******************************************************************************
* Main function!
******************************************************************************/
int main(int argc, char **argv)
{
	int res;
	char *file = NULL;
	int run = 0;
	micronucleus *myDevice = NULL;

	if(argc < 2)
	{
		printf("usage: %s [--run] [<intel-hexfile>]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	{
		printf("usage: %s [--run] [<intel-hexfile>]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if(strcmp(argv[1], "--run") == 0)
	{
    	run = 1;
		if(argc >= 3){
			file = argv[2];
		}
	}
	else
	{
		file = argv[1];
	}

	printf("> Please plug the device ... \n");
	printf("> Press CTRL+C to terminate the program.\n");
	
	while(myDevice == NULL)
	{
		myDevice = micronucleus_connect();
		delay(250);
	}

	printf("> Device is found!\n");
	
	delay(750);
	myDevice = micronucleus_connect();
	
	if(myDevice->page_size == 64)
	{
		printf("> Device looks like Attiny85!\n");
	}
	else if(myDevice->page_size == 32)
	{
		printf("> Device looks like Attiny45!\n");
	}
	else
	{
		printf("> Unsupported device!\n");
		return EXIT_FAILURE;
	}
	
	printf("> Available space for user application: %d bytes\n", myDevice->flash_size);
	printf("> Suggested sleep time between sending pages: %u milliseconds\n", myDevice->write_sleep);
	printf("> Whole page count: %d\n", myDevice->pages);
	printf("> Erase function sleep duration: %d milliseconds\n", myDevice->erase_sleep);
	
	memset(dataBuffer, 0xFF, sizeof(dataBuffer));
    
    int startAddress = 1, endAddress = 0;
	if (parseIntelHex(file, dataBuffer, &startAddress, &endAddress))
	{
    	printf("> Error parsing hex file.\n");
		return EXIT_FAILURE;
	}

	if(startAddress >= endAddress)
	{
		printf("> No data in input file, exiting.\n");
		return EXIT_FAILURE;
	}
	
	if(endAddress > myDevice->flash_size)
	{
		printf("> Program file is too big for the bootloader!\n");
		return EXIT_FAILURE;
	}
	
	/* Prints the decoded intel hex file */
	/*printf("> Decoded hex file starts ... \n");
	for(i=startAddress;i<endAddress;i+=16)
	{
		printf("%5d:\t",i);
		for(k=0;k<16;k++)
			printf("%2X ",dataBuffer[i+k]);
		printf("\n");
	}
	printf("> Decoded hex file ends ... \n");*/

	printf("> Erasing the memory ...\n");
	res = micronucleus_eraseFlash(myDevice);
	if(res!=0)
	{
		printf(">> Abort mission! An error has occured ...\n");
		printf(">> Please unplug the device and restart the program. \n");
		return EXIT_FAILURE;
	}
	
	printf("> Starting to upload ...\n");	
	res = micronucleus_writeFlash(myDevice,endAddress,dataBuffer);
	if(res!=0)
	{
		printf(">> Abort mission! An error has occured ...\n");
		printf(">> Please unplug the device and restart the program. \n");
		return EXIT_FAILURE;
	}
	
	if (run)
	{
    	printf("> Starting the user app ...\n");
    	res = micronucleus_startApp(myDevice);
    	if(res!=0)
    	{
    		printf(">> Abort mission! An error has occured ...\n");
    		printf(">> Please unplug the device and restart the program. \n");
    		return EXIT_FAILURE;
    	}
    }

	printf(">> Micronucleus done. Thank you!\n");

	return EXIT_SUCCESS;
}
/******************************************************************************/

/******************************************************************************/
static int  parseUntilColon(FILE *fp)
{
int c;

    do{
        c = getc(fp);
    }while(c != ':' && c != EOF);
    return c;
}
/******************************************************************************/

/******************************************************************************/
static int  parseHex(FILE *fp, int numDigits)
{
int     i;
char    temp[9];

    for(i = 0; i < numDigits; i++)
        temp[i] = getc(fp);
    temp[i] = 0;
    return strtol(temp, NULL, 16);
}
/******************************************************************************/

/******************************************************************************/
static int  parseIntelHex(char *hexfile, char* buffer, int *startAddr, int *endAddr)
{
int     address, base, d, segment, i, lineLen, sum;
FILE    *input;

    input = fopen(hexfile, "r");
    if(input == NULL){
        printf("> Error opening %s: %s\n", hexfile, strerror(errno));
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
            printf("> Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
        }
        if(*startAddr > base)
            *startAddr = base;
        if(*endAddr < address)
            *endAddr = address;
    }
    fclose(input);
    return 0;
}
/******************************************************************************/
