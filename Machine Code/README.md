## Generating object code

The easiest way to generate a sequence of instructions would be to use objdump on the object file. Generating sequence of instructions for the POWER architecture
is a bit difficult given the fact that most machines run on the x86 architecture, if you are on a POWER system you can generate the instructions easily. If you are
on any other machine and want to disassemble the object code (for a highlevel c code) for a power architecture you can use the below commands.

```
powerpc64le-linux-gnu-gcc temp.c -g -nostdlib -o power
powerpc64le-linux-gnu-objdump -dS power
```
For example we are on an x86 machine and we want to generate the sequence of instructions of a source code for the power architecture. The first command will generate
the object code in a file called "power". We then dissassemble it using the objdump command.
Note that we didnt use any standard libraries to make it easier for us to read the disassembled code, and thats why we used nostdlib when we compiled.
