#include <stddef.h>
#include <stdint.h>

static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	const char *p;

	// for (p = "Nishanth\n"; *p; ++p)
	// 	outb(0xE9, *p);
	
	int x = 10;
	*(long *) 0x400 = 42;
	
	// int y = 75641;
	int flag = 1;
	// for(int i = 2;i<y/2;i++){
	// 	if(y%i == 0){
	// 		flag = 0;
	// 		break;
	// 	}
	// }

	if(flag){
		for (p = "75641 is prime\n"; *p; ++p)
		outb(0xE9, *p);
	} else {
		for (p = "75641 is not prime\n"; *p; ++p)
		outb(0xE9, *p);
	}

	outb(0xE9, x);
	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
}
