#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)


struct vm {
	int sys_fd;
	int fd;
	char *mem;
};

void vm_init(struct vm *vm, size_t mem_size)
{
	struct kvm_userspace_memory_region memreg;

	vm->sys_fd = open("/dev/kvm", O_RDWR);

	ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
	
	vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);

    ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000);

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = 0;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
	ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg);
}

struct vcpu {
	int fd;
	struct kvm_run *kvm_run;
};

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	int vcpu_mmap_size;

	vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);

	vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    
	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED, vcpu->fd, 0);

}

int run_vm(struct vm *vm, struct vcpu *vcpu, size_t sz)
{
	struct kvm_regs regs;
	uint64_t memval = 0;
	int i = 0;
	for (;;) {
		ioctl(vcpu->fd, KVM_RUN, 0);
		printf("\nkvm run called for %d time\n",++i);

		switch (vcpu->kvm_run->exit_reason) {
		case KVM_EXIT_HLT:
			goto check;

		case KVM_EXIT_IO:
			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT
			    && vcpu->kvm_run->io.port == 0xE9) {
				char *p = (char *)vcpu->kvm_run;
				fwrite(p + vcpu->kvm_run->io.data_offset,
				       vcpu->kvm_run->io.size, 1, stdout);
				fflush(stdout);
				continue;
			}
			break;
		
		default:
			exit(1);
		}
	}

 check:
	ioctl(vcpu->fd, KVM_GET_REGS, &regs);
	memcpy(&memval, &vm->mem[0x400], sz);
	return 1;
}


static void setup_protected_mode(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1, /* Code/data */
		.l = 0,
		.g = 1, /* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE; /* enter protected mode */

	sregs->cs = seg;
	sregs->pvr = 0;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

 extern const unsigned char guest32[], guest32_end[];

int run_protected_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing.....\n");

	ioctl(vcpu->fd, KVM_GET_SREGS, &sregs);
	setup_protected_mode(&sregs);

	ioctl(vcpu->fd, KVM_SET_SREGS, &sregs);
	
	memset(&regs, 0, sizeof(regs));

	regs.rflags = 2;
	regs.rip = 0;

	ioctl(vcpu->fd, KVM_SET_REGS, &regs);

	memcpy(vm->mem, guest32, guest32_end-guest32);
	return run_vm(vm, vcpu, 4);
}

int main()
{
	struct vm vm;
	struct vcpu vcpu;
	vm_init(&vm, 0x200000);
	vcpu_init(&vm, &vcpu);
	return !run_protected_mode(&vm, &vcpu);
	
	return 1;
}
