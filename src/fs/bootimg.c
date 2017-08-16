#include <kernel.h>
#include <fs.h>
#include <string.h>

#define REGTYPE         '0'     /**< Regular file (preferred code). */
#define AREGTYPE        '\0'    /**< Regular file (alternate code). */
#define LNKTYPE         '1'     /**< Hard link. */
#define SYMTYPE         '2'     /**< Symbolic link (hard if not supported). */
#define CHRTYPE         '3'     /**< Character special. */
#define BLKTYPE         '4'     /**< Block special. */
#define DIRTYPE         '5'     /**< Directory.  */
#define FIFOTYPE        '6'     /**< Named pipe.  */
#define CONTTYPE        '7'     /**< Contiguous file. */

typedef struct tar_header {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
} tar_header_t;

/* Borrowed from libarchive 1.2.38 */
static const u32 max_uint64 = ~(u32)0;
static const s32 max_int64 = (s32)((~(u32)0) >> 1);
static const s32 min_int64 = (s32)(~((~(u32)0) >> 1));

static s32 tar_atol8(const char *p, unsigned char_cnt) {
        s32 l, limit, last_digit_limit;
        int digit, sign, base;

        base = 8;
        limit = max_int64 / base;
        last_digit_limit = max_int64 % base;

        while(*p == ' ' || *p == '\t')
                p++;

        if(*p == '-') {
                sign = -1;
                p++;
        } else {
                sign = 1;
        }

        l = 0;
        digit = *p - '0';
        while(digit >= 0 && digit < base && char_cnt-- > 0) {
                if(l > limit || (l == limit && digit > last_digit_limit)) {
                        l = max_uint64; /* Truncate on overflow. */
                        break;
                }

                l = (l * base) + digit;
                digit = *++p - '0';
        }

        return (sign < 0) ? -l : l;
}

static s32 tar_atol256(const char *_p, unsigned char_cnt) {
        s32 l, upper_limit, lower_limit;
        const unsigned char *p = (const unsigned char *)_p;

        upper_limit = max_int64 / 256;
        lower_limit = min_int64 / 256;

        /* Pad with 1 or 0 bits, depending on sign. */
        if((0x40 & *p) == 0x40)
                l = (s32)-1;
        else
                l = 0;

        l = (l << 6) | (0x3f & *p++);
        while(--char_cnt > 0) {
                if(l > upper_limit) {
                        l = max_int64; /* Truncate on overflow */
                        break;
                } else if(l < lower_limit) {
                        l = min_int64;
                        break;
                }
                l = (l << 8) | (0xff & (s32)*p++);
        }

        return l;
}

static s32 tar_atol(const char *p, unsigned char_cnt) {
        if(*p & 0x80)
                return tar_atol256(p, char_cnt);

        return tar_atol8(p, char_cnt);
}

int extract_tar(void *tar_start, u32 tar_size)
{
	if (tar_size < sizeof(tar_header_t))
		return -EINVAL;
	char *p = (char *)tar_start;
	tar_header_t *h = (tar_header_t *)p;
	if (memcmp(h->magic, "ustar", 5) != 0)
		return -EINVAL;
	while ((u64)p < (u64)tar_start + tar_size) {
		int ret;
		u32 size;
		file_t *f;
		char name[101];
		if (h->name[0] == 0 && h->name[1] == 0)
			break;
		name[0] = '/';
		name[1] = '\0';
		strcat(name, h->name);
		size = tar_atol(h->size, sizeof(h->size));
		switch (h->typeflag) {
		case REGTYPE:
		case AREGTYPE:
			f = vfs_open(name, O_WRONLY|O_CREAT|O_TRUNC);
			if (IS_ERR(f)) {
				printk("%s: Can't create file (%d)\n", name, PTR_ERR(f));
				break;
			}
			ret = vfs_write(f, p + 512, size);
			if (ret != size)
				printk("%s: File not fully extracted: %d instead of %d\n", name, ret, size);
			vfs_close(f);
			break;
		case DIRTYPE:
			ret = vfs_mkdir(name);
			if (ret < 0 && ret != -EEXIST)
				printk("%s: Can't create directory (%d)\n", name, ret);
			break;
		default:
			printk("Unknown file type '%c' (0x%x) for file '%s'. Skipping.\n",
				h->typeflag, h->typeflag, h->name);
			break;
		}
		p += 512;	/* skip header */
		if (size)
			p += ROUND_UP(size, 512);
		h = (tar_header_t *)p;
	}
	return 0;
}
