BENCH_DIR=../brainfuck-benchmark/benches
COMP_LLVM_BIN=./build/bf_llvm_comp
COMP_AOT_BIN=../bfc

OUTDIR=./bench_dir
rm -rf $OUTDIR
mkdir $OUTDIR

# list all files in the benchmark directory
for file in $BENCH_DIR/*; do
  echo "Benchmarking $(basename $file)"
  echo
  echo "LLVM Compilation"
  # remove extention from the file name
  filename=$(basename $file .b)
  # compile the file to llvm ir
  $COMP_LLVM_BIN $file > ${OUTDIR}/${filename}.ll
  # compile the llvm ir to an object file
  llc -filetype=obj -relocation-model=pic ${OUTDIR}/${filename}.ll -o ${OUTDIR}/${filename}.o
  # link the object file to an executable
  clang ${OUTDIR}/${filename}.o -o ${OUTDIR}/${filename}
  time ${OUTDIR}/${filename} > /dev/null
  rm ${OUTDIR}/${filename}.o
  rm ${OUTDIR}/${filename}
  
  echo
  echo "AOT Compilation"
  $COMP_AOT_BIN --aot $file ${OUTDIR}/${filename}.s
  nasm -f elf64 -o ${OUTDIR}/${filename}.o ${OUTDIR}/${filename}.s
  ld -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o ${OUTDIR}/${filename} ${OUTDIR}/${filename}.o -lc
  time ${OUTDIR}/${filename} > /dev/null
  echo
done
