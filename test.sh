#!/bin/sh
compiler_path=../../../go/1.14.6/src/github.com/takeru56/tcompiler

assert() {
  # arguments
  source=$1
  expected=$2
  # compile to bytecode
  go run $compiler_path "$source" > test.ir
  bytecode=$(<test.ir)

  # execute interpleter
  ./tvm
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
  go run $compiler_path $source > test.ir
  bytecode=$(<test.ir)

  # execute interpleter
  ./tvm
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

# echo "# TESTCASES"
# # echo "## Calculator Test"
# assert '1+1' 2
# assert '255+0' 255
# assert '255-254' 1
# assert '16*10' 160
# assert '65025/65025' 1
# assert '1==1' 1
# assert '1!=1' 0
# not_eq '1==1' 0
# not_eq '1!=1' 1
# assert '2>1' 1
# assert '1<2' 1
# assert '1>300' 0
# assert '230<33' 0
# echo "## Variable Test"
# assert 'a=1 a+3' 4
# assert '1*2' 2
# assert 'a=1 b=2 3+a*b' 5
# assert 'a = 2 b = 3 if 1 > 2 do b = 5 end a*b' 6
# assert 'a = 1 if 2 > 1 do a=3 end a*3' 9
# assert 'a = 1 if 4 > a do a = a+1 end if 2 > 1 do a=a+3 end a*3' 15
# assert 'a = 1 while 5 > a do a=a+1 end a' 5
echo "## Function Test"
assert 'hoge=3 def myFunc() a = 1 while 5 > a do a=a+1 end end hoge' 3
assert 'def myFunc() a = 1 b=2 return 3+a*b end myFunc()' 5
assert 'def myFunc() a = 1 b=2 return 3+a*b end 2 * myFunc()' 10
assert '
def hoge()
  return 1+1
end

def fuga()
  return 2+2-hoge()
end

return hoge()+fuga()*3' 8
assert '
def sum(a, b)
  return a+b
end

sum(3,6)' 9

assert '
def fibo(a)
  if a == 0 do
    return 0
  end
  if a == 1 do
    return 1
  end
  return fibo(a-1)+fibo(a-2)
end

fibo(10)' 55

assert '
class Motor
  def on(a)
    self.hoge = a*3
    return 5*a
  end

  def off()
    return self.hoge
  end
end

class LED
  def on(a, b)
    return a*b
  end

  def off()
  end
end

a = Motor()
b = Motor()
a.on(5)
b.on(2)
b.off()' 6

echo "DONE"

rm tvm
