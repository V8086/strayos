#ifndef CHADFS_FBLK_H
#define CHADFS_FBLK_H

#include "chadfs-typedefs.h"

#pragma pack(push, 1)
#define CHADFS_MAX_FILE_NAME							255U
#define CHADFS_FILE_ATTRIBUTE_DIRECTORY					0x01U
// #define CHADFS_FILE_ATTRIBUTE_LINK					0x02U
#define CHADFS_FILE_ATTRIBUTE_READABLE					0x04U
#define CHADFS_FILE_ATTRIBUTE_WRITEABLE					0x08U
#define CHADFS_FILE_ATTRIBUTE_HIDDEN					0x10U
/* CHADFS(32) file block */
typedef struct _chadfs32_fblk_t {
	uint8_t			name[CHADFS_MAX_FILE_NAME + 1];
	uint32_t		size;
	uint32_t		firstdblk;
	uint32_t		lastdblk;
	uint32_t		attributes;
	uint8_t			reserved[CHADFS_SECTOR_SIZE - 272];
} chadfs32_fblk_t;
#pragma pack(pop)

#endif