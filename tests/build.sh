#!/bin/bash

cd "../build"
cmake -build . || exit $?
cd ../tests
cp ../build/masm tmp