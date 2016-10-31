/*
 * iATag.c
 *
 *  Created on: Oct 31, 2016
 *      Author: hmerchan
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "iATag.h"

#define iATag_isReqaWupa(buf, len)         (((len) == 1) && (((buf)[0] == CMD_REQA) || ((buf)[0] == CMD_WUPA)))
#define iATag_isLiteSelect(buf)        ((buf)[1] == 0x70)
#define iATag_isLiteSdd(buf)           (((buf)[0] == iATag_Cfg.SDDRsp[iATag_Cfg.cascadeLevel][0]))
#define iATag_is1stSdd(buf)            (((buf)[0] == CCD_L1) && ((buf)[1] == 0x20))

#define iATag_isSDD_SEL_CMD(cmd)       ((cmd) == 0x93 || (cmd) == 0x95 || (cmd) == 0x97)

typedef struct
{
	iATag_RSPSET_t	Rsp; 				// Response set recieved as input during setu[
	uint8_t			state;				// state machine state
	uint8_t			rxLen;				// input command length
	uint8_t*		buf;				// input command buffer
	uint8_t   		cascadeLevel;		// current cascade level
	uint8_t   		cascadeLevelMax;	// max cascade level
	uint8_t			SDDRsp[3][7];		//SDD responses for cascade levels 1-3
}iATag_CFG_t;

iATag_CFG_t  iATag_Cfg;  // Bloabl structure for state machine


/*****************************************************************
* Function: _iATag_expandUid()
*
* Abstract: expand UID to the following format:
*               if the UID size is 4, the expanded UID will become
*                         UID0, UID1, UID2, UID3
*               if the UID size is 7, the expanded UID will become
*                         CT,   UID0, UID1, UID2,
*                         UID3, UID4, UID5, UID6
*               if the UID size is 10, the expanded UID will become
*                         CT,   UID0, UID1, UID2,
*                         CT,   UID3, UID4, UID5,
*                         UID6, UID7, UID8, UID9
*
* Input/Output:
*
* Return:
*    None
*
*
*****************************************************************/

void _iATag_expandUid(uint8_t* uid)
{
    uint32_t  i;
    uint32_t  idx = 0;

    for (i = 0; i <= iATag_Cfg.cascadeLevelMax; ++i)
    {
        uint32_t j = 2;
        uint32_t bcc = 0;
        iATag_Cfg.SDDRsp[i][0] = 0x93 + (i << 1);
        iATag_Cfg.SDDRsp[i][1] = 0x70;

        if (i < iATag_Cfg.cascadeLevelMax)
        {
            iATag_Cfg.SDDRsp[i][j ++] = CCD_CT;
            bcc ^= CCD_CT;
        }

        for (; j < 6; ++j)
        {
            uint32_t data = uid[idx ++];
            iATag_Cfg.SDDRsp[i][j] = data;
            bcc ^= data;
        }
        iATag_Cfg.SDDRsp[i][j] = bcc;
    }
}

/*****************************************************************
* Function: _iATag_decodeAC()
*
* Abstract: Decode ISO14443 A AC commands
* Return:
*
* Description:
*
*****************************************************************/
uint8_t _iATag_decodeAC(void)
{
    uint8_t * localSdd = &(iATag_Cfg.SDDRsp[iATag_Cfg.cascadeLevel][0]);

    // check if it is final Select command
    if (iATag_Cfg.rxLen == 7 && !memcmp(iATag_Cfg.buf, localSdd, 7))
    {
        return AC_FRAME_SEL;
    }

    // else check for potential SDD commands
    if (iATag_Cfg.buf[0] == localSdd[0])
    {
    	uint8_t validBytes = iATag_Cfg.buf[1] >> 4;
    	uint8_t validBits = iATag_Cfg.buf[1] & 0x0F;

        if (validBytes >= 2 && validBytes < 7)
        {
            uint8_t byteCnt = validBytes + (validBits ? 1 : 0);
            if (byteCnt == iATag_Cfg.rxLen && (!memcmp(iATag_Cfg.buf + 2, localSdd + 2, validBytes - 2)))
            {
                if (validBits)
                {
                    if (0xFF & ((localSdd[validBytes] ^ iATag_Cfg.buf[iATag_Cfg.rxLen - 1]) << (8 - validBits)))
                    {
                        return AC_FRAME_INVALID;
                    }
                }
                //return ((7 - validBytes) << 4) | (8 - validBits);
                return AC_FRAME_SDD;
            }
        }
    }

    return AC_FRAME_INVALID;
}


/*****************************************************************
* Function: void iATag_setup(iATag_RSPSET_t* Rsp)
*
* Abstract: Sets up the iATag_Cfg structure and initializes the state machine
*
* Input/Output:
*               Input:The Response set

*
* Return:
*
* Description:
*
*****************************************************************/
void iATag_setup(iATag_RSPSET_t* Rsp)
{
	uint8_t Atqa0;
	memset((void*)&iATag_Cfg,0,sizeof(iATag_Cfg));
	memcpy((void*)&iATag_Cfg.Rsp, Rsp,sizeof(iATag_RSPSET_t));
	Atqa0 = iATag_Cfg.Rsp.ATQA[0];
	iATag_Cfg.cascadeLevelMax = MIN(2, ((Atqa0 >> 6) & 0x03));
	_iATag_expandUid(iATag_Cfg.Rsp.UID);
}



/*****************************************************************
* Function: uint8_t iATag_procFrame(uint8_t* rx_buf,uint8_t len, uint8_t* tx_buf)
*
* Abstract: Process and input frame and generate output response
*
* Input/Output:
*               Input:
*               rx_buf ==> pointer to input frame
*               len ==> input frame length
*               tx_buf ==> putput response buffer
*                Return ==> tx response length
*
* Return:
*
* Description:
*
*****************************************************************/
uint8_t iATag_procFrame(uint8_t* rx_buf,uint8_t len, uint8_t* tx_buf)
{
	uint8_t retLen = 0;
	uint8_t ac_frame;
	uint8_t gotoIdle = 1;
	//TBD: Check CRC
	//TBD: Check transmission error in HW
	// We assume the frame is recieved correctly without errors

	iATag_Cfg.buf = rx_buf;
	iATag_Cfg.rxLen = len;

    switch (iATag_Cfg.state)
    {
    case ST_IDLE:
        if (iATag_isReqaWupa(iATag_Cfg.buf, iATag_Cfg.rxLen))
        {
            //Send ATQA response
        	iATag_Cfg.state = ST_READY;
        	iATag_Cfg.cascadeLevel = 0;
        	tx_buf[0] = iATag_Cfg.Rsp.ATQA[0];
        	tx_buf[1] = iATag_Cfg.Rsp.ATQA[1];
        	retLen = 2;
        	gotoIdle = 0;
        }
        break;
    case ST_READY:
        ac_frame = _iATag_decodeAC();

        // process SEL command
        if (ac_frame == AC_FRAME_SEL)
        {
            if (iATag_Cfg.cascadeLevelMax == iATag_Cfg.cascadeLevel)
            {
                //Now selected
            	tx_buf[0] = iATag_Cfg.Rsp.SAK;
            	retLen = 1;
            	iATag_Cfg.state = ST_ACTIVE;
            }
            else
            {
            	// mark UID not complete in SAK and increase ccd level
            	tx_buf[0] = iATag_Cfg.Rsp.SAK | 0x04;
            	retLen = 1;
                iATag_Cfg.cascadeLevel ++;
            }
            gotoIdle = 0;
        }
        // process SDD command
        else if (ac_frame == AC_FRAME_SDD)
        {
        	// from the beginning
            if (iATag_Cfg.buf[1] == 0x20)
            {
                uint8_t * tmpBuf;

                tmpBuf = iATag_Cfg.SDDRsp[iATag_Cfg.cascadeLevel] + 2;
                memcpy((void*)tx_buf, tmpBuf, 5);
                retLen = 5;
            }
            else
            {
            	uint8_t validByte;
                uint8_t* tmpBuf;

                validByte = ((iATag_Cfg.buf[1] >> 4) & 0xF);
                if(validByte <= 6)
                {
                    //Return 4 bytes of UID + BCC
                	tmpBuf = iATag_Cfg.SDDRsp[iATag_Cfg.cascadeLevel] + validByte;
                	retLen = 7 - validByte;
                    memcpy((void*)tx_buf, tmpBuf,retLen);
                }
            }
            gotoIdle = 0;
        }

        break;
    case ST_ACTIVE:
    	// fall through to default as currently
    	// this state machine cannot handle after SAK
    default:
    	break;

    }

    // if we recieved any command out of order
    // jump to IDLE state
    if(gotoIdle)
    {
    	iATag_Cfg.state = ST_IDLE;
    }

    return retLen;;
}



