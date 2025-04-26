#!/bin/bash

cd "../build"
make -j32 || exit $?
cd ../tests
cp ../build/masm tmp