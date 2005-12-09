#include <stdio.h>
#include <string.h>

#define MAX_KERNEL_SECS 64

unsigned char num_fats/*16*/, sec_per_clust/*13*/;
unsigned int root_ent_count/*17*/, fat_size16/*22*/, reserved_sec_count/*14*/;
unsigned root_dir_sectors, first_data_sec, first_root_dir_sec;

unsigned short clust_to_sec(unsigned short clust)
{
        return (clust - 2) * sec_per_clust + first_data_sec;
}

typedef struct direntry
{
        unsigned char name[11];
        char attr;
        char Reserved;
        char time_tenth;
        unsigned short time;
        unsigned short date;
        unsigned short access_date;
        unsigned short ClusNumHI;
        unsigned short write_time;
        unsigned short write_date;
        unsigned short cluster;
        unsigned long size;
} direntry_t;

int main(int argc, char *argv[])
{
	char name[11], *ptr=name, *ptr2=argv[1];
	int res;
	unsigned char buf[512];
	unsigned clust=0;
	FILE *out;
	direntry_t *d;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <kernel_name> <output_image>\n", argv[0]);
		return 1;
	}
	for (; *ptr2; ptr++, ptr2++)
	{
		if (*ptr2 != '.')
		{
			*ptr = *ptr2;
			if (*ptr>='a' && *ptr<='z') *ptr -= 'a'-'A';
		}
		else
		{
			for (; ptr<name+8; ptr++) *ptr=' ';
			ptr--;
		}
	}
	for (; ptr<name+11; ptr++) *ptr=' ';

	if ((out = fopen(argv[2], "r+")) == NULL)
	{
		perror("open output");
		return 1;
	}
	if (fread(buf, 512, 1, out) != 1)
	{
		perror("read output 1");
		return 1;
	}

	num_fats = buf[16];
	sec_per_clust = buf[13];
	root_ent_count = buf[17] | buf[18]<<8;
	fat_size16 = buf[22] | buf[23]<<8;
	reserved_sec_count = buf[14] | buf[15]<<8;
	root_dir_sectors = (root_ent_count*32 + 512 - 1)/512;
	first_data_sec = reserved_sec_count + num_fats*fat_size16 + root_dir_sectors;
        first_root_dir_sec = reserved_sec_count + num_fats*fat_size16;

	fseek(out, first_root_dir_sec * 512, SEEK_SET);
	if (fread(buf, 512, 1, out) != 1)
	{
		perror("root dir read output");
		return 1;
	}
	ptr=buf;
	while ((unsigned char *)ptr < buf+512)
	{
		d=(direntry_t *)ptr;
		ptr += 32;
		if (d->name[0] == 0) break;
		if (d->name[0] == 0xE5) continue;
		if (memcmp(d->name, name, 11) == 0) clust=d->cluster;
	}
	if (clust == 0) {fprintf(stderr, "not found\n"); return 1;}
	rewind(out);
	if (fread(buf, 512, 1, out) != 1)
	{
		perror("read output 2");
		return 1;
	}
//	unsigned short *sector_ptr=(unsigned short *)(buf + 510 - MAX_KERNEL_SECS*2);
	unsigned short sec_array[1024];
	unsigned short *sector_ptr = sec_array;
	unsigned char ker_size=0;
	unsigned short cursec=0;
	unsigned char sec2[512*2];
	while (clust < 0x0FF8)
	{
		int i;
		unsigned short offset, secnum, temp;
		unsigned short sec = clust_to_sec(clust);
		for (i=0; i<sec_per_clust; i++)
		{
			*(sector_ptr++) = sec+i;
			ker_size++;
		}
		offset = clust + clust/2;
		secnum = reserved_sec_count + offset/512;
		offset %= 512;
		if (cursec != secnum)
		{
			cursec=secnum;
			fseek(out, cursec * 512, SEEK_SET);
			if (fread(sec2, 512, 2, out) != 2)
			{
				perror("read output 3");
				return 1;
			}
		}
		temp = sec2[offset] | sec2[offset+1]<<8;
		if (clust & 1) temp >>= 4;		// odd
		else temp &= 0x0FFF;			// even
		clust = temp;
	}
	*sector_ptr = 0;
	unsigned short *p = (unsigned short *)(buf + 510 - MAX_KERNEL_SECS*2);
	sector_ptr = sec_array;
	while (*sector_ptr)
	{
		*(p++) = *(sector_ptr++);
		unsigned short count = 1;
		while (*sector_ptr && *sector_ptr == *(sector_ptr-1)+1) {count++; sector_ptr++;}
		*(p++) = count;
	}
	*p = 0;
	for (p = (unsigned short *)(buf + 510 - MAX_KERNEL_SECS*2); *p; p+=2)
		printf("%d,%d  ", *p, *(p+1));
	fseek(out, 0, SEEK_SET);
	if (fwrite(buf, 512, 1, out) != 1)
	{
		perror("write output");
		return 1;
	}
	fclose(out);
	printf("\n");
	return 0;
}
