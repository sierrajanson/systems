#!/usr/bin/env bash

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

thread_count=1
push_per_thread=1

if [[ `check_dir` -eq 1 ]]; then
    exit 1
fi

cfile=push_order.c

cp queue_users/$cfile .
clang -Wall -Werror -Wextra -Wpedantic -Wstrict-prototypes -DDEBUG -o queue_test queue.o $cfile -lpthread

./queue_test $thread_count $push_per_thread >output.txt 2>error.txt
rc=$?

new_files="$cfile queue_test output.txt error.txt"

if [[ $rc -eq 0 ]]; then
    echo "It worked!"
else
    echo "--------------------------------------------------------------------------------"
    echo "return code: $rc"
    echo "--------------------------------------------------------------------------------"
    echo "output:"
    cat output.txt
    echo "--------------------------------------------------------------------------------"
    echo "error:"
    cat error.txt
    echo "--------------------------------------------------------------------------------"

fi

cleanup $new_files
exit $rc
