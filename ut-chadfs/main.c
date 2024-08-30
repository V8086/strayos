#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <chadfs.h>

#define PANIC_ERR(__status) {\
	fprintf(stderr, "Error: `%s`!\n", chadfs_status_to_str(__status));\
	exit(-1);\
	return;\
}

void act_show_info(void* ppath);
void act_create_mblk(const char* mpath);
void act_add_vblk(const char* mpath, const char* name, uint32_t numiblks);
void act_list_vblks(const char* mpath);
void act_print_volume(const char* mpath, const char* name);
void act_print_file(const char* mpath, const char* fpath);
void act_list_dir(const char* mpath, const char* dpath);
void act_create_file(const char* mpath, const char* infpath, const char* extfpath);
void act_create_dir(const char* mpath, const char* indirpath);
void act_read_txt_file(const char* mpath, const char* infpath, uint32_t offset, uint32_t len);
void act_read_bin_file(const char* mpath, const char* infpath, uint32_t offset, uint32_t len);
void act_trunc_file(const char* mpath, const char* fpath, uint32_t len);
void act_remove_file(const char* mpath, const char* fpath);
void act_write_file(const char* mpath, const char* infpath, const char* extfpath, uint32_t offset);

int main(int argc, char** argv) {
	if (argc >= 2 && (!strcmp(argv[1], "-help") || !strcmp(argv[1], "-info"))) act_show_info(argv[0]);
	else if (argc >= 3 && !strcmp(argv[1], "-create-main")) act_create_mblk(argv[2]);
	else if (argc >= 5 && !strcmp(argv[1], "-add-volume")) act_add_vblk(argv[2], argv[3], (uint32_t)strtoul(argv[4], NULL, 10));
	else if (argc >= 3 && !strcmp(argv[1], "-list-volumes")) act_list_vblks(argv[2]);
	else if (argc >= 4 && !strcmp(argv[1], "-list-dir")) act_list_dir(argv[2], argv[3]);
	else if (argc >= 4 && !strcmp(argv[1], "-print-volume")) act_print_volume(argv[2], argv[3]);
	else if (argc >= 4 && !strcmp(argv[1], "-print-file")) act_print_file(argv[2], argv[3]);
	else if (argc >= 4 && !strcmp(argv[1], "-create-dir")) act_create_dir(argv[2], argv[3]);
	else if (argc >= 4 && !strcmp(argv[1], "-create-file")) {
		char* extfpath = NULL;
		if (argc >= 5) extfpath = argv[4];
		act_create_file(argv[2], argv[3], extfpath);
	}
	else if (argc >= 4 && !strcmp(argv[1], "-read-txt-file")) {
		uint32_t offset = 0;
		uint32_t len = 0;
		if (argc >= 5) {
			offset = (uint32_t)strtoul(argv[4], NULL, 10);
			if (argc >= 6) len = (uint32_t)strtoul(argv[5], NULL, 10);
		}

		act_read_txt_file(argv[2], argv[3], offset, len);
	}
	else if (argc >= 4 && !strcmp(argv[1], "-read-bin-file")) {
		uint32_t offset = 0;
		uint32_t len = 0;
		if (argc >= 5) {
			offset = (uint32_t)strtoul(argv[4], NULL, 10);
			if (argc >= 6) len = (uint32_t)strtoul(argv[5], NULL, 10);
		}

		act_read_bin_file(argv[2], argv[3], offset, len);
	}
	else if (
		argc >= 5 && !strcmp(argv[1], "-trunc-file")
	) act_trunc_file(argv[2], argv[3], (uint32_t)strtoul(argv[4], NULL, 10));
	else if (argc >= 4 && !strcmp(argv[1], "-remove-file")) act_remove_file(argv[2], argv[3]);
	else if (
		argc >= 6 && !strcmp(argv[1], "-write-file")
	) act_write_file(argv[2], argv[3], argv[4], (uint32_t)strtoul(argv[5], NULL, 10));
	else {
		fprintf(stderr, "Unknown action or/and invalid params! (`-help` - show info)\n");
		return -1;
	}
}

void act_show_info(void* ppath) {
	printf("CHADFS utility (v1). Usage: `%s <action> [params]`\nActions:\n", (char*)ppath);

	puts("`-help`/`-info` - show info(actions & params...)");
	puts("`-create-main <path>` - create CHADFS binary image");

	puts("`-add-volume <path> <name> <numiblks>` - add volume");
	puts("\t<name> - volume name");
	puts("\t<numiblks> - num of ID blocks");

	puts("`-list-volumes <path>` - list volumes");
	puts("`-list-dir <path> <dpath>` - list files in directory");

	puts("`-print-volume <path> <name>` - show volume (info)");
	puts("\t<name> - volume name");

	puts("`-print-file <path> <fpath>` - show file (info)");
	puts("\t<fpath> - file path (inside CHADFS binary img)");

	puts("`-copy-file <path> <infpath> [extfpath]` - create file");
	puts("`-create-dir <path> <indpath>` - create directory");
	puts("`-read-txt-file <path> <fpath> [offset] [size]` - read text file");
	puts("`-read-bin-file <path> <fpath> [offset] [size]` - read binary file");
	puts("`-trunc-file <path> <fpath> <size>` - truncate file");
	puts("`-remove-file <path> <fpath>` - remove file");
	puts("`-write-file <path> <infpath> <extfpath> <offset>` - copy external file content to internal file");
}

void act_create_mblk(const char* mpath) {
	FILE* f = fopen(mpath, "wb");
	if (!f) {
		fprintf(stderr, "Failed to create file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_init_mblk(&mblk);
	if (fwrite(&mblk, sizeof(mblk), 1, f) != 1) {
		fprintf(stderr, "Failed to write data to file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	fclose(f);
}

void act_add_vblk(const char* mpath, const char* name, uint32_t numiblks) {
	chadfs_status_t status;
	chadfs32_vblk_t vblk;
	chadfs_sv_t sv = { (char*)name, strlen(name) };
	chadfs32_init_vblk(&vblk, &sv, numiblks);

	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_loc_t mblkloc = { 0, &mblk };
	status = chadfs32_add_volume(f, &mblkloc, &vblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	fclose(f);
}

void act_list_vblks(const char* mpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	uint32_t saddr = mblk.firstvolume;
	chadfs32_vblk_t tmpvblk;
	for (size_t i = 0; i < mblk.numvolumes; ++i) {
		chadfs32_read_sector(f, saddr, &tmpvblk);
		printf("%u) `%s`(lba=0x%x):\n", (unsigned)(i + 1), (char*)tmpvblk.name, (unsigned)saddr);
		printf("Num of ID blocks: %u\n", (unsigned)tmpvblk.numiblks);
		printf("Num of file blocks: %u\n", (unsigned)tmpvblk.numfblks);
		printf("Num of data blocks: %u\n", (unsigned)tmpvblk.numdblks);
		printf("Next volume: 0x%x/%u\n\n", (unsigned)tmpvblk.nextvolume, (unsigned)tmpvblk.nextvolume);

		saddr += tmpvblk.nextvolume;
	}

	fclose(f);
}

void act_print_volume(const char* mpath, const char* name) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_loc_t mblkloc = { 0, &mblk };
	chadfs_sv_t sv = { (char*)name, strlen(name) };
	chadfs32_vblk_t tmpvblk;
	chadfs32_eloc_t tmpvblkeloc;
	status = chadfs32_read_vblk(f, &mblkloc, &sv, &tmpvblk, &tmpvblkeloc);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	printf("`%s` (lba=0x%x, index=%u):\n", (char*)tmpvblk.name, (unsigned)tmpvblkeloc.a, (unsigned)tmpvblkeloc.i);
	printf("Num of ID blocks: %u\n", (unsigned)tmpvblk.numiblks);
	printf("Num of file blocks: %u\n", (unsigned)tmpvblk.numfblks);
	printf("Num of data blocks: %u\n", (unsigned)tmpvblk.numdblks);
	printf("Next volume: 0x%x/%u\n\n", (unsigned)tmpvblk.nextvolume, (unsigned)tmpvblk.nextvolume);

	fclose(f);
}

void act_print_file(const char* mpath, const char* fpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_eloc_t tmpfblkeloc;
	chadfs32_fblk_t tmpfblk;
	chadfs32_loc_t mblkloc = { 0, &mblk };
	chadfs_sv_t svpath = { (char*)fpath, strlen(fpath) };
	status = chadfs32_read_fblk(f, &mblkloc, &svpath, &tmpfblk, &tmpfblkeloc, NULL, NULL);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	printf("`%s` (lba=0x%x, index=%u):\n", fpath, (unsigned)tmpfblkeloc.a, (unsigned)tmpfblkeloc.i);
	printf("Size: %u (bytes)\n", (unsigned)tmpfblk.size);
	printf("First data block index: %u\n", (unsigned)tmpfblk.firstdblk);
	printf("Last data block index: %u\n", (unsigned)tmpfblk.lastdblk);
	printf("Attributes: 0x%x\n", (unsigned)tmpfblk.attributes);

	if (tmpfblk.attributes & CHADFS_FILE_ATTRIBUTE_DIRECTORY) {
		chadfs32_dirit_t iter;
		status = chadfs32_create_iter(f, &mblkloc, &svpath, &iter, &tmpfblk);
		if (status != CHADFS_STATUS_OK) {
			if (status == CHADFS_STATUS_ZERO_DATA_LEN) puts("No files");
			else PANIC_ERR(status);
		}

		puts("Files:");
		do {
			printf("`%s/%s`:\n", fpath, (char*)tmpfblk.name);
			printf("Size: %u (bytes)\n", (unsigned)tmpfblk.size);
			printf("First data block index: %u\n", (unsigned)tmpfblk.firstdblk);
			printf("Last data block index: %u\n", (unsigned)tmpfblk.lastdblk);
			printf("Attributes: 0x%x\n\n", (unsigned)tmpfblk.attributes);
			status = chadfs32_move_iter(f, &iter, &tmpfblk);
			if (status != CHADFS_STATUS_OK && status != CHADFS_STATUS_ZERO_DATA_LEN) PANIC_ERR(status);
		} while (status != CHADFS_STATUS_ZERO_DATA_LEN);
	}

	fclose(f);
}

void act_list_dir(const char* mpath, const char* dpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_eloc_t tmpfblkeloc;
	chadfs32_fblk_t tmpfblk;
	chadfs32_loc_t mblkloc = { 0, &mblk };
	chadfs_sv_t svpath = { (char*)dpath, strlen(dpath) };
	status = chadfs32_read_fblk(f, &mblkloc, &svpath, &tmpfblk, &tmpfblkeloc, NULL, NULL);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	if (!(tmpfblk.attributes & CHADFS_FILE_ATTRIBUTE_DIRECTORY)) PANIC_ERR(CHADFS_STATUS_NOT_DIR);

	printf("Files in `%s`:\n", dpath);
	chadfs32_dirit_t iter;
	status = chadfs32_create_iter(f, &mblkloc, &svpath, &iter, &tmpfblk);
	if (status != CHADFS_STATUS_OK) {
		if (status == CHADFS_STATUS_ZERO_DATA_LEN) puts("No files");
		else PANIC_ERR(status);
	}

	do {
		printf("`%s/%s`\n", dpath, (char*)tmpfblk.name);
		status = chadfs32_move_iter(f, &iter, &tmpfblk);
		if (status != CHADFS_STATUS_OK && status != CHADFS_STATUS_ZERO_DATA_LEN) PANIC_ERR(status);
	} while (status != CHADFS_STATUS_ZERO_DATA_LEN);

	fclose(f);
}

void act_create_file(const char* mpath, const char* infpath, const char* extfpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs_sv_t svinfpath = { (char*)infpath, strlen(infpath) };
	if (extfpath) {
		FILE* extf = fopen(extfpath, "rb");
		if (!extf) {
			fprintf(stderr, "Failed to open file `%s`!\n", extfpath);
			exit(-1);
			return;
		}

		fseek(extf, 0, SEEK_END);
		size_t extflen = (size_t)ftell(extf);
		fseek(extf, 0, SEEK_SET);

		uint8_t* extfdata = (uint8_t*)malloc(extflen);
		if (!extfdata) {
			fprintf(stderr, "Not enough memory!\n");
			exit(-1);
			return;
		}

		if (fread(extfdata, extflen, 1, extf) != 1) {
			fprintf(stderr, "Failed to read file `%s`!\n", extfpath);
			exit(-1);
			return;
		}

		fclose(extf);

		status = chadfs32_create_file(
			f, &mblkloc, &svinfpath,
			CHADFS_FILE_ATTRIBUTE_READABLE | CHADFS_FILE_ATTRIBUTE_WRITEABLE,
			extfdata, (uint32_t)extflen
		);

		free(extfdata);
	}
	else status = chadfs32_create_file(
		f, &mblkloc, &svinfpath,
		CHADFS_FILE_ATTRIBUTE_READABLE | CHADFS_FILE_ATTRIBUTE_WRITEABLE,
		NULL, 0
	);

	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);
	fclose(f);
}

void act_create_dir(const char* mpath, const char* indirpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs_sv_t svdirpath = { (char*)indirpath, strlen(indirpath) };
	status = chadfs32_create_dir(
		f, &mblkloc, &svdirpath,
		CHADFS_FILE_ATTRIBUTE_READABLE | CHADFS_FILE_ATTRIBUTE_WRITEABLE
	);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);
	
	fclose(f);
}

void act_read_txt_file(const char* mpath, const char* infpath, uint32_t offset, uint32_t len) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs_sv_t svfpath = { (char*)infpath, strlen(infpath) };
	status = chadfs32_read_fblk(f, &mblkloc, &svfpath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	if (!offset) offset = 0;
	if (!len) len = fblk.size - offset;

	char* data = (char*)malloc(len);
	if (!data) {
		fprintf(stderr, "Not enough memory!\n");
		exit(-1);
		return;
	}

	status = chadfs32_read_file(f, &mblkloc, &svfpath, data, offset, len);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	for (size_t i = 0; i < len; ++i) putchar(data[i]);

	free(data);
	fclose(f);
}

void act_read_bin_file(const char* mpath, const char* infpath, uint32_t offset, uint32_t len) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs32_eloc_t fblkeloc;
	chadfs32_eloc_t vblkeloc;
	chadfs32_fblk_t fblk;
	chadfs32_vblk_t vblk;
	chadfs_sv_t svfpath = { (char*)infpath, strlen(infpath) };
	status = chadfs32_read_fblk(f, &mblkloc, &svfpath, &fblk, &fblkeloc, &vblk, &vblkeloc);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	if (!offset) offset = 0;
	if (!len) len = fblk.size - offset;

	uint8_t* data = (uint8_t*)malloc(len);
	if (!data) {
		fprintf(stderr, "Not enough memory!\n");
		exit(-1);
		return;
	}

	status = chadfs32_read_file(f, &mblkloc, &svfpath, data, offset, len);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	if (len) {
		size_t i = 0;
		for (; i < len - 1; ++i) printf("%02x ", (unsigned)data[i]);
		printf("%02x", (unsigned)data[i]);
	}

	free(data);
	fclose(f);
}

void act_trunc_file(const char* mpath, const char* fpath, uint32_t len) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs_sv_t svfpath = { (char*)fpath, strlen(fpath) };
	status = chadfs32_trunc_file(f, &mblkloc, &svfpath, len);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	fclose(f);
}

void act_remove_file(const char* mpath, const char* fpath) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	chadfs_sv_t svfpath = { (char*)fpath, strlen(fpath) };
	status = chadfs32_remove_file(f, &mblkloc, &svfpath);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	fclose(f);
}

void act_write_file(const char* mpath, const char* infpath, const char* extfpath, uint32_t offset) {
	chadfs_status_t status;
	FILE* f = fopen(mpath, "rb+");
	if (!f) {
		fprintf(stderr, "Failed to open file `%s`!\n", mpath);
		exit(-1);
		return;
	}

	chadfs32_mblk_t mblk;
	chadfs32_loc_t mblkloc = { 0, (void*)&mblk };
	status = chadfs32_read_mblk(f, 0, &mblk);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	FILE* extf = fopen(extfpath, "rb");
	if (!extf) {
		fprintf(stderr, "Failed to open file `%s`!\n", extfpath);
		exit(-1);
		return;
	}

	fseek(extf, 0, SEEK_END);
	size_t extflen = (size_t)ftell(extf);
	fseek(extf, 0, SEEK_SET);

	uint8_t* extfdata = (uint8_t*)malloc(extflen);
	if (!extfdata) {
		fprintf(stderr, "Not enough memory!\n");
		exit(-1);
		return;
	}

	if (fread(extfdata, extflen, 1, extf) != 1) {
		fprintf(stderr, "Failed to read file `%s`!\n", extfpath);
		exit(-1);
		return;
	}

	chadfs_sv_t svinfpath = { (char*)infpath, strlen(infpath) };
	status = chadfs32_write_file(f, &mblkloc, &svinfpath, extfdata, offset, (uint32_t)extflen);
	if (status != CHADFS_STATUS_OK) PANIC_ERR(status);

	free(extfdata);
	fclose(extf);
	fclose(f);
}

/* ========================================= */

void chadfs32_write_sector(void* dev, uint32_t address, const void* sectordata) {
	if (fseek((FILE*)dev, (long)(address << 9), SEEK_SET)) {
		fprintf(stderr, "fseek(...) != 0 (lba=0x%x)!\n", (unsigned)address);
		exit(-1);
		return;
	}

	if (fwrite(sectordata, CHADFS_SECTOR_SIZE, 1, (FILE*)dev) != 1) {
		fprintf(stderr, "fwrite(...) != 1 (lba=0x%x)!\n", (unsigned)address);
		exit(-1);
	}
}

void chadfs32_read_sector(void* dev, uint32_t address, void* sectordata) {
	if (fseek((FILE*)dev, (long)(address << 9), SEEK_SET)) {
		fprintf(stderr, "fseek(...) != 0 (lba=0x%x)!\n", (unsigned)address);
		exit(-1);
		return;
	}

	if (fread(sectordata, CHADFS_SECTOR_SIZE, 1, (FILE*)dev) != 1) {
		fprintf(stderr, "fread(...) != 1 (lba=0x%x)!\n", (unsigned)address);
		exit(-1);
	}
}