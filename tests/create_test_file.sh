mkdir tmp
./build.sh # build for creating files
./tmp/masm -c "$1.masm" "$1.expected" -g
./tmp/masm -i "$1.expected"
rm -rf tmp