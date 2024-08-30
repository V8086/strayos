#ifndef CHADFS_H
#define CHADFS_H

#include "chadfs-mblk.h"
#include "chadfs-vblk.h"
#include "chadfs-iblk.h"
#include "chadfs-fblk.h"
#include "chadfs-dirent.h"

#ifdef __cplusplus
extern "C" {
#endif
/* ================================================= */
	const char* chadfs_status_to_str(
		chadfs_status_t status
	);

	uint8_t chadfs_get_bytesum(
		const void* data,
		size_t l
	);

	uint32_t chadfs_get_path_hash(
		const chadfs_sv_t* spath
	);

	bool chadfs_get_volume_name(
		const chadfs_sv_t* spath,
		chadfs_sv_t* sname
	);

	bool chadfs_get_parent_dir(
		const chadfs_sv_t* spath, 
		chadfs_sv_t* spardir
	);

	bool chadfs_get_file_name(
		const chadfs_sv_t* spath,
		chadfs_sv_t* sfname
	);

	bool chadfs_cmpsv_s(
		const chadfs_sv_t* stgt,
		const char* str
	);
/* ================================================= */
	bool chadfs32_check_mblk(
		chadfs32_mblk_t* mblk
	);

	void chadfs32_init_mblk(
		chadfs32_mblk_t* mblk
	);
	
	chadfs_status_t chadfs32_init_vblk(
		chadfs32_vblk_t* vblk,
		const chadfs_sv_t* sname,
		uint32_t numiblks
	);
	
	chadfs_status_t chadfs32_init_fblk(
		chadfs32_fblk_t* fblk,
		const chadfs_sv_t* sname,
		uint32_t size
	);
/* ================================================= */
	chadfs_status_t chadfs32_find_free_fblk(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		chadfs32_eloc_t* iblkeloc
	);

	chadfs_status_t chadfs32_find_free_dblk(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		chadfs32_eloc_t* iblkeloc
	);

	chadfs_status_t chadfs32_find_next_free_dblk(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		uint32_t iprev,
		chadfs32_eloc_t* iblkeloc
	);
/* ================================================= */
	chadfs_status_t chadfs32_read_mblk(
		void* dev,
		uint32_t address,
		chadfs32_mblk_t* mblk
	);

	chadfs_status_t chadfs32_read_vblk(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* sname,
		chadfs32_vblk_t* vblk,
		chadfs32_eloc_t* vblkeloc
	);

	chadfs_status_t chadfs32_read_fblk(
		void* dev, 
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		chadfs32_fblk_t* fblk,
		chadfs32_eloc_t* fblkeloc,
		chadfs32_vblk_t* vblk,
		chadfs32_eloc_t* vblkeloc
	);
/* ================================================= */
	chadfs_status_t chadfs32_write_data(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		const void* data,
		uint32_t len,
		chadfs32_eloc_t* firstieloc,
		chadfs32_eloc_t* lastieloc
	);

	chadfs_status_t chadfs32_read_data(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		uint32_t ifirstidblk,
		void* buffer,
		uint32_t offset,
		uint32_t len
	);

	chadfs_status_t chadfs32_cut_data(
		void* dev,
		const chadfs32_loc_t* vblkloc,
		uint32_t ifirstidblk,
		uint32_t offset,
		chadfs32_eloc_t* lastidblkeloc
	);
/* ================================================= */
	chadfs_status_t chadfs32_create_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		uint32_t attributes,
		const void* data,
		uint32_t len
	);

	chadfs_status_t chadfs32_create_dir(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		uint32_t attributes
	);

	chadfs_status_t chadfs32_read_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		void* buffer,
		uint32_t offset,
		uint32_t len
	);

	chadfs_status_t chadfs32_append_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		const void* data,
		uint32_t len
	);

	chadfs_status_t chadfs32_trunc_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		uint32_t len
	);

	chadfs_status_t chadfs32_remove_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath
	);

	chadfs_status_t chadfs32_write_file(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		const void* data,
		uint32_t offset,
		uint32_t len
	);
/* ================================================= */
	chadfs_status_t chadfs32_add_volume(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs32_vblk_t* vblk
	);
/* ================================================= */
	chadfs_status_t chadfs32_create_iter(
		void* dev,
		const chadfs32_loc_t* mblkloc,
		const chadfs_sv_t* spath,
		chadfs32_dirit_t* iter,
		chadfs32_fblk_t* firstfblk
	);

	chadfs_status_t chadfs32_move_iter(
		void* dev,
		chadfs32_dirit_t* iter,
		chadfs32_fblk_t* fblk
	);
/* ================================================= */
#ifdef __cplusplus
}
#endif

#endif