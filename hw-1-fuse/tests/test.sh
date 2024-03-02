#!/bin/bash

cp -r $1 $2
cd $2

# test directory simple
mkdir test01 
if [ ! -d "test01" ]; then
    echo "directory test failed"
    exit -1
fi

touch test01/test.txt

mv test01 another_test01

if [ -d "tes01" ]; then 
    echo "directory test failed"
fi 
if [ ! -d "another_test01" ]; then
    echo "directory test failed"
    exit -1
fi
if [ ! -f "another_test01/test.txt" ]; then
    echo "directory test failed"
    exit -1
fi

rm -rf another_test01


# test file simple 
mkdir test02 
touch test02/test.txt
echo "simple test of course" > test02/test.txt 

if ! grep -Fxq "simple test of course" test02/test.txt
then
    echo "file test failed"
    exit -1
fi

rm -rf test02


# test with project
cd tests/test_project
mkdir build 
cd build 
if ! cmake .. 
then 
    echo "project test failed" 
    exit -1
fi

if ! make 
then 
    echo "project test failed" 
    exit -1
fi

if ! ./test 
then 
    echo "project test failed" 
fi

cd $2
rm -rf tests

echo "all test passed" 
exit 0

