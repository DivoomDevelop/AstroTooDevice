#ifndef _NFTL_TYPE_H
#define _NFTL_TYPE_H

#include "nftl_errno.h"

#define ECC_LIMIT               10
#define ERR_ECC               (-2)
#define ERR_TIMEOUT           (-3)
#define NFTL_YES 1
#define NFTL_NO 0

typedef enum {
	NFTL_SUCCESS		= 0,
	NFTL_FAILURE		= 1,
	NFTL_INVALID_PARTITION	= 2,
	NFTL_INVALID_ADDRESS	= 3,
	NFTL_FLUSH_ERROR	= 4,
	NFTL_PAGENOTFOUND	= 5,
	NFTL_NO_FREE_BLOCKS	= 6,
	NFTL_NO_INVALID_BLOCKS	= 7,
	NFTL_COMPLETE		= 8,
	NFTL_READ_FAIL		= 11,
	NFTL_WRITE_FAIL		= 12,
} _nftl_error;

#endif /*_NFTL_TYPE_H*/
