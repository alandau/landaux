const char string[] = "global string\n";
char in_bss[100];
int global_data=5;

typedef unsigned long u64;

#define CLOBERRED "cx", "r11", "memory"

#define SYSCALL0(rettype, name, nr) \
	rettype name(void) { 	\
		long ret;	\
		asm volatile("syscall" : "=a" (ret) : "0" (nr) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL1(rettype, name, nr, type1) \
	rettype name(type1 arg1) { 	\
		long ret;	\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL2(rettype, name, nr, type1, type2) \
	rettype name(type1 arg1, type2 args) { 	\
		long ret;	\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1), "S"((u64)arg2) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL3(rettype, name, nr, type1, type2, type3) \
	rettype name(type1 arg1, type2 arg2, type3 arg3) { 	\
		long ret;	\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1), "S"((u64)arg2),	\
						      "d"((u64)arg3) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL4(rettype, name, nr, type1, type2, type3, type4) \
	rettype name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) { 	\
		long ret;	\
		register u64 r10 asm ("r10") = (u64)arg4;	\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1), "S"((u64)arg2),	\
				                      "d"((u64)arg3), "r"(r10) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL5(rettype, name, nr, type1, type2, type3, type4, type5) \
	rettype name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) { 	\
		long ret;	\
		register u64 r10 asm ("r10") = (u64)arg4;	\
		register u64 r8 asm ("r8") = (u64)arg5;		\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1), "S"((u64)arg2),	\
				                      "d"((u64)arg3), "r"(r10), "r"(r8) : CLOBERRED);	\
		return (rettype)ret;	\
	}

#define SYSCALL6(rettype, name, nr, type1, type2, type3, type4, type5, type6) \
	rettype name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) { 	\
		long ret;	\
		register u64 r10 asm ("r10") = (u64)arg4;	\
		register u64 r8 asm ("r8") = (u64)arg5;		\
		register u64 r9 asm ("r9") = (u64)arg6;		\
		asm volatile("syscall" : "=a" (ret) : "0" (nr), "D" ((u64)arg1), "S"((u64)arg2),	\
				                      "d"((u64)arg3), "r"(r10), "r"(r8), "r"(r9) : CLOBERRED);	\
		return (rettype)ret;	\
	}

SYSCALL1(long, printk, 0, const char *)
SYSCALL4(long, printk2, 0, const char *, long, long, long)
SYSCALL0(long, fork, 1)
SYSCALL1(long, exec, 2, const char *)
SYSCALL0(long, exit, 3)
SYSCALL0(int, getpid, 4)
SYSCALL0(int, pause, 5)
SYSCALL0(int, yield, 6)
SYSCALL1(int, usleep, 7, long)


int main(void)
{
	printk("start\n");
	usleep(1000000);
	printk("start2\n");
	char s[3];
	s[0] = '1';
	s[1] = '\n';
	int pid = fork();
	if (pid == 0) {
		usleep(500000);
		while (1) {
			printk2("child: pid=%d\n", getpid(), 0, 0);
			usleep(1000000);
		}
		s[0]='2';
		//printk(s);
#if 0
		pid = fork();
		if (pid == 0) {
			printk2("grandchild, pid=%d\n", getpid(), 0, 0);
			exit();
		}
		printk("child after 2nd fork\n");
#endif
		//pause();
		return 0;
		//exit();
	} else {
		while (1) {
			printk2("parent: pid=%d, child=%d\n", getpid(), pid, 0);
			usleep(1000000);
		}
		s[0]='3';
		//printk(s);
		//for (int i=0; i<0xfffffff; i++) asm volatile("");
		//printk(s);
	}
	pause();
	exit();
#if 0
	char string2[] = "local string\n";
	//asm volatile("mov $0x1122334455667788, %rax; mov %rax, %rbx; mov %rax, %rcx");
	printk(string);
	printk(string2);
	exec("/prog");
	while (1);
	exit();
	printk2("0=%d s=[%s], long=%lx\n", 12345678, (u64)string, 0xaabbccdd1122344);
	while (1);
#endif
#if 0
	printk("haha\n");
	printk2("%s\n%d\n%d\n", string, global_data, in_bss[5]);
	printk2("before fork %d\n", getpid(), 0, 0);
	int x = fork();
	if (x == 0) {
		printk2("child, getpid=%d\n", getpid(), 0, 0);
		exit();
	}
	printk2("parent: x=%d, getpid=%d\n", x, getpid(), 0);
	pause();
#endif
	return 0;
}
