### Building the interpreter ###

To build the interpreter and the compiler run `make`

### Running the interpreter ###

To run with switch/case version of the interpreter:

```
./bfi --interp BF_FILE
```

To run with [computed goto](https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables) version of the interpreter:

```
./bfi --cgoto BF_FILE
```

To run with the profiler:

```
./bfi -i -p BF_FILE
```

### Running the compiler AOT ###

Currently the compiler emits x86_64 assembly. Here are steps to compile and run BF code on x86_64 GNU/Linux machine:

```
./bfc --aot mandel.bf mandel.asm
nasm -f elf64 -o mandel.o mandel.asm
ld -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o mandel mandel.o -lc
```

### Running the compiler JIT ###

To run the JIT compiler:

```
./bfc --jit mandel.bf
```
