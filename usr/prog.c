const char string[] = "global string\n";
char in_bss[100];
int global_data=5;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
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
	rettype name(type1 arg1, type2 arg2) { 	\
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

SYSCALL4(int, getdents, 8, const char *, void *, u32, int)
SYSCALL2(int, open, 9, const char *, int)
SYSCALL1(int, close, 10, int)
SYSCALL3(int, read, 11, int, void *, u32)
SYSCALL3(int, write, 12, int, const void *, u32)
SYSCALL3(int, lseek, 13, int, u32, int)

SYSCALL1(int, mkdir, 14, const char *)
SYSCALL1(int, rmdir, 15, const char *)
SYSCALL1(int, unlink, 16, const char *)
SYSCALL2(int, mount, 17, const char *, const char *)


void *strcpy(char *dest, const char *src)
{
	u64 tmp1, tmp2, tmp3;
	__asm__ __volatile__ (
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al, %%al\n\t"
		"jnz 1b"
		: "=&S" (tmp1), "=&D" (tmp2), "=&a" (tmp3)
		: "0" (src), "1" (dest)
		: "memory");
	return dest;
}

u32 strlen(const char *s)
{
	int tmp;
	register unsigned long res;
	__asm__ __volatile__ (
		"repnz\n\t"
		"scasb\n\t"
		"not %0\n\t"
		"dec %0\n\t"
		"movl %%ecx, %%ecx"
		: "=c" (res), "=&D" (tmp)
		: "0" (0xFFFFFFFF), "1" (s), "a" (0));
	return res;
}

char *strcat(char *dest, const char *src)
{
	int len = strlen(dest);
	strcpy(dest + len, src);
	return dest;
}

static u8 malloc_buf[1<<20];
static int malloc_ptr;
void *malloc(int size) {
	malloc_ptr = (malloc_ptr + 7) / 8 * 8;
	if (malloc_ptr + size > 1<<20) {
		printk2("OUT OF MEMORY %d\n", size, 0, 0);
		pause();
	}
	void *p = &malloc_buf[malloc_ptr];
	malloc_ptr += size;
	return p;
}

void free(void *p) {
}

#define O_RDONLY	1
#define O_WRONLY	2
#define O_RDWR		3

#define O_APPEND	4
#define O_CREAT		8
#define O_EXCL		16
#define O_TRUNC		32

#define PATH_MAX 255

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define DIR_FILE	1
#define DIR_DIR		2
//#define DIR_LINK	3
#define DIR_TYPE(x)	((x) & 7)

typedef struct {
	u32 reclen;
	u32 mode;
	u32 size;
	char name[0];
} dentry_t;

void tree(const char *path)
{
	char buf[100];
	int i;
	dentry_t *dp;
	printk2("D %s\n", (u64)path, 0, 0);
	int count, start = 0;
	while (1) {
		count = getdents(path, buf, 100, start);
		if (count < 0) {
			printk2("count=%d\n", count, 0, 0);
			break;
		}
		if (count == 0)
			break;
		dp = (dentry_t *)buf;
		for (i = 0; i < count; i++) {
			if (DIR_TYPE(dp->mode) == DIR_DIR) {
				char *s = malloc(strlen(path) + 1 + strlen(dp->name) + 1);
				s[0] = '\0';
				strcpy(s, path);
				if (path[strlen(path)-1] != '/')
					strcat(s, "/");
				strcat(s, dp->name);
				tree(s);
				free(s);
			} else {
				printk2("F %s%s%s\n", (u64)path, (u64)(path[strlen(path)-1]=='/'?"":"/"), (u64)dp->name);
			}
			dp = (dentry_t *)(buf + dp->reclen);
		}
		start += count;
	}
}

int main(void)
{
	printk("start\n");
	tree("/");
	int fd = open("/dev/platform/console/dev", O_RDWR);
	if (fd < 0) {
		printk2("fd=%d\n", fd, 0, 0);
		pause();
	}
	const char *str = "i'm writing!!\n";
	int err = write(fd, str, strlen(str));
	if (err < 0) {
		printk2("err=%d\n", err, 0, 0);
		pause();
	}
	char k[10];
	do {
		err = read(fd, k, 10);
	//printk2("read=%d\n", err, 0, 0);
		k[err] = 0;
		write(fd, k, err);
	} while (err > 0 && k[err - 1] != '\n');
	err = close(fd);
	if (err < 0) {
		printk2("close=%d\n", err, 0, 0);
		pause();
	}
	pause();
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
