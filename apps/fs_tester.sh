#!/bin/bash

set -o pipefail

log() {
    echo -e "${*}"
}

inf() {
    log "Info: ${*}"
}

error() {
    log "Error: ${*}"
}

die() {
    error "${*}"
    exit 1
}

compare_output() {
    # 1: actual output
    # 2: expected output
    if [ "${1}" != "${2}" ]; then
        error "Test failed: Expected '${2}', got '${1}'"
        return 1
    else
        inf "Test passed: '${1}'"
        return 0
    fi
}

run_test() {
    local test_cmd="${1}"
    local expected_output="${2}"

    local actual_output
    actual_output=$(${test_cmd})

    compare_output "${actual_output}" "${expected_output}"
}

make_fs() {
    ./fs_make.x test.fs 100 || die "Failed to create file system"
}

clean_fs() {
    rm -f test.fs
}

test_mount_unmount() {
    inf "Testing mount and unmount"
    make_fs
    ./test_fs.x info test.fs > /dev/null || die "Failed to mount"
    ./test_fs.x umount test.fs > /dev/null || die "Failed to unmount"
    clean_fs
}

test_create_file() {
    inf "Testing file creation"
    make_fs
    ./test_fs.x create test.fs newfile.txt > /dev/null || die "Failed to create file"
    local expected="file: newfile.txt, size: 0, data_blk: 65535"
    run_test "./test_fs.x ls test.fs" "${expected}"
    ./test_fs.x umount test.fs > /dev/null
    clean_fs
}

test_delete_file() {
    inf "Testing file deletion"
    make_fs
    ./test_fs.x create test.fs newfile.txt > /dev/null || die "Failed to create file"
    ./test_fs.x delete test.fs newfile.txt > /dev/null || die "Failed to delete file"
    local expected="FS Ls:"
    run_test "./test_fs.x ls test.fs" "${expected}"
    ./test_fs.x umount test.fs > /dev/null
    clean_fs
}

# Add more tests here...

main() {
    test_mount_unmount
    test_create_file
    test_delete_file
    # Call other test functions here...
}

main
