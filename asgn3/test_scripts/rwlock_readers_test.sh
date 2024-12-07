#!/usr/bin/env bash

source_dir=`dirname ${BASH_SOURCE}`
source "$source_dir/utils.sh"

priority=READERS
n=0
num_readers=2
num_writers=2
test_time=1

ANY_RD=1
ONE_WR=2
STRESS=4
TPUT=8

if [[ `check_dir` -eq 1 ]]; then
    exit 1
fi

cfile=rwlock_test.c

cp rwlock_users/$cfile .
clang -Wall -Werror -Wextra -Wpedantic -Wstrict-prototypes -DDEBUG -o rwlock_test rwlock.o $cfile -lpthread

./rwlock_test $priority $n $num_readers $num_writers $test_time >output.txt 2>error.txt
rc=$?

new_files="$new_files rwlock_test output.txt error.txt"

if [ $(($rc & $ANY_RD)) -eq 0 ]; then
    echo "It worked!"
    rc=0
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
