#ifndef CHADFS_IBLK_H
#define CHADFS_IBLK_H

#include "chadfs-typedefs.h"

#pragma pack(push, 1)
typedef struct _chadfs32_ifile_t {
	uint32_t		id;
	uint32_t		active;
} chadfs32_ifile_t;

typedef struct _chadfs32_idata_t {
	uint32_t		numbytes;
	uint32_t		nextdata;
} chadfs32_idata_t;

#define CHADFS_NUMOF_IBLK_ENTRIES						(CHADFS_SECTOR_SIZE >> 3)
/* CHADFS(32) id block */
typedef union _chadfs32_iblk_t {
	chadfs32_ifile_t	f[CHADFS_NUMOF_IBLK_ENTRIES];
	chadfs32_idata_t	d[CHADFS_NUMOF_IBLK_ENTRIES];
} chadfs32_iblk_t;
#pragma pack(pop)

#endif