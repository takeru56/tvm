#!/bin/sh

assert() {
  # arguments
  source=$1
  expected=$2

  # compile to bytecode
  go run ~/go/1.14.6/src/github.com/takeru56/t "$source" > out.tt
  bytecode=$(<out.tt)

  # execute interpleter
  ./tvm "$bytecode"
  actual=$?

  # output results
  ESC=$(printf '\033')
  if [ "$actual" = "$expected" ]; then
    printf "${ESC}[32m%s${ESC}[m\n" "[PASS] $source => $actual"
  else
    printf "${ESC}[31m%s${ESC}[m\n" "[FAIL] $source => $expected expected, but got $actual"
  fi
}

not_eq() {
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
    printf "${ESC}[31m%s${ESC}[m\n" "[FAIL] $source => $expected expected, but got $actual"
  else
    printf "${ESC}[32m%s${ESC}[m\n" "[PASS] $source => $actual, not $expected"
  fi
}

# compile
make tvm

echo "# TESTCASES"
echo "## Calculator Test"
assert '1+1' 2
assert '255+0' 255
assert '255-254' 1
assert '16*10' 160
assert '65025/65025' 1
assert '1==1' 1
assert '1!=1' 0
not_eq '1==1' 0
not_eq '1!=1' 1
assert '2>1' 1
assert '1<2' 1
assert '1>300' 0
assert '230<33' 0
echo "## Variable Test"
assert 'a=1 a+3' 4
assert 'a = 3 b = 10 a*b' 30
assert 'if 2 > 1 do b=1+1 end b=b+1 a = 1 a*b' 3

echo "DONE"

rm tvm
rm out.tt
