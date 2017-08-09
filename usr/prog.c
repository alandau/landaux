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

int main(void)
{
	printk("haha\n");
	printk2("%s\n%d\n%d\n", string, global_data, in_bss[5]);
	int x = fork();
	if (x == 0) {
		printk("child\n");
		exit();
	}
	printk2("parent: x=%d\n", x, 0, 0);
	while (1);
	return 0;
}
