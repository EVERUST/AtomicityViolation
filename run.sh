export LLVM_DIR=/usr/lib/llvm-13

# which input
file_type='rust'

# input c file
filename=sum

# input rust file
filename_r=tokio_59c0044  #grin_24i0013 ## atomic # simple_atomicity #rust_test # grin_24c0017 # tokio_59c0044 # grin_24i0013 #crossbeam_10c0012 #rwlock #arc_rwlock #condvar #one_mutex #other_types # rust_test # multi_mutex

rm -rf build
mkdir build
cd build

clang -pthread -c ../lib/inttrace.c -o inttrace.o
#clang -O0 -S -emit-llvm ../lib/inttrace.c -o inttrace.ll
#clang -O0 -S -emit-llvm ../lib/inttrace.c -target x86_64-unknown-linux-gnu -o inttrace.ll
#clang -O0 -S -emit-llvm ../lib/inttrace.c -o inttrace.bc
#rustc --emit=llvm-ir ../lib/inttrace.rs
#rustc --emit=llvm-ir ../lib/inttrace.rs

#rustc --crate-type staticlib ../lib/linker.rs


cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../HelloWorld/
make


if [[ $file_type = 'c' ]] ## c
then
	
	$LLVM_DIR/bin/clang -fno-discard-value-names -g -O0 -S -emit-llvm ../inputs/$filename.c -o $filename.ll
	$LLVM_DIR/bin/opt -o out.ll -load-pass-plugin ./libHelloWorld.so -passes=hello-world $filename.ll
	$LLVM_DIR/bin/clang out.ll inttrace.o

else ## rust
	
	rustc --emit=llvm-ir -g -C opt-level=0 ../inputs/$filename_r.rs
	$LLVM_DIR/bin/opt -o out.ll -load-pass-plugin ./libHelloWorld.so -passes=hello-world $filename_r.ll
	# add stdlib of rust
	$LLVM_DIR/bin/clang -L/usr/lib/rustlib/x86_64-unknown-linux-gnu/lib/ -lstd-100ac2470628c6dd out.ll inttrace.o -pthread

fi

#./a.out
#cd ..
##python3 atom.py


##### FROM intwrite tutorial
#clang -c -g -Xclang -load -Xclang ./libHelloWorld.so ../inputs/sum.c
#clang -o sum sum.o inttrace.o
#./sum

##### FOR Hello World tutorial
#$LLVM_DIR/bin/clang -O1 -S -emit-llvm ../inputs/input_for_hello.c -o input_for_hello.ll
#$LLVM_DIR/bin/opt -load-pass-plugin ./libHelloWorld.so -passes=hello-world -disable-output input_for_hello.ll



