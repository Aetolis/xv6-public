#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#include "types.h"
#include "fs.h"

// define macros for directory type
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

// array of bitmasks
char bitmask[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

// test 1 -
// Each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV). If not, print ERROR: bad inode.
int test_inode_type(struct dinode *inode, uint ninodes)
{
  for(int i = 0; i < ninodes; i++, inode++){
    switch(inode->type){
      case 0:
        break;
      case T_DIR:
        break;
      case T_FILE:
        break;
      case T_DEV:
        break;
      default:
        return 1;
    }
  }
  return 0;
}

// test 2 -
// For in-use inodes, each address that is used by inode is valid (points to a valid datablock address within the image).
// If the direct block is used and is invalid, print ERROR: bad direct address in inode.
// If the indirect block is in use and is invalid, print ERROR: bad indirect address in inode.
// For effeciency purposes, this function also updates inode_log, diradd_log, indadd_log, and inodenum_log, which are used in subsequent functions.
void test_inode_addrs(char *img_buffer, struct superblock *sb, struct dinode *inode, uchar *inode_log, uchar *diradd_log, uchar *indadd_log, uchar *inodenum_log)
{
  uint *indirect_block;
  // start sector of data blocks
  uint dblockstart = sb->size - sb->nblocks;

  for(int i = 0; i < sb->ninodes; i++, inode++){
    if(inode->type == 0)
      continue;

    // mark in-use inodes in inodenum_log
    inodenum_log[i] = 1;

    // for all direct data blocks
    for(int j = 0; j < NDIRECT; j++){
      if(inode->addrs[j] == 0)
        continue;

      // check if data block is valid
      if(inode->addrs[j] < dblockstart || inode->addrs[j] >= sb->size || inode->addrs[j] == 0){
        fprintf(stderr, "ERROR: bad direct address in inode.\n");
        exit(1);
      }

      // mark datablock as in use
      if(inode_log[inode->addrs[j]-dblockstart] == 0)
        inode_log[inode->addrs[j]-dblockstart] = 1;

      // record number of times direct address was reffered to in inodes
      diradd_log[inode->addrs[j]-dblockstart]++;
    }

    // if indirect block is in use
    if(inode->addrs[NDIRECT] != 0){
      // if indirect block is not valid
      if(inode->addrs[NDIRECT] < (sb->size - sb->nblocks) || inode->addrs[NDIRECT] >= sb->size){
        fprintf(stderr, "ERROR: bad indirect address in inode.\n");
        exit(1);
      }

      // mark indirect data block as in use
      if(inode_log[inode->addrs[NDIRECT]-dblockstart] == 0)
        inode_log[inode->addrs[NDIRECT]-dblockstart] = 1;

      // record number of times indirect block address was reffered to in inodes
      indadd_log[inode->addrs[NDIRECT]-dblockstart]++;

      // calculate address of indirect block
      // img_buffer + (block number of indirect block * size of block)
      indirect_block = (uint *) (img_buffer + (inode->addrs[NDIRECT] * BSIZE));
      // for all data block pointers in indirect block
      for(int j = 0; j < NINDIRECT; j++, indirect_block++){
        if(*indirect_block == 0)
          continue;

        if(*indirect_block < dblockstart || *indirect_block >= sb->size){
          fprintf(stderr, "ERROR: bad indirect address in inode.\n");
          exit(1);
        }

        // mark data block as in use
        if(inode_log[*indirect_block-dblockstart] == 0)
          inode_log[*indirect_block-dblockstart] = 1;

        // record number of times indirect address was reffered to in inodes
        indadd_log[*indirect_block-dblockstart]++;
      }
    }
  }
}

// test 3 -
// Root directory exists, its inode number is 1, and the parent of the root directory is itself.
// If not, print ERROR: root directory does not exist.
int test_root(char *img_buffer, struct dinode *inode)
{
  // initialize flags
  int p_flag = 0;
  int c_flag = 0;
  struct dirent *dir_ent = (struct dirent *) (img_buffer + (inode->addrs[0] * BSIZE));
  uint *indirect_block;

  // if root inode is not a directory
  if(inode->type != T_DIR)
    return 1;

  // for all direct data blocks
  for(int i = 0; i < NDIRECT; i++){
    // skip if data block is not in use
    if(inode->addrs[i] == 0)
      continue;

    // for all directory entries in data block
    for(int j = 0; j < (BSIZE/sizeof(struct dirent)); j++, dir_ent++){
      // if current dir has not been found yet
      if(c_flag == 0 && strcmp(dir_ent->name, ".") == 0){
        c_flag = 1;
        if(dir_ent->inum != 1)
          return 1;
      }

      // if parent dir has not been found yet
      else if(p_flag == 0 && strcmp(dir_ent->name, "..") == 0){
        p_flag = 1;
        if(dir_ent->inum != 1)
          return 1;
      }
      // if both "." and ".." directories have already been found
      if(c_flag == 1 && p_flag == 1)
        return 0;
    }
    if(c_flag == 1 && p_flag == 1)
      return 0;
  }

  // if "." and ".." has already be found in direct data blocks
  if(c_flag == 1 && p_flag == 1)
    return 0;

  // if indirect block is in use
  if(inode->addrs[NDIRECT] != 0){
    // calculate address of indirect block
    indirect_block = (uint *) (img_buffer + (inode->addrs[NDIRECT] * BSIZE));

    // for all indirect data blocks
    for(int i = 0; i < NINDIRECT; i++, indirect_block++){
      // skip if data block is not in use
      if(*indirect_block == 0)
        continue;

      // initialize dir_ent as first directory entry in data block
      dir_ent = (struct dirent *) (img_buffer + (*indirect_block * BSIZE));

      // for all directory entries in data block
      for(int j = 0; j < (BSIZE/sizeof(struct dirent)); j++, dir_ent++){
        // if current dir has not been found yet
        if(c_flag == 0 && strcmp(dir_ent->name, ".") == 0){
          c_flag = 1;
          if(dir_ent->inum != 1)
            return 1;
        }

        // if parent dir has not been found yet
        else if(p_flag == 0 && strcmp(dir_ent->name, "..") == 0){
          p_flag = 1;
          if(dir_ent->inum != 1)
            return 1;
        }
        // if both "." and ".." directories have already been found
        if(c_flag == 1 && p_flag == 1)
          return 0;
      }
      if(c_flag == 1 && p_flag == 1)
        return 0;
    }
    if(c_flag == 1 && p_flag == 1)
      return 0;
  }
  // if "." or ".." directory entry was not found
  if(c_flag == 0 || p_flag == 0)
    return 1;

  return 0;
}

// extra credit 1 -
// find and return inum of child directory.
ushort find_child_dir(char *img_buffer, struct dinode *inode, ushort cur_inum)
{
  struct dirent *dir_ent;
  uint *indirect_block;

  // for all direct data blocks
  for(int i = 0; i < NDIRECT; i++){
    // skip if data block is not in use
    if(inode->addrs[i] == 0)
      continue;

    dir_ent = (struct dirent *) (img_buffer + (inode->addrs[i] * BSIZE));

    // for all directory entries in data block
    for(int j = 0; j < (BSIZE/sizeof(struct dirent)); j++, dir_ent++){
      // if current dir has not be found yet
      if(dir_ent->inum == cur_inum){
        return 0;
      }
    }
  }

  // if indirect block is in use
  if(inode->addrs[NDIRECT] != 0){
    // calculate address of indirect block
    indirect_block = (uint *) (img_buffer + (inode->addrs[NDIRECT] * BSIZE));

    // for all indirect data blocks
    for(int i = 0; i < NINDIRECT; i++, indirect_block++){
      // skip if data block is not in use
      if(*indirect_block == 0)
        continue;

      // initialize dir_ent as first directory entry in data block
      dir_ent = (struct dirent *) (img_buffer + (*indirect_block * BSIZE));

      // for all directory entries in data block
      for(int j = 0; j < (BSIZE/sizeof(struct dirent)); j++, dir_ent++){
        // if current dir has not be found yet
        if(dir_ent->inum == cur_inum){
          return 0;
        }
      }
    }
  }
  return 1;
}

// test 4 -
// Each directory contains . and .. entries, and the . entry points to the directory itself.
// If not, print ERROR: directory not properly formatted.
int test_dir_format(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)
{
  int p_flag;
  int c_flag;
  struct dirent *dir_ent;
  uint *indirect_block;

  // for all inodes
  for(int i = 0; i < ninodes; i++, inode++){
    if(inode->type == T_DIR){
      // initialize flags to keep track if "." and ".." have been found
      p_flag = 0;
      c_flag = 0;

      // for all direct data block
      for(int j = 0; j < NDIRECT; j++){
        // skip if data block is not in use
        if(inode->addrs[j] == 0)
          continue;

        dir_ent = (struct dirent *) (img_buffer + (inode->addrs[j] * BSIZE));

        // for all directory entries in data block
        for(int k = 0; k < (BSIZE/sizeof(struct dirent)); k++, dir_ent++){
          // mark inode as in use in dir_log
          if(dir_ent->inum != 0 && strcmp(dir_ent->name, ".") != 0 && strcmp(dir_ent->name, "..") != 0)
            dir_log[dir_ent->inum]++;

          // if current dir has not be found yet
          else if(c_flag == 0 && strcmp(dir_ent->name, ".") == 0){
            c_flag = 1;
            if(dir_ent->inum != i)
              return 1;
          }

          // if parent dir has not been found yet
          else if(p_flag == 0 && strcmp(dir_ent->name, "..") == 0){
            p_flag = 1;
            if(i != 1 && find_child_dir(img_buffer, (struct dinode *) (img_buffer + (inodestart * BSIZE) + (dir_ent->inum * sizeof(struct dinode))), i)){
              fprintf(stderr, "ERROR: parent directory mismatch.\n");
              exit(1);
            }
          }
        }
      }

      // if indirect block is in use
      if(inode->addrs[NDIRECT] != 0){
        // calculate address of indirect block
        indirect_block = (uint *) (img_buffer + (inode->addrs[NDIRECT] * BSIZE));

        // for all indirect data blocks
        for(int j = 0; j < NINDIRECT; j++, indirect_block++){
          // skip if data block is not in use
          if(*indirect_block == 0)
            continue;

          // initialize dir_ent as first directory entry in data block
          dir_ent = (struct dirent *) (img_buffer + (*indirect_block * BSIZE));

          // for all directory entries in data block
          for(int k = 0; k < (BSIZE/sizeof(struct dirent)); k++, dir_ent++){
            // mark inode as in use in dir_log
            if(dir_ent->inum != 0 && strcmp(dir_ent->name, ".") != 0 && strcmp(dir_ent->name, "..") != 0)
              dir_log[dir_ent->inum]++;

            // if current dir has not be found yet
            else if(c_flag == 0 && strcmp(dir_ent->name, ".") == 0){
              c_flag = 1;
              if(dir_ent->inum != i)
                return 1;
            }

            // if parent dir has not been found yet
            else if(p_flag == 0 && strcmp(dir_ent->name, "..") == 0){
              p_flag = 1;
              if(i != 1 && find_child_dir(img_buffer, (struct dinode *) (img_buffer + (inodestart * BSIZE) + (dir_ent->inum * sizeof(struct dinode))), i)){
                fprintf(stderr, "ERROR: parent directory mismatch.\n");
                exit(1);
              }
            }
          }
        }
      }

      // if "." or ".." directory entry was not found
      if(c_flag == 0 || p_flag == 0)
        return 1;
    }
  }
  return 0;
}

// test 5 -
// For in-use inodes, each address in use is also marked in use in the bitmap.
// If not, print ERROR: address used by inode but marked free in bitmap.
int test_inode_bitmap(char *img_buffer, struct superblock *sb, struct dinode *inode)
{
  uchar *bitmap;
  uint *indirect_block;
  uchar bmap_bit;

  // for all inodes
  for(int i = 0; i < sb->ninodes; i++, inode++){
    // skip inodes that are not in use
    if(inode->type == 0)
      continue;

    // pointer to start of bitmap
    // (img_buffer) + (start sector of bitmap * block size)
    bitmap = (uchar *) (img_buffer + ((sb->bmapstart) * BSIZE));

    // for all direct datablock addresses
    for(int i = 0; i < (NDIRECT + 1); i++){
      // skip entries in addrs that are not in use
      if(inode->addrs[i] == 0)
        continue;

      // find corresponding bit of sector in bitmap
      // (byte bit is in) & (corresponding bitmask to isolate bit) >> (shift by this much)
      bmap_bit = ((*(bitmap + inode->addrs[i]/8)) & (bitmask[inode->addrs[i]%8])) >> (inode->addrs[i]%8);
      //printf("%d:%d\n", inode->addrs[i], bmap_bit);
      if(!bmap_bit)
        return 1;
    }

    // for all indirect addresses if indirect block exists
    if(inode->addrs[NDIRECT] != 0){
      // check if indirect datablock is marked as in use in bitmap
      bmap_bit = ((*(bitmap + inode->addrs[NDIRECT]/8)) & (bitmask[inode->addrs[NDIRECT]%8])) >> (inode->addrs[NDIRECT]%8);
      if(!bmap_bit)
        return 1;

      // calculate address of indirect block
      indirect_block = (uint *) (img_buffer + (inode->addrs[NDIRECT] * BSIZE));
      // for all possible indirect addresses
      for(int i = 0; i < NINDIRECT; i++, indirect_block++){
        // skip if data block is not in use
        if(*indirect_block == 0)
          continue;

        // check if datablock is marked in use in bitmap
        bmap_bit = ((*(bitmap + *indirect_block/8)) & (bitmask[*indirect_block%8])) >> (*indirect_block%8);
        //printf("%d:%d\n", *indirect_block, bmap_bit);
        if(!bmap_bit)
          return 1;
      }
    }
  }

  return 0;
}

// test 6 -
// For blocks marked in-use in bitmap, the block should actually be in-use in an inode or indirect block somewhere.
// If not, print ERROR: bitmap marks block in use but it is not in use.
int test_bitmap(char *img_buffer, uchar *bmap_log, uchar *inode_log, uint size, uint nblocks, uint bmapstart)
{
  // pointer to start of bitmap
  // (img_buffer) + (start sector of bitmap * block size)
  uchar *bitmap = (uchar *) (img_buffer + ((bmapstart) * BSIZE));
  // start of data blocks
  uint dblockstart = size - nblocks;

  for(int i = dblockstart; i < size; i++){
    bmap_log[i-dblockstart] = ((*(bitmap + i/8)) & (bitmask[i%8])) >> (i%8);
  }

  for(int i = 0; i < nblocks; i++){
    if(bmap_log[i] != inode_log[i])
      return 1;
  }
  return 0;
}

// test 7 -
// For in-use inodes, each direct address in use is only used once.
// If not, print ERROR: direct address used more than once.
int test_diraddrs(uchar *diradd_log, uint nblocks)
{
  for(int i = 0; i < nblocks; i++){
    if(diradd_log[i] > 1)
      return 1;
  }
  return 0;
}

// test 8 -
// For in-use inodes, each indirect address in use is only used once.
// If not, print ERROR: indirect address used more than once.
int test_indaddrs(uchar *indadd_log, uint nblocks)
{
  for(int i = 0; i < nblocks; i++){
    if(indadd_log[i] > 1)
      return 1;
  }
  return 0;
}

// test 9 -
// For all inodes marked in use, each must be referred to in at least one directory.
// If not, print ERROR: inode marked use but not found in a directory.
int test_dirs(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)
{
  for(int i = 0; i < ninodes; i++, inode++){
    if(inode->type != 0 && dir_log[i] == 0)
      return 1;
  }
  return 0;
}

// test 10 -
// For each inode number that is referred to in a valid directory, it is actually marked in use.
// If not, print ERROR: inode referred to in directory but marked free.
int test_inode_dir(uchar *inodenum_log, uchar *dir_log, uint ninodes)
{
  for(int i = 0; i < ninodes; i++){
    if((inodenum_log[i] == 0 && dir_log[i] >= 1) || (inodenum_log[i] == 1 && dir_log[i] < 1))
      return 1;
  }
  return 0;
}

// test 11 -
// Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly).
// If not, print ERROR: bad reference count for file.
int test_references(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)
{
  for(int i = 0; i < ninodes; i++, inode++){
    if(inode->type == T_FILE && inode->nlink != dir_log[i])
      return 1;
  }
  return 0;
}

// test 12 -
// No extra links allowed for directories (each directory only appears in one other directory).
// If not, print ERROR: directory appears more than once in file system.
int test_links(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)
{
  for(int i = 0; i < ninodes; i++, inode++){
    if(inode->type == T_DIR && dir_log[i] > 1)
      return 1;
  }
  return 0;
}

int xcheck(char *filename)
{
  struct stat statbuf;
  int file_ptr;
  char *img_buffer;

  // obtain stat of file
  if(stat(filename, &statbuf) != 0){
    fprintf(stderr, "File %s stat error.\n", filename);
    exit(1);
  }

  // open file
  if((file_ptr = open(filename, O_RDONLY)) == -1){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  // use mmap to access file-system image
  if((img_buffer = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, file_ptr, 0)) == MAP_FAILED){
    fprintf(stderr, "File %s mmap error.\n", filename);
    exit(1);
  }

  // close file
  if(close(file_ptr) == -1){
    exit(1);
  }

  // superblock is on block 1
  struct superblock *sb = (struct superblock *) (img_buffer + BSIZE);

  // inode variable points to start of inodes
  struct dinode *inode = (struct dinode *) (img_buffer + (sb->inodestart * BSIZE));

  // initialize arrays to keep track of inodes, bmap, etc.
  uchar bmap_log[sb->nblocks];
  uchar inode_log[sb->nblocks];
  uchar diradd_log[sb->nblocks];
  uchar indadd_log[sb->nblocks];
  uchar inodenum_log[sb->ninodes];
  uchar dir_log[sb->ninodes];

  // set all elements of array to be zero
  memset(inode_log, 0, sb->nblocks);
  memset(bmap_log, 0, sb->nblocks);
  memset(diradd_log, 0, sb->nblocks);
  memset(indadd_log, 0, sb->nblocks);
  memset(inodenum_log, 0, sb->ninodes);
  memset(dir_log, 0, sb->ninodes);

  // test 1
  if(test_inode_type(inode, sb->ninodes)){
    fprintf(stderr, "ERROR: bad inode.\n");
    exit(1);
  }

  // test 2
  test_inode_addrs(img_buffer, sb, inode, inode_log, diradd_log, indadd_log, inodenum_log);

  // test 3
  if(test_root(img_buffer, (struct dinode *) ((img_buffer + (sb->inodestart * BSIZE)) + sizeof(struct dinode)))){
    fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }

  // if test_root did not raise error, mark root node as in use
  dir_log[1] = 1;

  // test 4
  if(test_dir_format(img_buffer, inode, dir_log, sb->ninodes, sb->inodestart)){
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }

  // test 5
  if(test_inode_bitmap(img_buffer, sb, inode)){
    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }

  // test 6
  if(test_bitmap(img_buffer, bmap_log, inode_log, sb->size, sb->nblocks, sb->bmapstart)){
    fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
    exit(1);
  }

  // test 7
  if(test_diraddrs(diradd_log, sb->nblocks)){
    fprintf(stderr, "ERROR: direct address used more than once.\n");
    exit(1);
  }

  // test 8
  if(test_indaddrs(indadd_log, sb->nblocks)){
    fprintf(stderr, "ERROR: indirect address used more than once.\n");
    exit(1);
  }

  // test 9
  if(test_dirs(img_buffer, inode, dir_log, sb->ninodes, sb->inodestart)){
    fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
    exit(1);
  }

  // test 10
  if(test_inode_dir(inodenum_log, dir_log, sb->ninodes)){
    fprintf(stderr, "ERROR: inode reffered to in directory but marked free.\n");
    exit(1);
  }

  // test 11
  if(test_references(img_buffer, inode, dir_log, sb->ninodes, sb->inodestart)){
    fprintf(stderr, "ERROR: bad reference count for file.\n");
    exit(1);
  }

  // test 12
  if(test_links(img_buffer, inode, dir_log, sb->ninodes, sb->inodestart)){
    fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
    exit(1);
  }

  // unmap file from memory
  munmap(img_buffer, statbuf.st_size);

  return 0;
}

// extra credit 3 -
// repair
int repair_fs(char *filename)
{
  struct stat statbuf;
  int file_ptr;
  char *img_buffer;
  uint *indirect_block;
  struct dirent *dir_ent;
  struct dinode *lost_found;
  int flag = 0;

  // obtain stat of file
  if(stat(filename, &statbuf) != 0){
    fprintf(stderr, "File %s stat error.\n", filename);
    exit(1);
  }

  // open file
  if((file_ptr = open(filename, O_RDWR)) == -1){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  // use mmap to access file-system image
  if((img_buffer = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_ptr, 0)) == MAP_FAILED){
    fprintf(stderr, "File %s mmap error.\n", filename);
    exit(1);
  }

  // close file
  if(close(file_ptr) == -1){
    exit(1);
  }

  // superblock is on block 1
  struct superblock *sb = (struct superblock *) (img_buffer + BSIZE);

  // inode variable points to start of inodes
  struct dinode *inode = (struct dinode *) (img_buffer + (sb->inodestart * BSIZE));

  uchar *bitmap = (uchar *) (img_buffer + ((sb->bmapstart) * BSIZE));

  // root inode is second inode
  struct dinode *root = (struct dinode *) ((img_buffer + (sb->inodestart * BSIZE)) + sizeof(struct dinode));

  // for all direct data block
  for(int i = 0; i < NDIRECT; i++){
    // skip if data block is not in use
    if(root->addrs[i] == 0)
      continue;

    dir_ent = (struct dirent *) (img_buffer + (root->addrs[i] * BSIZE));

    // for all directory entries in data block
    for(int j = 0; j < (BSIZE/sizeof(struct dirent)); j++, dir_ent++){
      if(flag == 0 && strcmp(dir_ent->name, "lost_found") == 0){
        flag = 1;
        lost_found = (struct dinode *) ((img_buffer + (sb->inodestart * BSIZE)) + (dir_ent->inum * sizeof(struct dinode)));
      }
      if(flag)
        break;
    }
    if(flag)
      break;
  }

  // if indirect block is in use
  if(flag == 0 && root->addrs[NDIRECT] != 0){
    // calculate address of indirect block
    indirect_block = (uint *) (img_buffer + (root->addrs[NDIRECT] * BSIZE));

    // for all indirect data blocks
    for(int j = 0; j < NINDIRECT; j++, indirect_block++){
      // skip if data block is not in use
      if(*indirect_block == 0)
        continue;

      // initialize dir_ent as first directory entry in data block
      dir_ent = (struct dirent *) (img_buffer + (*indirect_block * BSIZE));

      // for all directory entries in data block
      for(int k = 0; k < (BSIZE/sizeof(struct dirent)); k++, dir_ent++){
        if(flag == 0 && strcmp(dir_ent->name, "lost_found") == 0){
          flag = 1;
          lost_found = (struct dinode *) ((img_buffer + (sb->inodestart * BSIZE)) + (dir_ent->inum * sizeof(struct dinode)));
        }
        if(flag)
          break;
      }
      if(flag)
        break;
    }
  }

  // lost_found not found in root directory
  if(flag == 0){
    fprintf(stderr, "ERROR: lost_found directory does not exist.\n");
    exit(1);
  }

  uchar dir_log[sb->ninodes];
  memset(dir_log, 0, sb->ninodes);
  dir_log[1] = 1;

  // test 4
  if(test_dir_format(img_buffer, inode, dir_log, sb->ninodes, sb->inodestart)){
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }

  // for all inodes
  for(int i = 0; i < sb->ninodes; i++, inode++){
    // if inodes is marked in use but not found in a directory
    if(inode->type != 0 && dir_log[i] == 0){
      flag = 0;
      // for all direct data block
      for(int j = 0; j < NDIRECT; j++){
        dir_ent = (struct dirent *) (img_buffer + (lost_found->addrs[j] * BSIZE));

        // for all directory entries in data block
        for(int k = 0; k < (BSIZE/sizeof(struct dirent)); k++, dir_ent++){
          // find a empty directory entry to create new directory entry
          if(dir_ent->inum == 0){
            flag = 1;
            // set inum of new directory entry to be lost inode
            dir_ent->inum = i;
            // placeholder name
            dir_ent->name[0] = 76;
            // set bit in bitmap to be 1 if 0
            if(((*(bitmap + lost_found->addrs[j]/8)) & (bitmask[lost_found->addrs[j]%8])) >> (lost_found->addrs[j]%8) == 0){
              if(lost_found->addrs[j] % 8 == 0)
                *(bitmap + lost_found->addrs[j]/8) = *(bitmap + lost_found->addrs[j]/8) | (1 << (lost_found->addrs[j]%8));
              else{
                *(bitmap + lost_found->addrs[j]/8) = *(bitmap + lost_found->addrs[j]/8) | (1 << ((lost_found->addrs[j]%8) - 1));
              }
            }
          }
          if(flag)
            break;
        }
        if(flag)
          break;
      }

      // if indirect block is in use
      if(flag == 0 && lost_found->addrs[NDIRECT] != 0){
        // calculate address of indirect block
        indirect_block = (uint *) (img_buffer + (lost_found->addrs[NDIRECT] * BSIZE));

        // for all indirect data blocks
        for(int j = 0; j < NINDIRECT; j++, indirect_block++){
          // skip if data block is not in use
          if(*indirect_block == 0)
            continue;

          // initialize dir_ent as first directory entry in data block
          dir_ent = (struct dirent *) (img_buffer + (*indirect_block * BSIZE));

          // for all directory entries in data block
          for(int k = 0; k < (BSIZE/sizeof(struct dirent)); k++, dir_ent++){
            // find a empty directory entry
            if(dir_ent->inum == 0){
              flag = 1;
              dir_ent->inum = i;
              dir_ent->name[0] = 76;
              if(((*(bitmap + *indirect_block/8)) & (bitmask[*indirect_block%8])) >> (*indirect_block%8) == 0){
                if(*indirect_block % 8 == 0)
                  *(bitmap + *indirect_block/8) = *(bitmap + *indirect_block/8) | (1 << (*indirect_block%8));
                else{
                  *(bitmap + *indirect_block/8) = *(bitmap + *indirect_block/8) | (1 << ((*indirect_block%8) - 1));
                }
              }
            }
            if(flag)
              break;
          }
          if(flag)
            break;
        }
      }

      if(flag == 0){
        fprintf(stderr, "ERROR: lost_found directory is full.\n");
        exit(1);
      }
    }
  }

  // unmap file from memory
  munmap(img_buffer, statbuf.st_size);

  return 0;
}

// main function
int main(int argc, char **argv)
{
  // exit if no image file is provided
  if(argc < 2){
    fprintf(stderr, "Usage: xcheck <file_system_image>\n");
    exit(1);
  }

  // extra credit 3
  if(strcmp(argv[1], "-r") == 0){
    // if no image file is provided
    if(argc < 3){
      fprintf(stderr, "Usage: xcheck -r <image_to_be_repaired>\n");
      exit(1);
    }
    // call function to repair error if it exists
    repair_fs(argv[2]);
  }

  else{
    // call file system checker
    xcheck(argv[1]);
  }

  return 0;
}
