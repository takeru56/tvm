#!/bin/sh

assert() {
  expected="$1"
  input="$2"

  ./tvm "$input"
  actual=$?

  ESC=$(printf '\033')
  if [ "$actual" = "$expected" ]; then
    printf "${ESC}[32m%s${ESC}[m\n" "[PASS] $input => $actual"
  else
    printf "${ESC}[31m%s${ESC}[m\n" "[FAIL] $input => $expected expected, but got $actual"
  fi
}

echo "# TESTCASES"
echo "## Calculator Test"
assert 2   "00000100000101"
assert 255 "0000FF00000001"
assert 1   "0000FF0000FE02" 
assert 160 "00000A00001003"
assert 1   "00FFFF00FFFF04"
echo "DONE"

