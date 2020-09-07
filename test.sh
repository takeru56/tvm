#!/bin/sh

assert() {
  # arguments
  source=$1
  expected=$2

  # compile to bytecode
  go run ~/go/1.14.6/src/github.com/takeru56/t $source > out.tt
  bytecode=$(<out.tt)

  # execute interpleter
  ./tvm "$bytecode"
  actual=$?

  # output results
  ESC=$(printf '\033')
  if [ "$actual" = "$expected" ]; then
    printf "${ESC}[32m%s${ESC}[m\n" "[PASS] $input => $actual"
  else
    printf "${ESC}[31m%s${ESC}[m\n" "[FAIL] $input => $expected expected, but got $actual"
  fi
}

echo "# TESTCASES"
echo "## Calculator Test"
assert 1+1 2
assert 255+0 255
assert 255-254 1
assert 16*10 160
assert 65025/65025 1
echo "DONE"

rm out.tt
