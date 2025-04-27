mkdir tmp
./build.sh # build for creating files
cd code
../tmp/masm -c "$1.masm" "$1.expected"
../tmp/masm -i "$1.expected"
cd ..
rm -rf tmp