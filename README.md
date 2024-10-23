# PL/0 Compiler


## Description

    This project is an implementation of a Recursive Descent Parser and Intermediate
    Code Generator for the programming language PL/0. It builds off of the lexical analyzer
    that takes in the source code and tokenizes it. If a program does not follow the correct
    grammar conventions of PL/0, then a message will be outputted indicating the type
    of error present. This project uses the full PL/0 grammar rather than just a subset.

---------------------------------------

## Compilation Instructions

    Compile the code in terminal using command "gcc hw4compiler.c"

---------------------------------------

## Usage

    Run the compiled code using "./a.out [arguments]"

    # It is assumed that the argument for this program is a .txt file
    # containing a program written in PL/0.

---------------------------------------

## Example

    Example command:
        ./a.out input.txt

    Expected output:
        Should output the PL/0 program source code and then corresponding assembly instructions, as
        well as a file called elf.txt containing the assembly code that can be run in the VM.
        If there is an error, then only a message indicating the type of error that has occurred
        will be outputted.

---------------------------------------
