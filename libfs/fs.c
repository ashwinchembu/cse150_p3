#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disk.h"
#include "fs.h"


#define FS_SIGNATURE 6000536558536704837

#define FAT_EOC 0xFFFF

#define MAX_FILENAME 16

#define EMPTY '\0'

#define FAT_SIZE (4096/2)

//superblock struct
struct __attribute__((__packed__)) superblock_t {
  uint64_t signature;
  uint16_t total_blk_count;
  uint16_t rdir_blk;
  uint16_t data_blk_idx;
  uint16_t data_blk;
  uint8_t fat_blk_count;
  uint8_t padding[BLOCK_SIZE - 17];
};

//create superblock instance
struct superblock_t *superblock;

//fat block struct
struct __attribute__((__packed__)) fat_block_entry_t {
  uint16_t directory;
};

//create fat_block structure 
struct fat_block_entry_t *fat_block_arr;
struct __attribute__((__packed__)) root_dir_entry_t {
  uint8_t filename[MAX_FILENAME];
  uint32_t size;
  uint16_t first_idx;
  uint8_t padding[10];
};

//create root_dir instance
struct root_dir_entry_t root_dir[FS_FILE_MAX_COUNT];

//create file directory entries
struct file_info {
  size_t offset;
  int loc;
};

//create file directory instance
struct file_info file_directory[FS_OPEN_MAX_COUNT];

bool validmount = false;

void close_fd(void) {

  //iterate through file directory and close them
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    file_directory[i].loc = -1;
  }
}

int fs_mount(const char *diskname) {

  //check if disk exists
  int safe = block_disk_open(diskname);
  if (safe != 0) {

    return -1;
  }

  //allocate space for superblock and read 
  superblock = calloc(BLOCK_SIZE, 1);
  if (superblock == NULL) {
    return -1;
  }
  if (block_read(0, superblock) == -1) {
    block_disk_close();
    return -1;
  }

  //place root directory into appropriate array
  if (block_read(superblock->rdir_blk, root_dir) == -1) {

    block_disk_close();
    return -1;
  }

  //make a fat block array and read from superblock
  fat_block_arr = malloc((superblock->fat_blk_count) * BLOCK_SIZE);
  for (int i = 0; i < superblock->fat_blk_count; i++) {

    if (block_read(i + 1, &fat_block_arr[i * FAT_SIZE]) == -1) {

      return -1;
    }
  }

  //check signature
  if (superblock->signature != FS_SIGNATURE) {
    return -1;
  }

  //check number of blocks
  if (superblock->total_blk_count != block_disk_count()) {
    return -1;
  }

  //close all open files
  close_fd();

  //global state for later calls
  validmount = true;

  return 0;
}

int fs_umount(void) {

  //check if there are any open files
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    if (file_directory[i].loc != -1) {

      return -1;  
    }
  }

  //check if state is invalid
  if (validmount == false) {

    return -1;
  }

  //mark as unmounted
  validmount = false;


  //write back fat blocks
  for (int i = 0; i < superblock->fat_blk_count; i++) {
    if (block_write(i + 1, &fat_block_arr[i * FAT_SIZE]) == -1) {
      return -1;
    }
  }

  //write back root directory
  block_write(superblock->rdir_blk, root_dir);

  //close disk
  int safe = block_disk_close();

  //check if closed
  if (safe != 0) {

    return -1;
  }

  return 0;
}

int fs_info(void) {

  //check if there is a disk mounted
  if (validmount == false) {
    return -1;
  }
  printf("FS Info:\n");
  printf("total_blk_count=%d\n", superblock->total_blk_count);
  printf("fat_blk_count=%d\n", superblock->fat_blk_count);
  printf("rdir_blk=%d\n", superblock->rdir_blk);
  printf("data_blk=%d\n", superblock->data_blk_idx);
  printf("data_blk_count=%d\n", superblock->data_blk);

  //iterate over fat blocks and count open ones
  int free_fats = 0;
  for (int i = 0; i < superblock->data_blk; i++) {
    if (fat_block_arr[i].directory == 0) {

      free_fats++;
    }
  }
  printf("fat_free_ratio=%d/%d\n", free_fats, superblock->data_blk);

  //iterate over root directory and count open ones
  int empty_rdir = 0;
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    if (root_dir[i].filename[0] == EMPTY) {

      empty_rdir++;
    }
  }
  printf("rdir_free_ratio=%d/%d\n", empty_rdir, FS_FILE_MAX_COUNT);

  return 0;
}

int findempty(struct root_dir_entry_t *arr) {

  //iterate over root directory and find the empty filename
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

    if (arr[i].filename[0] == EMPTY) {

      return i;
    }
  }

  return -1;
}

int findfile(struct root_dir_entry_t *arr, const char *filename) {

  //iterate over files and find corresponding file location
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

    if (strcmp((char *)arr[i].filename, filename) == 0) {

      return i;
      break;
    }
  }

  return -1;
}

int fs_create(const char *filename) {

  //check legitimacy of request
  if (validmount == false) {

    return -1;
  }
  if (superblock == NULL) {

    return -1;
  }
  if (strlen(filename) > MAX_FILENAME && filename != NULL && filename[strlen(filename)] == '\0') {

    return -1;
  }

  //find the next open spot in the root directory
  int insert = findempty(root_dir);

  //see if file is already in the root directory
  int exists = findfile(root_dir, filename);
  if (insert != -1 && exists == -1) {

    //copy file info to root directory and write to block
    strcpy((char *)root_dir[insert].filename, filename);
    root_dir[insert].size = 0;
    root_dir[insert].first_idx = FAT_EOC;
    block_write(superblock->rdir_blk, root_dir);
    return 0;
  }

  return -1;
}

int fs_delete(const char *filename) {

  //check legitimacy of request
  if (validmount == false) {

    return -1;
  }

  //see if file is in root directory
  int exists = findfile(root_dir, filename);
  if (exists != -1) {

    //reset the info at the directory and iterate through fat blocks and clear them
    memset((char *)root_dir[exists].filename, 0,
           strlen((char *)root_dir[exists].filename));
    root_dir[exists].size = 0;
    uint16_t iter = root_dir[exists].first_idx + superblock->data_blk_idx;
    uint16_t prev;
    while (iter != FAT_EOC) {
      prev = fat_block_arr[iter].directory;
      fat_block_arr[iter].directory = 0;
      iter = prev;
    }

    //reset first index and write to block
    root_dir[exists].first_idx = FAT_EOC;
    block_write(superblock->rdir_blk, root_dir);

    return 0;
  }

  return exists;
}

void printinfo(struct root_dir_entry_t *entry) {

  //print info from entry
  if (entry != NULL) {

    printf("file: %s, ", entry->filename);
    printf("size: %d, ", entry->size);
    printf("data_blk: %d\n", entry->first_idx);
  }
}

int fs_ls(void) {

  //mount ls
  if (validmount == false) {

    return -1;
  }

  //iterate over root directory and print
  printf("FS Ls:\n");
  for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

    struct root_dir_entry_t *entry = &root_dir[i];
    if (entry->filename[0] != EMPTY) {
      printinfo(entry);
    }
  }

  return 0;
}

int findemptyindir(struct file_info *arr) {

  //iterate over array and find which file directory is empty
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
    if (arr[i].loc == -1) {

      return i;
      break;
    }
  }

  return -1;
}

int fs_open(const char *filename) {
  
  //check prerequisities
  if (validmount == false && strlen(filename) < MAX_FILENAME  &&filename != NULL && filename[strlen(filename)] == '\0') {

    return -1;
  }

  //find next open directory
  int open_directory = findemptyindir(file_directory);

  //find if it exists
  int exists = findfile(root_dir, filename);

  //if it exists and there is an open directory, set the file descriptor information accordingly
  if (exists == -1 || open_directory == -1) {

    return -1;
  }

  file_directory[open_directory].loc = exists;
  file_directory[open_directory].offset = 0;
  return 0;
}

int fs_close(int fd) {

  //if the file directory exists, reset the information
  if (validmount == false || fd > FS_OPEN_MAX_COUNT || fd < 0 ){
    return -1;
  }

  int location = file_directory[fd].loc;
  if (location != -1) {

    file_directory[fd].loc = -1;
    return 0;
  }

  return -1;
}

int fs_stat(int fd) {

  //if the file directory exists, retrieve the information
  if (validmount == false || fd > FS_OPEN_MAX_COUNT){

    return -1;
  }

  //print the size of the file based on its respective location 
  int location = file_directory[fd].loc;
  if (location != -1 && validmount) {

    return (root_dir[location].size);
  }
  return -1;
}

int fs_lseek(int fd, size_t offset) {

  //if the file directory exists, change the offset
  if (validmount == false || fd > FS_OPEN_MAX_COUNT) {
    return -1;
  }

  //check if the fd is open, then ensure the offset is less than the size and change the offset 
  if (file_directory[fd].loc != -1) {

    uint32_t size = root_dir[file_directory[fd].loc].size;
    if (offset <= size) {

      file_directory[fd].offset = offset;
      return 0;
    }
  }

  return -1;
}

//initialize bounce buffer
uint8_t bounce[BLOCK_SIZE];

size_t find_data_blk(int fd) {

  //get offset and block offset is on in the file
  size_t offset = file_directory[fd].offset;
  int block = offset / BLOCK_SIZE;

  //get the starting index from the root directory
  uint16_t start = (uint16_t)root_dir[file_directory[fd].loc].first_idx;

  //if it doesn't exist in the root directory
  if (start == FAT_EOC) {
    return FAT_EOC;
  }

  //iterate through and return the block it on in the file directory
  while (block > 0) {
    if (start == FAT_EOC) {
      return block;
    }

    int block = start / FAT_SIZE;
    if (block < superblock->fat_blk_count) {

      start = fat_block_arr[block].directory;
      block--;
    } else {

      return start;
    }
  }

  return start;
}
int extend(int fd) {

  //get latest block in chain
  size_t insert = root_dir[file_directory[fd].loc].first_idx;
  if (insert != FAT_EOC) {
    while (fat_block_arr[insert].directory != FAT_EOC) {
      insert = fat_block_arr[insert].directory;
    }
  }

  //iterate and find new place in fat blocks
  for (int i = 0; i < superblock->data_blk; i++) {
    if (fat_block_arr[i].directory == 0) {

      //if new file, add to root directory
      if (root_dir[file_directory[fd].loc].first_idx == FAT_EOC) {

        root_dir[file_directory[fd].loc].first_idx = i;
      } else {

        //set current last pointer to new space
        fat_block_arr[insert].directory = i;
      }

      //set new space to FAT_EOC to show end
      fat_block_arr[i].directory = FAT_EOC;
      return i;

    }
  }

  return -1;
}
int fs_write(int fd, void *buf, size_t count) {

  //check preqrequisites
  if (buf == NULL || fd > FS_OPEN_MAX_COUNT || file_directory[fd].loc == -1 ||
      validmount == false) {
    return -1;
  }

  //find the latest block and offset on the block
  size_t start_block_idx = find_data_blk(fd);
  int start_block_offset = file_directory[fd].offset % BLOCK_SIZE;

  //initalize iterator variables
  size_t bytes_copied = 0;
  int bytes_left = (int)count;
  while (bytes_left > 0) {

    // reset bounce
    memset(bounce, 0, BLOCK_SIZE);

    // get the number of bytes to copy in block
    int added_bytes = BLOCK_SIZE - start_block_offset;

    if (bytes_left < added_bytes) {
      // last page
      added_bytes = bytes_left;
    }

    // red block to bounce or make new one
    if (start_block_idx == FAT_EOC) {

      int valid = extend(fd);
      start_block_idx = valid;

      if (valid == -1) {

        return bytes_copied;
      }
    } else {

      block_read(start_block_idx + superblock->data_blk_idx, bounce);
    }
    // copy part of chunk to end of count to end of buffer
    memcpy(bounce + start_block_offset, (uint8_t *)buf + bytes_copied,
           added_bytes);

    //write to block
    block_write(start_block_idx + superblock->data_blk_idx, bounce);


    //adjust incrementers accordingly
    bytes_left -= added_bytes;
    bytes_copied += added_bytes;


    //iterate to next block if more exist
    if (bytes_left > 0) {

      int block = start_block_idx;
      if (block < superblock->data_blk) {

        // move to the next block
        start_block_idx = fat_block_arr[block].directory;
        start_block_offset = 0;
      } else {

        break;
      }
    }
  }

  //move offset and change size accordingly
  file_directory[fd].offset += bytes_copied;
  root_dir[file_directory[fd].loc].size += bytes_copied;
  return bytes_copied;
}

int fs_read(int fd, void *buf, size_t count) {

  //check preqrequisites
  if (buf == NULL || fd > FS_OPEN_MAX_COUNT || file_directory[fd].loc == -1 ||
      validmount == false) {
    return -1;
  }

  //find the latest block and offset on the block
  size_t start_block_idx = find_data_blk(fd);
  int start_block_offset = file_directory[fd].offset % BLOCK_SIZE;

  //initialize iterators
  size_t bytes_copied = 0;
  int bytes_left = (int)count;


  //check if there are bytes left to read and block is valid

  while (bytes_left > 0 && start_block_idx != FAT_EOC) {

    // reset bounce
    memset(bounce, 0, BLOCK_SIZE);

    // get the number of bytes to copy in block
    int added_bytes = BLOCK_SIZE - start_block_offset;

    if (bytes_left < added_bytes) {

      // last page
      added_bytes = bytes_left;
    }
    
    // read block to bounce
    block_read(start_block_idx + superblock->data_blk_idx, bounce);

    // copy start of chunk to end of count to end of buffer
    memcpy((uint8_t *)buf + bytes_copied, bounce + start_block_offset,
           added_bytes);
    added_bytes = strlen((char*) bounce + start_block_offset);

    // reduce total blocks left to copy
    bytes_left -= added_bytes;
    bytes_copied += added_bytes;

    //if bytes are left, iterate through
    if (bytes_left > 0) {

      int block = start_block_idx;

      // fat_block_arr[block].directory, superblock->data_blk);
      if (block < superblock->data_blk &&
          fat_block_arr[block].directory != FAT_EOC) {

        // move to the next block
        start_block_idx = fat_block_arr[block].directory;
        start_block_offset = 0;

      } else {

        break;
      }
    }
  }

  //move offset
  file_directory[fd].offset += bytes_copied;
  return bytes_copied;
}
