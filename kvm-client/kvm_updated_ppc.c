/* Sample code for /dev/kvm API
 *
 * Copyright (c) 2015 Intel Corporation
 * Author: Josh Triplett <josh@joshtriplett.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

// #include <fcntl.h>
// #include <linux/kvm.h>

// #include <stdio.h>
// #include <stdlib.h>

// #include <sys/ioctl.h>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <sys/types.h>

#ifndef __powerpc64__
#error "unsupported architecture"
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/kvm.h>

#define EPAPR_MAGIC (0x45504150)
int main(void)
{
    int kvm, vmfd, vcpufd, ret, i;
    // unsigned long number_of_instructions = 10;
    // const uint8_t code[] = {
    //     0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
    //     0x00, 0xd8,       /* add %bl, %al */
    //     0x04, '0',        /* add $'0', %al */
    //     0xee,             /* out %al, (%dx) */
    //     0xb0, '\n',       /* mov $'\n', %al */
    //     0xee,             /* out %al, (%dx) */
    //     0xf4,             /* hlt */
    // };
    //     uint8_t *mem;
    void *vmmem;
    struct kvm_sregs sregs;
    size_t mmap_size;
    struct kvm_run *run;
    unsigned long pgsize;
    struct kvm_userspace_memory_region vmmreg;

    kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm == -1)
        err(1, "/dev/kvm");

    /* Make sure we have the stable version of the API */
    ret = ioctl(kvm, KVM_GET_API_VERSION, NULL);
    if (ret == -1)
        err(1, "KVM_GET_API_VERSION");
    if (ret != 12)
        errx(1, "KVM_GET_API_VERSION %d, expected 12", ret);

    /*
e.g, to configure a guest to use 48bit physical address size :

    vm_fd = ioctl(dev_fd, KVM_CREATE_VM, KVM_VM_TYPE_ARM_IPA_SIZE(48));

The requested size (IPA_Bits) must be :
  0 - Implies default size, 40bits (for backward compatibility)

  or

  N - Implies N bits, where N is a positive integer such that,
      32 <= N <= Host_IPA_Limit

Host_IPA_Limit is the maximum possible value for IPA_Bits on the host and
is dependent on the CPU capability and the kernel configuration. The limit can
be retrieved using KVM_CAP_ARM_VM_IPA_SIZE of the KVM_CHECK_EXTENSION
ioctl() at run-time.
*/
    vmfd = ioctl(kvm, KVM_CREATE_VM, KVM_VM_PPC_HV);
    if (vmfd == -1)
        err(1, "KVM_CREATE_VM");

    // Run time page size
    pgsize = sysconf(_SC_PAGESIZE);

    /* Allocate one aligned page of guest memory to hold the code. */
    //     mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    vmmem = mmap(NULL, pgsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!vmmem)
        err(1, "allocating guest memory");
    //     memcpy(mem, code, sizeof(code));

    // Why is this being done?
    // Why these particular values?
    printf("page size id %ld\n", pgsize);
    for (i = 0; i < pgsize / sizeof(unsigned int); i++)
        ((unsigned int *)vmmem)[i] = 0x00000006;

    // ((unsigned int *)vmmem)[1] = 0x39200002;
    // ((unsigned int *)vmmem)[2] = 0x913f0020;
    // ((unsigned int *)vmmem)[3] = 0x3900000a;
    // ((unsigned int *)vmmem)[4] = 0x913f0024;
    // ((unsigned int *)vmmem)[5] = 0x815f0020;
    // ((unsigned int *)vmmem)[6] = 0x813f0024;
    // ((unsigned int *)vmmem)[7] = 0x7d2a4a14;
    // ((unsigned int *)vmmem)[8] = 0x913f0028;
    // ((unsigned int *)vmmem)[9] = 0x60000000;


    ((unsigned int *)vmmem)[0] = 0x48000000; /* b	0x0 */

    memset(&vmmreg, 0, sizeof(struct kvm_userspace_memory_region));
    vmmreg.slot = 0;
    vmmreg.guest_phys_addr = 0x0;
    vmmreg.memory_size = pgsize;
    vmmreg.userspace_addr = (unsigned long)vmmem;
    vmmreg.flags = 0;

    /* Map it to the second page frame (to avoid the real-mode IDT at 0). */
    //     struct kvm_userspace_memory_region region = {
    //         .slot = 0,
    //         .guest_phys_addr = 0x1000,
    //         .memory_size = 0x1000,
    //         .userspace_addr = (uint64_t)mem,
    //     };
    //     ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);

    ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &vmmreg);
    if (ret == -1)
        err(1, "KVM_SET_USER_MEMORY_REGION");

    //     vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, (unsigned long)0);

    //Does 0 signify the vcpu id?
    vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, 0UL);
    if (vcpufd == -1)
        err(1, "KVM_CREATE_VCPU");

    /* Map the shared kvm_run structure and following data. */
    ret = ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (ret == -1)
        err(1, "KVM_GET_VCPU_MMAP_SIZE");
    mmap_size = ret;
    if (mmap_size < sizeof(*run))
        errx(1, "KVM_GET_VCPU_MMAP_SIZE unexpectedly small");

    run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpufd, 0);
    if (!run)
        err(1, "mmap vcpu");

    /* Initialize CS to point at 0, via a read-modify-write of sregs. */
    ret = ioctl(vcpufd, KVM_GET_SREGS, &sregs);
    if (ret == -1)
        err(1, "KVM_GET_SREGS");
    //sregs.cs.base = 0;
    //sregs.cs.selector = 0;
    //Determines version of the processor (32 bit)
    //     sregs.pvr = 0;

    //     ret = ioctl(vcpufd, KVM_SET_SREGS, &sregs);
    //     if (ret == -1)
    //         err(1, "KVM_SET_SREGS");
    /* Initialize registers: instruction pointer for our code, addends, and
     * initial flags required by ppc architecture. */
    struct kvm_regs regs = {
        //Count register
        .ctr = 0,
        //Link Register
        .lr = 0,
        //Indicates overflow and carry condition
        .xer = 0,
        //Machine state register -- specifies running in 64 bit mode
        .msr = 0x10000000,
        .pc = 0,
        //Save and restore register
        .srr0 = 0,
        .srr1 = 0,
        //Used by OS
        .sprg0 = 0,
        .sprg1 = 0,
        .sprg2 = 0,
        .sprg3 = 0,
        .sprg4 = 0,
        .sprg5 = 0,
        .sprg6 = 0,
        .sprg7 = 0,
        //Process ID
        .pid = 0x0,

        // qemu sets these this way (e500.c:ppce500_cpu_reset)
        .gpr[1] = (16 << 20) - 8,
        .gpr[3] = 0xdead,
        .gpr[4] = 0xbeef,
        .gpr[5] = 0,
        .gpr[6] = EPAPR_MAGIC,
        .gpr[7] = 0x1000,
        .gpr[8] = 0,
        .gpr[9] = 0,
        //Condition Register
        .cr = 0x0,
    };
    ret = ioctl(vcpufd, KVM_SET_REGS, &regs);
    if (ret == -1)
        err(1, "KVM_SET_REGS");
    //     printf("kvm pvr = 0x%08x\n", regs.pvr);
    printf("kvm msr = 0x%016lx\n", regs.msr);
    printf("kvm regs gpr[1] = 0x%016lx\n", regs.gpr[1]);
    printf("kvm regs gpr[3] = 0x%016lx\n", regs.gpr[3]);
    printf("kvm regs pc = 0x%016lx\n", regs.pc);
    string error_string = "kvm error has occurred, and exit is cause " ;
    /* Repeatedly run code and handle VM exits. */
    while (1)
    {
        ret = ioctl(vcpufd, KVM_RUN, NULL);
        if (ret == -1){
            error_string = error_string +  run->exit_reason;
            puts(error_string);
            err(1, "KVM_RUN");
        }
        switch (run->exit_reason)
        {
        case KVM_EXIT_HLT:
            puts("KVM_EXIT_HLT");
            return 0;
        case KVM_EXIT_IO:
            if (run->io.direction == KVM_EXIT_IO_OUT && run->io.size == 1 && run->io.port == 0x3f8 && run->io.count == 1)
                putchar(*(((char *)run) + run->io.data_offset));
            else
                errx(1, "unhandled KVM_EXIT_IO");
            break;
        case KVM_EXIT_FAIL_ENTRY:
            errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                 (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
        case KVM_EXIT_INTERNAL_ERROR:
            errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", run->internal.suberror);
        default:
            errx(1, "exit_reason = 0x%x", run->exit_reason);
        }
    }
    printf("aftere execution kvm regs gpr[9] = 0x%016lx\n", regs.gpr[9]);
}
