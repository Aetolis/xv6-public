# Project 7 - File System Checker

## A Basic Checker

File system checker program can be compiled using `make xcheck`. Call file system checker program by invoking `xcheck file_system_image`.

Each test for the file system checker `xcheck` was implemented in the following functions:
  1. Test 1: `test_inode_type(struct dinode *inode, uint ninodes)`
  2. Test 2: `test_inode_addrs(char *img_buffer, struct superblock *sb, struct dinode *inode, uchar *inode_log, uchar *diradd_log, uchar *indadd_log, uchar *inodenum_log)`
  3. Test 3: `test_root(char *img_buffer, struct dinode *inode)`
  4. Test 4: `test_dir_format(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)`
  5. Test 5: `test_inode_bitmap(char *img_buffer, struct superblock *sb, struct dinode *inode)`
  6. Test 6: `test_bitmap(char *img_buffer, uchar *bmap_log, uchar *inode_log, uint size, uint nblocks, uint bmapstart)`
  7. Test 7: `test_diraddrs(uchar *diradd_log, uint nblocks)`
  8. Test 8: `test_indaddrs(uchar *indadd_log, uint nblocks)`
  9. Test 9: `test_dirs(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)`
  10. Test 10: `test_inode_dir(uchar *inodenum_log, uchar *dir_log, uint ninodes)`


## A Better Checker

Call repair operation using file system checker program by using `-r` flag and specifying image to be repaired directly after. For example, `xcheck -r image_to_be_repaired`.

  1. Test 11: `test_references(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)`
  2. Test 12: `test_links(char *img_buffer, struct dinode *inode, uchar *dir_log, uint ninodes, uint inodestart)`
  3. Extra credit 1: `find_child_dir(char *img_buffer, struct dinode *inode, ushort cur_inum)`
  4. Extra credit 3: `repair_fs(char *filename)`
