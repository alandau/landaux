const char string[] = "global string";
char in_bss[100];
int global_data=5;

int printk(const char *s)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (0), "d"(s));
	return ret;
}

int printk2(const char *s, long a, long b, long c)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (0), "d"(s), "c"(a), "b"(b), "S"(c));
	return ret;
}

int fork(void)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (1));
	return ret;
}

int exit(void)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (3));
	return ret;
}

int getpid(void)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (4));
	return ret;
}

int pause(void)
{
	int ret;
	asm volatile ("int $0x30" : "=a"(ret) : "0" (5));
	return ret;
}

int main(void)
{
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
	return 0;
}
