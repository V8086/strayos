#ifndef CHADFS_MBLK_H
#define CHADFS_MBLK_H

#include "chadfs-typedefs.h"

#pragma pack(push, 1)
#define CHADFS_SIGNATURE								"CHADFS  "
#define CHADFS_VERSION32								32U
/* CHADFS(32) main block */
typedef struct _chadfs32_mblk_t {
	uint8_t			signature[8];						/* CHADFS_SIGNATURE */
	uint8_t			version;
	uint8_t			csum;

	uint32_t		numvolumes;
	uint32_t		firstvolume;

	uint8_t			reserved[CHADFS_SECTOR_SIZE - 18];
} chadfs32_mblk_t;
#pragma pack(pop)

#endif