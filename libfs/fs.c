#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */

struct  __attribute__((__packed__)) superblock_t{
	uint64_t signature;
	uint16_t total_blk_count;
	uint16_t rdir_blk;
	uint16_t data_blk_idx;
	uint16_t data_blk;
	uint8_t fat_blk_count;
	uint8_t padding[BLOCK_SIZE-17];
} ;
struct superblock_t* superblock;
struct __attribute__((__packed__)) fat_block_t{
	uint16_t files[BLOCK_SIZE/2];
} ;
struct fat_block_t* fat_block;
struct  __attribute__((__packed__)) root_dir_entry_t{
	uint8_t filename[MAX_FILENAME];
	uint32_t size;
	uint16_t first_idx;
	uint8_t padding[10];
};
struct __attribute__((__packed__)) root_dir_t{
	struct root_dir_entry_t* entries[FS_FILE_MAX_COUNT];
};
struct root_dir_t root_dir;
int fs_mount(const char *diskname)
{
	printf("One\n");
	int safe = block_disk_open(diskname);
	if (safe != 0){
		return -1;
	}
	printf("Two\n");
	if (block_read(0, &superblock) == -1) {
        block_disk_close();
        return -1;
    }
	printf("Three\n");

	if (block_read(superblock->rdir_blk, &root_dir) == -1){
		block_disk_close();
        return -1;
	}
	printf("Four\n");
	for (int i = 0; i < superblock->fat_blk_count; i++) {
        if (block_read(1 + i, &fat_block) == -1) {
           return -1;
        }
	}
	printf("Five\n");
	if (superblock->signature == FS_SIGNATURE){
		return -1;
	}
	printf("Six\n");
	if (superblock->total_blk_count != block_disk_count()){
		return -1;
	}
	return 0;

}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	return 0;
}

int fs_info(void)
{
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", superblock->total_blk_count);
	printf("fat_blk_count=%d\n", superblock->fat_blk_count);
	printf("rdir_blk=%d\n", superblock->rdir_blk);
	printf("data_blk=%d\n", superblock->data_blk_idx);
	printf("data_blk_count=%d\n", superblock->data_blk);
	printf("signature=%ld\n", superblock->signature);
	//printf("fat_free_ratio=%d",);
	int empty_rdir = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(root_dir->entries[i]->filename[0] == EMPTY){
			empty_rdir++;
		}
	}
	printf("rdir_free_ratio=%d/%d", empty_rdir, FS_FILE_MAX_COUNT);
	return 0;
	
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

