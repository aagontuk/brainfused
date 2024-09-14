### Building the interpreter ###

To build the interpreter run `make`

### Running the interpreter ###

To run with switch/case version of the interpreter:

```
./bfi --interp BF_FILE
```

To run with [computed goto](https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables) version of the interpreter:

```
./bfi --cgoto BF_FILE
```
