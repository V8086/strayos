#include <chadfs.h>

/* ================================================= */
static const char* CHADFS_STATUS_STRS[] = {
	"OK",
	"TOO LONG VOLUME NAME",
	"TOO LONG FILE NAME",
	"INVALID MAIN BLOCK",
	"VOLUME ALREADY EXISTS",
	"FILE ALREADY EXISTS",
	"ZERO VOLUME LENGTH",
	"INVALID PATH",
	"VOLUME NOT FOUND",
	"FILE NOT FOUND",
	"NOT ENOUGH SPACE",
	"ZERO DATA LENGTH",
	"INVALID OFFSET",
};

/* ================================================= */

/*
	Get a pointer to a string representing the status
*/
const char* chadfs_status_to_str(
	chadfs_status_t status
) {
	const size_t numstrs = sizeof(CHADFS_STATUS_STRS) / sizeof(CHADFS_STATUS_STRS[0]);
	if ((size_t)status >= numstrs) return NULL;
	return CHADFS_STATUS_STRS[(size_t)status];
}

uint8_t chadfs_get_bytesum(
	const void* data,
	size_t l
) {
	uint8_t res = 0;
	for (size_t i = 0; i < l; ++i) res += ((uint8_t*)data)[i];
	return res;
}

uint32_t chadfs_get_path_hash(
	const chadfs_sv_t* spath
) {
	return Murmur3Dword((uint8_t*)spath->s, spath->l, CHADFS_SEED);
}

/*
	Get volume name from path
*/
bool chadfs_get_volume_name(
	const chadfs_sv_t* spath,
	chadfs_sv_t* sname
) {
	if (!spath->l) return false;

	sname->s = spath->s;
	for (size_t i = 0; i < spath->l; ++i) if (spath->s[i] == '/') {
		sname->l = i;
		return i != 0;
	}

	sname->l = spath->l;
	return true;
}


/*
	Get parent dir path
*/
bool chadfs_get_parent_dir(
	const chadfs_sv_t* spath, 
	chadfs_sv_t* spardir
) {
	if (!spath->l) return false;

	spardir->s = spath->s;
	for (size_t i = spath->l - 1; i < spath->l; --i) {
		if (spath->s[i] == '/') {
			spardir->l = i;
			return i;
		}
	}

	spardir->l = spath->l;
	return true;
}


/*
	Get file name from path
*/
bool chadfs_get_file_name(
	const chadfs_sv_t* spath,
	chadfs_sv_t* sfname
) {
	if (!spath->l) return false;

	for (size_t i = spath->l - 1; i < spath->l; --i) {
		if (spath->s[i] == '/') {
			sfname->s = spath->s + i + 1;
			sfname->l = spath->l - i - 1;
			return i && sfname->l;
		}
	}

	sfname->s = spath->s;
	sfname->l = spath->l;
	return true;
}

/*
	Compare string view with C string
*/
bool chadfs_cmpsv_s(
	const chadfs_sv_t* stgt,
	const char* str
) {
	size_t strl = strlen(str);
	return strl == stgt->l && !memcmp(stgt->s, str, strl);
}

/* ================================================= */

/*
	Validate chadfs32_mblk_t
*/
bool chadfs32_check_mblk(
	chadfs32_mblk_t* mblk
) {
	if (memcmp(mblk->signature, CHADFS_SIGNATURE, sizeof(mblk->signature))) return false;
	return mblk->version == CHADFS_VERSION32 && !chadfs_get_bytesum(mblk, 10);
}

void chadfs32_init_mblk(
	chadfs32_mblk_t* mblk
) {
	memset(mblk, 0, sizeof(*mblk));
	memcpy(mblk->signature, CHADFS_SIGNATURE, sizeof(mblk->signature));
	mblk->version = CHADFS_VERSION32;
	
	uint8_t csval = 0;
	for (size_t i = 0; i < 9; ++i) csval += ((uint8_t*)mblk)[i];
	mblk->csum = (uint8_t)(-csval);
}

chadfs_status_t chadfs32_init_vblk(
	chadfs32_vblk_t* vblk,
	const chadfs_sv_t* sname,
	uint32_t numiblks
) {
	if (sname->l > CHADFS_MAX_VOLUME_NAME) return CHADFS_STATUS_TOO_LONG_VOLUME_NAME;

	memset(vblk, 0, sizeof(*vblk));
	memcpy(vblk->name, sname->s, sname->l);
	vblk->numiblks = numiblks;

	return CHADFS_STATUS_OK;
}

chadfs_status_t chadfs32_init_fblk(
	chadfs32_fblk_t* fblk,
	const chadfs_sv_t* sname,
	uint32_t size
) {
	if (sname->l > CHADFS_MAX_VOLUME_NAME) return CHADFS_STATUS_TOO_LONG_FILE_NAME;

	memset(fblk, 0, sizeof(*fblk));
	memcpy(fblk->name, sname->s, sname->l);
	fblk->size = size;

	return CHADFS_STATUS_OK;
}

/* ================================================= */

/*
	Find the first free cell in the ID table for a file
*/
chadfs_status_t chadfs32_find_free_fblk(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	chadfs32_eloc_t* iblkeloc
) {
	chadfs32_vblk_t* vblk = (chadfs32_vblk_t*)vblkloc->d;
	uint32_t itaddr = vblkloc->a + 1;

	chadfs32_iblk_t iblk;
	for (uint32_t i = 0; i < vblk->numiblks; ++i) {
		chadfs32_read_sector(dev, itaddr + i, &iblk);
		for (uint32_t j = 0; j < CHADFS_NUMOF_IBLK_ENTRIES; ++j) {
			if (!iblk.f[j].active) {
				if (iblkeloc) {
					iblkeloc->a = itaddr + i;
					iblkeloc->d = NULL;
					iblkeloc->i = CHADFS_ABS_INDEX(i, j);
				}

				return CHADFS_STATUS_OK;
			}
		}
	}

	return CHADFS_STATUS_NOT_ENOUGH_SPACE;
}

/*
	Find the first free cell in the ID table for data
*/
chadfs_status_t chadfs32_find_free_dblk(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	chadfs32_eloc_t* iblkeloc
) {
	chadfs32_vblk_t* vblk = (chadfs32_vblk_t*)vblkloc->d;
	uint32_t itaddr = vblkloc->a + 1;

	chadfs32_iblk_t iblk;
	for (uint32_t i = vblk->numiblks - 1; i < vblk->numiblks; --i) {
		chadfs32_read_sector(dev, itaddr + i, &iblk);
		for (uint32_t j = CHADFS_NUMOF_IBLK_ENTRIES - 1; j < CHADFS_NUMOF_IBLK_ENTRIES; --j) {
			if (!iblk.d[j].numbytes) {
				if (iblkeloc) {
					iblkeloc->a = itaddr + i;
					iblkeloc->d = NULL;
					iblkeloc->i = CHADFS_ABS_INDEX(i, j);
				}

				return CHADFS_STATUS_OK;
			}
		}
	}

	return CHADFS_STATUS_NOT_ENOUGH_SPACE;
}

/*
	Find the next free cell in the ID table for data
*/
chadfs_status_t chadfs32_find_next_free_dblk(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	uint32_t iprev,
	chadfs32_eloc_t* iblkeloc
) {
	if (!iprev) return CHADFS_STATUS_NOT_ENOUGH_SPACE;

	chadfs32_vblk_t* vblk = (chadfs32_vblk_t*)vblkloc->d;
	uint32_t itaddr = vblkloc->a + 1;
	iprev -= 1;

	chadfs32_iblk_t iblk;
	uint32_t i = CHADFS_IBLK_INDEX(iprev);
	uint32_t j = CHADFS_IENTRY_INDEX(iprev);
	for (; i < vblk->numiblks; --i) {
		chadfs32_read_sector(dev, itaddr + i, &iblk);
		for (; j < CHADFS_NUMOF_IBLK_ENTRIES; --j) {
			if (!iblk.d[j].numbytes) {
				if (iblkeloc) {
					iblkeloc->a = itaddr + i;
					iblkeloc->d = NULL;
					iblkeloc->i = CHADFS_ABS_INDEX(i, j);
				}

				return CHADFS_STATUS_OK;
			}
		}

		j = CHADFS_NUMOF_IBLK_ENTRIES - 1;
	}

	return CHADFS_STATUS_NOT_ENOUGH_SPACE;
}

/* ================================================= */

/*
	Read and validate chadfs32_mblk_t
*/
chadfs_status_t chadfs32_read_mblk(
	void* dev,
	uint32_t address,
	chadfs32_mblk_t* mblk
) {
	chadfs32_read_sector(dev, address, mblk);
	if (!chadfs32_check_mblk(mblk)) return CHADFS_STATUS_INVALID_MBLK;

	return CHADFS_STATUS_OK;
}

/*
	Find the volume and read its block
*/
chadfs_status_t chadfs32_read_vblk(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* sname,
	chadfs32_vblk_t* vblk,
	chadfs32_eloc_t* vblkeloc
) {
	chadfs32_mblk_t* mblk = (chadfs32_mblk_t*)mblkloc->d;
	
	chadfs32_vblk_t tmpvblk;
	uint32_t caddr = mblkloc->a + mblk->firstvolume;
	for (uint32_t i = 0; i < mblk->numvolumes; ++i) {
		chadfs32_read_sector(dev, caddr, &tmpvblk);
		if (chadfs_cmpsv_s(sname, (char*)tmpvblk.name)) {
			if (vblk) memcpy(vblk, &tmpvblk, sizeof(*vblk));
			if (vblkeloc) {
				vblkeloc->a = caddr;
				vblkeloc->i = i;
				vblkeloc->d = vblk;
			}

			return CHADFS_STATUS_OK;
		}

		caddr += tmpvblk.nextvolume;
	}

	return CHADFS_STATUS_VOLUME_NOT_FOUND;
}

/*
	Find a file and read its block
*/
chadfs_status_t chadfs32_read_fblk(
	void* dev, 
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	chadfs32_fblk_t* fblk,
	chadfs32_eloc_t* fblkeloc,
	chadfs32_vblk_t* vblk,
	chadfs32_eloc_t* vblkeloc
) {
	chadfs_status_t status;
	uint32_t fileid = chadfs_get_path_hash(spath);
	
	chadfs_sv_t svvolname;
	chadfs_sv_t svfname;
	if (
		!chadfs_get_volume_name(spath, &svvolname) ||
		!chadfs_get_file_name(spath, &svfname)
	) return CHADFS_STATUS_INVALID_PATH;

	chadfs32_vblk_t tmpvblk;
	chadfs32_eloc_t tmpvblkeloc;
	status = chadfs32_read_vblk(dev, mblkloc, &svvolname, &tmpvblk, &tmpvblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	if (vblk) memcpy(vblk, &tmpvblk, sizeof(*vblk));
	if (vblkeloc) memcpy(vblkeloc, &tmpvblkeloc, sizeof(*vblkeloc));

	chadfs32_iblk_t tmpiblk;
	chadfs32_fblk_t tmpfblk;
	uint32_t saddr = tmpvblkeloc.a + 1;
	uint32_t taddr = saddr + tmpvblk.numiblks;
	for (uint32_t i = 0; i < tmpvblk.numiblks; ++i) {
		chadfs32_read_sector(dev, saddr + i, &tmpiblk);
		for (uint32_t j = 0; j < CHADFS_NUMOF_IBLK_ENTRIES; ++j) {
			if (tmpiblk.f[j].id == fileid) {
				chadfs32_read_sector(dev, taddr + CHADFS_ABS_INDEX(i, j), &tmpfblk);
				if (chadfs_cmpsv_s(&svfname, (char*)tmpfblk.name)) {
					if (fblk) memcpy(fblk, &tmpfblk, sizeof(*fblk));
					if (fblkeloc) {
						fblkeloc->i = CHADFS_ABS_INDEX(i, j);
						fblkeloc->d = fblk;
						fblkeloc->a = taddr + fblkeloc->i;
					}

					return CHADFS_STATUS_OK;
				}
			}
		}
	}

	return CHADFS_STATUS_FILE_NOT_FOUND;
}

/* ================================================= */

/*
	Write new data
*/
chadfs_status_t chadfs32_write_data(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	const void* data,
	uint32_t len,
	chadfs32_eloc_t* firstieloc,
	chadfs32_eloc_t* lastieloc
) {
	chadfs_status_t status;
	chadfs32_vblk_t* vblk = (chadfs32_vblk_t*)vblkloc->d;
	const uint32_t neededblks = CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	const uint32_t freeblks = CHADFS_FREE_BLKS(vblk->numiblks, vblk->numfblks, vblk->numdblks);
	if (freeblks < neededblks) return CHADFS_STATUS_NOT_ENOUGH_SPACE;

	uint32_t itaddr = vblkloc->a + 1;
	uint32_t dtaddr = itaddr + vblk->numiblks;

	chadfs32_eloc_t icurblkeloc;
	chadfs32_eloc_t inxtblkeloc;
	status = chadfs32_find_free_dblk(dev, vblkloc, &icurblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	if (firstieloc) memcpy(firstieloc, &icurblkeloc, sizeof(*firstieloc));
	
	uint32_t iiblk;
	uint32_t iientry;
	uint32_t addedbytes;
	chadfs32_iblk_t iblk;
	uint8_t tmp[CHADFS_SECTOR_SIZE];
	for (uint32_t i = 0; i < neededblks && len; ++i) {
		iiblk = CHADFS_IBLK_INDEX(icurblkeloc.i);
		iientry = CHADFS_IENTRY_INDEX(icurblkeloc.i);
		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);

		if (len > CHADFS_SECTOR_SIZE) addedbytes = CHADFS_SECTOR_SIZE;
		else addedbytes = len;
		iblk.d[iientry].numbytes = addedbytes;

		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, data, addedbytes);
		chadfs32_write_sector(dev, dtaddr + CHADFS_ABS_INDEX(iiblk, iientry), tmp);
		data = (void*)((size_t)data + addedbytes);
		len -= addedbytes;

		if (!len) {
			iblk.d[iientry].nextdata = 0;	/* (uint32_t)icurblkeloc.i; */
			chadfs32_write_sector(dev, itaddr + iiblk, &iblk);
			if (lastieloc) memcpy(lastieloc, &icurblkeloc, sizeof(*lastieloc));
			return CHADFS_STATUS_OK;
		}

		status = chadfs32_find_next_free_dblk(dev, vblkloc, icurblkeloc.i, &inxtblkeloc);
		if (status != CHADFS_STATUS_OK) return status;

		iblk.d[iientry].nextdata = inxtblkeloc.i;
		chadfs32_write_sector(dev, itaddr + iiblk, &iblk);
		memcpy(&icurblkeloc, &inxtblkeloc, sizeof(icurblkeloc));
	}

	return CHADFS_STATUS_ZERO_DATA_LEN;
}

chadfs_status_t chadfs32_read_data(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	uint32_t ifirstidblk,
	void* buffer,
	uint32_t offset,
	uint32_t len
) {
	chadfs32_vblk_t* vblk = (chadfs32_vblk_t*)vblkloc->d;
	const uint32_t numientries = vblk->numiblks * CHADFS_NUMOF_IBLK_ENTRIES;
	if (ifirstidblk >= numientries) return CHADFS_STATUS_INVALID_OFFSET;

	uint32_t itaddr = vblkloc->a + 1;
	uint32_t dtaddr = itaddr + vblk->numiblks;

	uint32_t iiblk;
	uint32_t iientry;
	chadfs32_iblk_t iblk;
	uint8_t tmp[CHADFS_SECTOR_SIZE];
	uint32_t byteoffset = offset % CHADFS_SECTOR_SIZE;
	uint32_t sectorindex = offset / CHADFS_SECTOR_SIZE;
	if (sectorindex || byteoffset) {
		for (uint32_t i = 0; i < sectorindex; ++i) {
			iiblk = CHADFS_IBLK_INDEX(ifirstidblk);
			iientry = CHADFS_IENTRY_INDEX(ifirstidblk);
			chadfs32_read_sector(dev, itaddr + iiblk, &iblk);

			ifirstidblk = iblk.d[iientry].nextdata;
		}

		iiblk = CHADFS_IBLK_INDEX(ifirstidblk);
		iientry = CHADFS_IENTRY_INDEX(ifirstidblk);
		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
		chadfs32_read_sector(dev, dtaddr + ifirstidblk, tmp);
		if (byteoffset + len <= CHADFS_SECTOR_SIZE) {
			memcpy(buffer, &tmp[byteoffset], len);
			return CHADFS_STATUS_OK;
		}

		uint32_t addedbytes = iblk.d[iientry].numbytes - byteoffset;
		memcpy(buffer, &tmp[byteoffset], addedbytes);
		buffer = (void*)((size_t)buffer + addedbytes);
		len -= addedbytes;
		ifirstidblk = iblk.d[iientry].nextdata;
	}

	const uint32_t neededblks = CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	for (uint32_t i = 0; i < neededblks && len; ++i) {
		iiblk = CHADFS_IBLK_INDEX(ifirstidblk);
		iientry = CHADFS_IENTRY_INDEX(ifirstidblk);
		chadfs32_read_sector(dev, dtaddr + ifirstidblk, tmp);

		if (len <= CHADFS_SECTOR_SIZE) {
			memcpy(buffer, tmp, len);
			return CHADFS_STATUS_OK;
		}

		memcpy(buffer, tmp, CHADFS_SECTOR_SIZE);
		buffer = (void*)((size_t)buffer + CHADFS_SECTOR_SIZE);
		len -= CHADFS_SECTOR_SIZE;

		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
		ifirstidblk = iblk.d[iientry].nextdata;
	}

	return CHADFS_STATUS_INVALID_OFFSET;
}

chadfs_status_t chadfs32_cut_data(
	void* dev,
	const chadfs32_loc_t* vblkloc,
	uint32_t ifirstidblk,
	uint32_t offset,
	chadfs32_eloc_t* lastidblkeloc
) {
	uint32_t itaddr = vblkloc->a + 1;

	chadfs32_iblk_t iblk;
	uint32_t iiblk = CHADFS_IBLK_INDEX(ifirstidblk);
	uint32_t iientry = CHADFS_IENTRY_INDEX(ifirstidblk);
	uint32_t leftinlast = offset % CHADFS_SECTOR_SIZE;
	uint32_t leftfullsectors = offset / CHADFS_SECTOR_SIZE;
	if (leftfullsectors) {
		if (!leftinlast) {
			leftinlast = CHADFS_SECTOR_SIZE;
			leftfullsectors -= 1;
		}

		for (size_t i = 0; i < leftfullsectors; ++i) {
			chadfs32_read_sector(dev, itaddr + iiblk, &iblk);

			ifirstidblk = iblk.d[iiblk].nextdata;
			iiblk = CHADFS_IBLK_INDEX(ifirstidblk);
			iientry = CHADFS_IENTRY_INDEX(ifirstidblk);
		}
	}

	uint32_t icurdblk;
	if (offset) {
		if (lastidblkeloc) {
			lastidblkeloc->a = itaddr + iiblk;
			lastidblkeloc->d = NULL;
			lastidblkeloc->i = ifirstidblk;
		}

		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
		icurdblk = iblk.d[iientry].nextdata;
		iblk.d[iientry].numbytes = leftinlast;
		iblk.d[iientry].nextdata = 0;
		chadfs32_write_sector(dev, itaddr + iiblk, &iblk);

		iiblk = CHADFS_IBLK_INDEX(icurdblk);
		iientry = CHADFS_IENTRY_INDEX(icurdblk);
	}
	else {
		if (lastidblkeloc) memset(lastidblkeloc, 0, sizeof(*lastidblkeloc));
		icurdblk = ifirstidblk;
	}

	if (icurdblk) {
		do {
			chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
			icurdblk = iblk.d[iientry].nextdata;
			memset(&iblk.d[iientry], 0, sizeof(iblk.d[iientry]));
			chadfs32_write_sector(dev, itaddr + iiblk, &iblk);

			iiblk = CHADFS_IBLK_INDEX(icurdblk);
			iientry = CHADFS_IENTRY_INDEX(icurdblk);
		} while (iblk.d[iientry].nextdata);
	}

	return CHADFS_STATUS_OK;
}

/* ================================================= */

/*
	Create new file
*/
chadfs_status_t chadfs32_create_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	uint32_t attributes,
	const void* data,
	uint32_t len
) {
	chadfs_status_t status;
	status = chadfs32_read_fblk(dev, mblkloc, spath, NULL, NULL, NULL, NULL);
	if (status == CHADFS_STATUS_OK) return CHADFS_STATUS_FILE_ALREADY_EXISTS;

	chadfs_sv_t svvolname;
	chadfs_sv_t svfilename;
	chadfs_sv_t svpardir;
	if (
		!chadfs_get_volume_name(spath, &svvolname) ||
		!chadfs_get_file_name(spath, &svfilename) ||
		!chadfs_get_parent_dir(spath, &svpardir)
	) return CHADFS_STATUS_INVALID_PATH;

	uint32_t fileid = chadfs_get_path_hash(spath);

	chadfs32_vblk_t vblk;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_vblk(dev, mblkloc, &svvolname, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	const uint32_t neededblks = 1 + CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	const uint32_t freeblks = vblk.numiblks * CHADFS_NUMOF_IBLK_ENTRIES - vblk.numfblks - vblk.numdblks;
	if (freeblks < neededblks) return CHADFS_STATUS_NOT_ENOUGH_SPACE;

	uint32_t itaddr = vblkeloc.a + 1;
	uint32_t dtaddr = itaddr + vblk.numiblks;

	chadfs32_eloc_t ifileblkeloc;
	status = chadfs32_find_free_fblk(dev, (chadfs32_loc_t*)&vblkeloc, &ifileblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	uint32_t iientry = CHADFS_IENTRY_INDEX(ifileblkeloc.i);

	chadfs32_fblk_t fblk;
	status = chadfs32_init_fblk(&fblk, &svfilename, len);
	if (status != CHADFS_STATUS_OK) return status;

	chadfs32_iblk_t iblk;
	chadfs32_read_sector(dev, ifileblkeloc.a, &iblk);

	iblk.f[iientry].id = fileid;
	iblk.f[iientry].active = 1;
	chadfs32_write_sector(dev, ifileblkeloc.a, &iblk);

	chadfs32_eloc_t lastieloc;
	chadfs32_eloc_t firstieloc;
	if (data && len) {
		status = chadfs32_write_data(dev, (chadfs32_loc_t*)&vblkeloc, data, len, &firstieloc, &lastieloc);
		if (status != CHADFS_STATUS_OK) return status;

		fblk.size = len;
		fblk.firstdblk = firstieloc.i;
		fblk.lastdblk = lastieloc.i;
	}
	else {
		fblk.size = 0;
		fblk.firstdblk = 0;
		fblk.lastdblk = 0;
	}

	fblk.attributes = attributes;
	chadfs32_write_sector(dev, dtaddr + ifileblkeloc.i, &fblk);

	vblk.numfblks += 1;
	vblk.numdblks += neededblks - 1;
	chadfs32_write_sector(dev, vblkeloc.a, &vblk);

	chadfs32_dirent_t direntry = { fileid, ifileblkeloc.i };
	status = chadfs32_append_file(dev, mblkloc, &svpardir, &direntry, sizeof(direntry));
	return status;
}

/*
	Create new directory
*/
chadfs_status_t chadfs32_create_dir(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	uint32_t attributes
) {
	return chadfs32_create_file(
		dev,
		mblkloc,
		spath,
		attributes | CHADFS_FILE_ATTRIBUTE_DIRECTORY,
		NULL,
		0
	);
}

chadfs_status_t chadfs32_read_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	void* buffer,
	uint32_t offset,
	uint32_t len
) {
	chadfs_status_t status;
	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_fblk(dev, mblkloc, spath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	return chadfs32_read_data(dev, (chadfs32_loc_t*)&vblkeloc, fblk.firstdblk, buffer, offset, len);
}

chadfs_status_t chadfs32_append_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	const void* data,
	uint32_t len
) {
	if (!len) return CHADFS_STATUS_ZERO_DATA_LEN;

	chadfs_status_t status;
	chadfs_sv_t svfilename;
	if (
		!chadfs_get_file_name(spath, &svfilename)
	) return CHADFS_STATUS_INVALID_PATH;

	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_fblk(dev, mblkloc, spath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	uint32_t itaddr = vblkeloc.a + 1;
	uint32_t dtaddr = itaddr + vblk.numiblks;

	chadfs32_iblk_t iblk;
	chadfs32_eloc_t lastieloc;
	chadfs32_eloc_t firstieloc;
	uint8_t tmp[CHADFS_SECTOR_SIZE];
	uint32_t iiblk = CHADFS_IBLK_INDEX(fblk.lastdblk);
	uint32_t iientry = CHADFS_IENTRY_INDEX(fblk.lastdblk);
	uint32_t leftbytes = fblk.size % CHADFS_SECTOR_SIZE;
	if (leftbytes) {
		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
		chadfs32_read_sector(dev, dtaddr + fblk.lastdblk, tmp);

		if (leftbytes + len <= CHADFS_SECTOR_SIZE) {
			memcpy(&tmp[leftbytes], data, len);
			chadfs32_write_sector(dev, dtaddr + fblk.lastdblk, tmp);

			iblk.d[iientry].numbytes += len;
			chadfs32_write_sector(dev, itaddr + iiblk, &iblk);
			
			fblk.size += len;
			chadfs32_write_sector(dev, fblkeloc.a, &fblk);
			return CHADFS_STATUS_OK;
		}
		
		uint32_t addedbytes = CHADFS_SECTOR_SIZE - leftbytes;
		memcpy(&tmp[leftbytes], data, addedbytes);
		chadfs32_write_sector(dev, dtaddr + fblk.lastdblk, tmp);
		
		iblk.d[iientry].numbytes += addedbytes;
		data = (void*)((size_t)data + addedbytes);
		len -= addedbytes;

		status = chadfs32_write_data(dev, (chadfs32_loc_t*)&vblkeloc, data, len, &firstieloc, &lastieloc);
		if (status != CHADFS_STATUS_OK) return status;

		iblk.d[iientry].nextdata = firstieloc.i;
		chadfs32_write_sector(dev, itaddr + iiblk, &iblk);

		fblk.lastdblk = lastieloc.i;
		chadfs32_write_sector(dev, fblkeloc.a, &fblk);

		vblk.numdblks += CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
		chadfs32_write_sector(dev, vblkeloc.a, &vblk);
		return CHADFS_STATUS_OK;
	}

	status = chadfs32_write_data(dev, (chadfs32_loc_t*)&vblkeloc, data, len, &firstieloc, &lastieloc);
	if (status != CHADFS_STATUS_OK) return status;

	if (fblk.size) {
		chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
		iblk.d[iientry].nextdata = firstieloc.i;
		chadfs32_write_sector(dev, itaddr + iiblk, &iblk);
	}
	else fblk.firstdblk = firstieloc.i;

	fblk.size += len;
	fblk.lastdblk = lastieloc.i;
	chadfs32_write_sector(dev, fblkeloc.a, &fblk);

	vblk.numdblks += CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	chadfs32_write_sector(dev, vblkeloc.a, &vblk);
	return CHADFS_STATUS_OK;
}

chadfs_status_t chadfs32_trunc_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	uint32_t len
) {
	chadfs_status_t status;
	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_fblk(dev, mblkloc, spath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;
	if (len > fblk.size) return CHADFS_STATUS_INVALID_OFFSET;
	if (len == fblk.size) return CHADFS_STATUS_OK;

	chadfs32_eloc_t lastidblkeloc;
	status = chadfs32_cut_data(dev, (chadfs32_loc_t*)&vblkeloc, fblk.firstdblk, len, &lastidblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	const uint32_t oldsectors = CHADFS_ALIGN_VALUE_UP(fblk.size, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	const uint32_t savedsectors = CHADFS_ALIGN_VALUE_UP(len, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;

	fblk.size = len;
	fblk.lastdblk = lastidblkeloc.i;
	if (!lastidblkeloc.i) fblk.firstdblk = 0;
	chadfs32_write_sector(dev, fblkeloc.a, &fblk);

	vblk.numdblks -= oldsectors - savedsectors;
	chadfs32_write_sector(dev, vblkeloc.a, &vblk);
	return CHADFS_STATUS_OK;
}

chadfs_status_t chadfs32_remove_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath
) {
	chadfs_status_t status;
	chadfs_sv_t svpardir;
	chadfs_sv_t svfname;
	if (
		!chadfs_get_parent_dir(spath, &svpardir) ||
		!chadfs_get_file_name(spath, &svfname)
	) return CHADFS_STATUS_INVALID_PATH;

	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_fblk(dev, mblkloc, spath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;

	uint32_t itaddr = vblkeloc.a + 1;

	status = chadfs32_cut_data(dev, (chadfs32_loc_t*)&vblkeloc, fblk.firstdblk, 0, NULL);
	if (status != CHADFS_STATUS_OK) return status;

	chadfs32_iblk_t iblk;
	uint32_t iiblk = CHADFS_IBLK_INDEX(fblkeloc.i);
	uint32_t iientry = CHADFS_IENTRY_INDEX(fblkeloc.i);
	chadfs32_read_sector(dev, itaddr + iiblk, &iblk);
	memset(&iblk.f[iientry], 0, sizeof(iblk.f[iientry]));
	chadfs32_write_sector(dev, itaddr + iiblk, &iblk);

	vblk.numfblks -= 1;
	vblk.numdblks -= CHADFS_ALIGN_VALUE_UP(fblk.size, CHADFS_SECTOR_SIZE) / CHADFS_SECTOR_SIZE;
	chadfs32_write_sector(dev, vblkeloc.a, &vblk);

	/* fix dir data */
	status = chadfs32_read_fblk(dev, mblkloc, &svpardir, &fblk, NULL, NULL, NULL);
	if (status != CHADFS_STATUS_OK) return status;
	const uint32_t dirsize = fblk.size;
	const uint32_t lastdirentryoffset = dirsize - (uint32_t)sizeof(chadfs32_dirent_t);

	chadfs32_dirit_t iter;
	status = chadfs32_create_iter(dev, mblkloc, &svpardir, &iter, &fblk);
	if (status == CHADFS_STATUS_ZERO_DATA_LEN) return CHADFS_STATUS_FILE_NOT_FOUND;

	uint32_t direntryoffset = 0;
	do {
		if (!memcmp(svfname.s, fblk.name, svfname.l)) {
			chadfs32_dirent_t lastdirentry;
			status = chadfs32_read_file(dev, mblkloc, &svpardir, &lastdirentry, lastdirentryoffset, sizeof(chadfs32_dirent_t));
			if (status != CHADFS_STATUS_OK) return status;
			
			status = chadfs32_write_file(dev, mblkloc, &svpardir, &lastdirentry, direntryoffset, sizeof(chadfs32_dirent_t));
			if (status != CHADFS_STATUS_OK) return status;

			return chadfs32_trunc_file(dev, mblkloc, &svpardir, lastdirentryoffset);
		}
		
		status = chadfs32_move_iter(dev, &iter, &fblk);
		if (status != CHADFS_STATUS_OK) {
			if (status == CHADFS_STATUS_ZERO_DATA_LEN) break;
			return status;
		}

		direntryoffset += sizeof(chadfs32_dirent_t);
	} while (status == CHADFS_STATUS_OK);

	return CHADFS_STATUS_FILE_NOT_FOUND;
}

chadfs_status_t chadfs32_write_file(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	const void* data,
	uint32_t offset,
	uint32_t len
) {
	chadfs_status_t status = chadfs32_trunc_file(dev, mblkloc, spath, offset);
	if (status != CHADFS_STATUS_OK) return status;
	return chadfs32_append_file(dev, mblkloc, spath, data, len);
}

/* ================================================= */

/*
	Add new volume
*/
chadfs_status_t chadfs32_add_volume(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs32_vblk_t* vblk
) {
	if (!vblk->numiblks) return CHADFS_STATUS_ZERO_VOLUME_LEN;

	chadfs_status_t status;
	chadfs32_mblk_t* mblk = (chadfs32_mblk_t*)mblkloc->d;
	chadfs32_vblk_t tmpvblk;
	uint32_t saddr;
	if (!mblk->numvolumes) {
		mblk->numvolumes = 1;
		mblk->firstvolume = 1;
		saddr = 1;
	}
	else {
		saddr = mblkloc->a + mblk->firstvolume;
		for (uint32_t i = 0; i < mblk->numvolumes; ++i) {
			chadfs32_read_sector(dev, saddr, &tmpvblk);
			if (!strcmp((char*)tmpvblk.name, (char*)vblk->name)) return CHADFS_STATUS_VOLUME_ALREADY_EXISTS;

			saddr += tmpvblk.nextvolume;
		}

		tmpvblk.nextvolume = 1 + tmpvblk.numiblks * (1 + CHADFS_NUMOF_IBLK_ENTRIES);
		chadfs32_write_sector(dev, saddr, &tmpvblk);
		saddr += tmpvblk.nextvolume;
		mblk->numvolumes += 1;
	}

	memcpy(&tmpvblk, vblk, sizeof(tmpvblk));
	tmpvblk.numfblks = 1;
	chadfs32_write_sector(dev, saddr, &tmpvblk);
	saddr += 1;
	
	uint8_t tmp[CHADFS_SECTOR_SIZE];
	memset(tmp, 0, sizeof(tmp));

	chadfs_sv_t volname = CHADFS_STATIC_SV(vblk->name, strlen((char*)vblk->name));
	((chadfs32_iblk_t*)tmp)->f[0].id = chadfs_get_path_hash(&volname);
	((chadfs32_iblk_t*)tmp)->f[0].active = 1;
	chadfs32_write_sector(dev, saddr, tmp);
	((chadfs32_iblk_t*)tmp)->f[0].id = 0;
	((chadfs32_iblk_t*)tmp)->f[0].active = 0;
	saddr += 1;

	const uint32_t totalvolsectors = vblk->numiblks * (1 + CHADFS_NUMOF_IBLK_ENTRIES) - 1;
	for (uint32_t i = 0; i < totalvolsectors; ++i) chadfs32_write_sector(dev, saddr + i, tmp);

	chadfs32_fblk_t tmpfblk;
	status = chadfs32_init_fblk(&tmpfblk, &volname, 0);
	if (status != CHADFS_STATUS_OK) return status;

	chadfs32_write_sector(dev, saddr - 1 + vblk->numiblks, &tmpfblk);

	mblk->csum = (uint8_t)(-chadfs_get_bytesum(mblk, 9));
	chadfs32_write_sector(dev, 0, mblk);

	return CHADFS_STATUS_OK;
}

/* ================================================= */

/*
	Create directory iterator
*/
chadfs_status_t chadfs32_create_iter(
	void* dev,
	const chadfs32_loc_t* mblkloc,
	const chadfs_sv_t* spath,
	chadfs32_dirit_t* iter,
	chadfs32_fblk_t* firstfblk
) {
	chadfs_status_t status;
	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	status = chadfs32_read_fblk(dev, mblkloc, spath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) return status;
	if (!(fblk.attributes & CHADFS_FILE_ATTRIBUTE_DIRECTORY)) return CHADFS_STATUS_NOT_DIR;
	if (!fblk.size) return CHADFS_STATUS_ZERO_DATA_LEN;

	chadfs32_dirit_t newiter;
	newiter.itbladdr = vblkeloc.a + 1;
	newiter.dtbladdr = newiter.itbladdr + vblk.numiblks;
	newiter.idcurrent = fblk.firstdblk;
	newiter.idirentry = 0;
	newiter.direntries = fblk.size / sizeof(chadfs32_dirent_t);

	if (iter) memcpy(iter, &newiter, sizeof(*iter));
	if (firstfblk) {
		uint8_t tmp[CHADFS_SECTOR_SIZE];
		chadfs32_read_sector(dev, newiter.dtbladdr + newiter.idcurrent, tmp);

		const uint32_t ifblk = ((chadfs32_dirent_t*)tmp)[0].index;
		chadfs32_read_sector(dev, newiter.dtbladdr + ifblk, firstfblk);
	}

	return CHADFS_STATUS_OK;
}

/*
	Move directory iterator
*/
chadfs_status_t chadfs32_move_iter(
	void* dev,
	chadfs32_dirit_t* iter,
	chadfs32_fblk_t* fblk
) {
	chadfs32_iblk_t iblk;
	uint32_t iiblk = CHADFS_IBLK_INDEX(iter->idcurrent);
	uint32_t iientry = CHADFS_IENTRY_INDEX(iter->idcurrent);
	iter->idirentry += 1;
	if (iter->idirentry >= iter->direntries) return CHADFS_STATUS_ZERO_DATA_LEN;

	uint32_t irelentry = iter->idirentry % CHADFS_NUMOF_DIR_DBLK_ENTRIES;
	if (!irelentry) {
		chadfs32_read_sector(dev, iter->itbladdr + iiblk, &iblk);
		iter->idcurrent = iblk.d[iientry].nextdata;
	}

	if (fblk) {
		uint8_t tmp[CHADFS_SECTOR_SIZE];
		chadfs32_read_sector(dev, iter->dtbladdr + iter->idcurrent, tmp);

		const uint32_t ifblk = ((chadfs32_dirent_t*)tmp)[irelentry].index;
		chadfs32_read_sector(dev, iter->dtbladdr + ifblk, fblk);
	}

	return CHADFS_STATUS_OK;
}

/* ================================================= */