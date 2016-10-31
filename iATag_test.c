/*
 ============================================================================
 Name        : iATag_test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "iATag.h"


void printRsp(uint8_t* buf, uint8_t len)
{
	uint8_t i = 0;
	printf("\n Got Rsp. Len = %d. RSP: ",len);
	while(i < len)
	{
		printf("0x%x ",buf[i]);
		i++;
	}
}
int main(void) {
	iATag_RSPSET_t iATagRsp;
	uint8_t Cmd[10];
	uint8_t rsp[10];
	uint8_t rspLen = 0;

	// setup a standard ISO14443 A tag
	iATagRsp.ATQA[0] = 0x04;
	iATagRsp.ATQA[1] = 0x00;
	iATagRsp.UID[0] = 0x08;
	iATagRsp.UID[1] = 0x01;
	iATagRsp.UID[2] = 0x02;
	iATagRsp.UID[3] = 0x03;
	iATagRsp.SAK = 0x20;

	// setup the tag
	iATag_setup(&iATagRsp);

	printf("\n Sending WUPA");
	Cmd[0] = 0x26;
	rspLen = iATag_procFrame(Cmd,1,rsp);
	printRsp(rsp,rspLen);

	printf("\n Sending SDD REQ");
	Cmd[0] = 0x93;
	Cmd[1] = 0x20;
	rspLen = iATag_procFrame(Cmd,2,rsp);
	printRsp(rsp,rspLen);

	printf("\n Sending SEL REQ");
	Cmd[0] = 0x93;
	Cmd[1] = 0x70;
	memcpy(Cmd + 2,rsp,rspLen);
	rspLen = iATag_procFrame(Cmd,7,rsp);
	printRsp(rsp,rspLen);

	return EXIT_SUCCESS;
}
