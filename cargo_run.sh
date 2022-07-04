export LLVM=/usr/lib/llvm-13
export TARGET_PROGRAM=./target/thread_pool/src/main.rs
export TARGET_HOME_DIR=inputs/rayon_41i0019
export RUSTFLAGS=--emit=llvm-ir

rm -rf build
mkdir build
cd build

clang -pthread -c ../lib/inttrace.c -o inttrace.o
cmake -DLT_LLVM_INSTALL_DIR=$LLVM ../HelloWorld
make

cd ../$TARGET_HOME_DIR
cargo clean
cargo -v build # rust sets default opt-level for dev is 0.
cnt_entry=0
for entry in target/debug/deps/*.ll
do 
	cp $entry ../../build/
	((cnt_entry=cnt_entry+1))
done

cd ../../build

for entry in *.ll
do
	$LLVM/bin/opt -o out_$entry -load-pass-plugin ./libHelloWorld.so -passes=hello-world $entry
done

$LLVM/bin/clang -v -no-pie out_*.ll -L /usr/lib/rustlib/x86_64-unknown-linux-gnu/lib/ -lstd-100ac2470628c6dd -ltest-d3d2dadbc02750a3 inttrace.o -pthread -lm -ldl
