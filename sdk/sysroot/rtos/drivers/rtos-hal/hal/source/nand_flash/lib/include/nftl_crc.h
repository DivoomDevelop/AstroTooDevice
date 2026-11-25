#ifndef __NFTL_CRC_H
#define __NFTL_CRC_H

#include "nftl_osal.h"

u16 nand_crc16(u16 crc, u8 const *buffer, u32 len);
uint32 crc_wrong(struct _nftl_zone *zone, u8 *mbuf, u8 *spare_data);

#endif
