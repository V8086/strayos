#ifndef CHADFS_VBLK_H
#define CHADFS_VBLK_H

#include "chadfs-typedefs.h"

#pragma pack(push, 1)
#define CHADFS_MAX_VOLUME_NAME							31U
#define CHADFS_TOTAL_BLKS(__viblks)						((__viblks) * CHADFS_NUMOF_IBLK_ENTRIES)
#define CHADFS_FREE_BLKS(__viblks, __vfblks, __vdblks)	(CHADFS_TOTAL_BLKS(__viblks) - (__vfblks) - (__vdblks))
/* CHADFS(32) volume block */
typedef struct _chadfs32_vblk_t {
	uint8_t			name[CHADFS_MAX_VOLUME_NAME + 1];
	uint32_t		numiblks;
	uint32_t		numfblks;
	uint32_t		numdblks;
	uint32_t		nextvolume;

	uint8_t			reserved[CHADFS_SECTOR_SIZE - 48];
} chadfs32_vblk_t;
#pragma pack(pop)

#endif