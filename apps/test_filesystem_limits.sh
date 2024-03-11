#!/bin/bash

./fs_make.x test.fs 10

cat <<END_SCRIPT > too_many_file.script
MOUNT
CREATE file1
CREATE file2
CREATE file3
CREATE file4
CREATE file5
CREATE file6
CREATE file7
CREATE file8
CREATE file9
CREATE file10
CREATE file11  
UMOUNT
END_SCRIPT

./test_fs.x script test.fs too_many_file.script


cat <<END_SCRIPT > too_many_data.script
MOUNT
CREATE bigfile
OPEN bigfile
WRITE 1 DATA "0123456789abcdef" 
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"
WRITE 1 DATA "0123456789abcdef"  
CLOSE 1
UMOUNT
END_SCRIPT


./test_fs.x script test.fs too_many_data.script

rm test.fs
