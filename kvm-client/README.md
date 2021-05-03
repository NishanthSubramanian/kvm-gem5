# kvm-gem5

There are 3 folders, Each folder contains a kvm client

1.  x86-kvm-client.c --> x86 kvm client
    guest-code.c --> guest code executed by kvm-client.c

To run:

```
make .
```

2.  kvm-asm.c --> Another x86 client that executes assemble code directly

To run:

```
gcc kvm-asm.c
```

3.  kvm_latest.c --> PPC kvm client

To run:

```
gcc kvm-latest.c
```
