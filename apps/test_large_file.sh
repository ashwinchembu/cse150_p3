#!/bin/bash

dd if=/dev/urandom of=large_file bs=4096 count=10

./fs_make.x test.fs 100
./fs_ref.x add test.fs large_file

cat <<END_SCRIPT > read_large_file.script
MOUNT
OPEN    large_file
READ    40960   FILE    large_file
UMOUNT
END_SCRIPT

./test_fs.x script test.fs read_large_file.script

rm large_file
rm read_large_file.script
rm test.fs
