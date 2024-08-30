#ifndef CHADFS_DIRENT_H
#define CHADFS_DIRENT_H

#include "chadfs-typedefs.h"

#pragma pack(push, 1)
#define CHADFS_NUMOF_DIR_DBLK_ENTRIES					(CHADFS_SECTOR_SIZE / sizeof(chadfs32_dirent_t))
/* CHADFS(32) dir entry */
typedef struct _chadfs32_dirent_t {
	uint32_t		id;
	uint32_t		index;
} chadfs32_dirent_t;
#pragma pack(pop)

#endif