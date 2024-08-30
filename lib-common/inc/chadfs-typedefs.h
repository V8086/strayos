#ifndef CHADFS_TYPEDEFS_H
#define CHADFS_TYPEDEFS_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "murmur.h"

#ifndef CHADFS_SECTOR_SIZE
#define CHADFS_SECTOR_SIZE								512U
#endif
#if CHADFS_SECTOR_SIZE % 512 != 0
#error CHADFS_SECTOR_SIZE % 512 != 0
#endif
#if CHADFS_SECTOR_SIZE < 512
#error CHADFS_SECTOR_SIZE < 512
#endif

/* Seed to generate a file ID by its path */
#define CHADFS_SEED										0xAB0BA777U

#define CHADFS_ALIGN_VALUE_UP(__v, __al)				(((__v) + (__al) - 1) & ~((__al) - 1))
#define CHADFS_ABS_INDEX(__iIDB, __iE)					(((__iIDB) * CHADFS_NUMOF_IBLK_ENTRIES) + (__iE))
#define CHADFS_IBLK_INDEX(__iabs)						((__iabs) / CHADFS_NUMOF_IBLK_ENTRIES)
#define CHADFS_IENTRY_INDEX(__iabs)						((__iabs) % CHADFS_NUMOF_IBLK_ENTRIES)

typedef enum _chadfs_status_t {
	CHADFS_STATUS_OK,
	CHADFS_STATUS_TOO_LONG_VOLUME_NAME,
	CHADFS_STATUS_TOO_LONG_FILE_NAME,
	CHADFS_STATUS_INVALID_MBLK,
	CHADFS_STATUS_VOLUME_ALREADY_EXISTS,
	CHADFS_STATUS_FILE_ALREADY_EXISTS,
	CHADFS_STATUS_ZERO_VOLUME_LEN,
	CHADFS_STATUS_INVALID_PATH,
	CHADFS_STATUS_VOLUME_NOT_FOUND,
	CHADFS_STATUS_FILE_NOT_FOUND,
	CHADFS_STATUS_NOT_ENOUGH_SPACE,
	CHADFS_STATUS_ZERO_DATA_LEN,
	CHADFS_STATUS_INVALID_OFFSET,
	CHADFS_STATUS_NOT_DIR,
} chadfs_status_t;


#define CHADFS_STATIC_SV(__str, __l)					{ (char*)(__str), (__l) }
/* CHADFS string view */
typedef struct _chadfs_sv_t {
	char*		s;										/* string */
	size_t		l;										/* length */
} chadfs_sv_t;

/* CHADFS(32) location */
typedef struct _chadfs32_loc_t {
	uint32_t	a;										/* address of sector */
	void*		d;										/* data (OPTIONAL)*/
} chadfs32_loc_t;

/* CHADFS(32) extended location */
typedef struct _chadfs32_eloc_t {
	uint32_t	a;										/* address of sector */
	void*		d;										/* data (OPTIONAL)*/
	uint32_t	i;										/* index */
} chadfs32_eloc_t;

/* CHADFS(32) directory itertator */
typedef struct _chadfs32_dirit_t {
	uint32_t	itbladdr;								/* id table address */
	uint32_t	dtbladdr;								/* data table address */
	uint32_t	idcurrent;								/* current chadfs_idata_t index */

	uint32_t	idirentry;								/* current dir entry index */
	uint32_t	direntries;								/* num of dir entries */
} chadfs32_dirit_t;

/*
	Must be implemented by programmer
*/
void chadfs32_write_sector(
	void* dev,
	uint32_t address,
	const void* sectordata
);

/*
	Must be implemented by programmer
*/
void chadfs32_read_sector(
	void* dev,
	uint32_t address,
	void* sectordata
);

#endif