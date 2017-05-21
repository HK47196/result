#! /bin/sh
#
# run_tests.sh
# Copyright (C) 2017 rsw0x
#
# Distributed under terms of the MPLv2 license.
#


FILES=compile_tests/*.cxx
count=0

for f in $FILES; do
  if [ ! -e $f ]; then
    tput setaf 1; tput bold; echo "${f} doesn't exist?"; tput sgr0
    exit 2
  fi
  if c++ -I. -std=c++14 $f -c -o /dev/null 2>/dev/null; then
    tput setaf 1; tput bold; echo "${f} shouldn't have compiled, but it did. ERROR."; tput sgr0
    exit 1
  fi
  ((count++))
done
tput setaf 2; tput bold; echo "OK, ${count} tests passed."; tput sgr0
exit 0
