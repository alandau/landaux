#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *in, *out;
	char buf[512]={0};
	int count, retval=0;
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <boot_sector> <output_image>\n", argv[0]);
		return 1;
	}
	if ((in = fopen(argv[1], "r")) == NULL)
	{
		perror("input file");
		return 1;
	}
	if ((out = fopen(argv[2], "r+")) == NULL)
	{
		fclose(in);
		perror("output file");
		return 1;
	}
	if ((count = fread(buf, 1, 510, in)) == -1)
	{
		perror("read error");
		retval = 1;
		goto out_close;
	}
	buf[510] = 0x55;
	buf[511] = 0xAA;
	if ((count = fwrite(buf, 512, 1, out)) != 1)
	{
		perror("write error");
		retval = 1;
		goto out_close;
	}
out_close:
	fclose(in);
	fclose(out);
	return retval;
}
