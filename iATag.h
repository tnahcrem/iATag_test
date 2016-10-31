/*
 * iATag.h
 *
 *  Created on: Oct 31, 2016
 *      Author: hmerchan
 */

#ifndef IATAG_H_
#define IATAG_H_


typedef struct
{
    uint8_t		ATQA[2];
    uint8_t  	UID[10];
    uint8_t		SAK;
}__attribute__((packed)) iATag_RSPSET_t;


typedef enum
{
	ST_IDLE = 0,
	ST_READY,
	ST_ACTIVE
}iATag_STATE_e;

typedef enum
{
    CCD_L1     = 0x93,
    CCD_L2     = 0x95,
    CCD_L3     = 0x97,
	CCD_CT	   = 0x88,
}CCD_TYP_e;

typedef enum
{
	CMD_REQA = 0x26,
	CMD_WUPA = 0x52,
}CMD_TYP_e;

typedef enum
{
	AC_FRAME_SEL = 0,
	AC_FRAME_SDD = 1,
	AC_FRAME_INVALID = 0xFF
}AC_FRAME_e;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void iATag_setup(iATag_RSPSET_t* Rsp);
uint8_t iATag_procFrame(uint8_t* rx_buf,uint8_t len, uint8_t* tx_buf);

#endif /* IATAG_H_ */
