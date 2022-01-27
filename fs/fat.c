#include "types.h"
#include "multiboot.h"
#include "printk.h"
#include "string.h"

typedef
struct dbr_t {
	uint8_t		jmp[3];
	uint8_t		oem_name[8];
	uint16_t	bytes_per_sec;
	uint8_t		sec_per_clus;
	uint16_t	resvd_sec_cnt;
	uint8_t		num_fats;
	uint16_t	root_ent_cnt;
	uint16_t	total_sec16;
	uint8_t		media;
	uint16_t	fat_size;
	uint16_t	sec_per_trk;
	uint16_t	num_heads;
	uint32_t	hidd_sec;
	uint32_t	total_sec32;
	uint8_t		drv_num;
	uint8_t		reserved;
	uint8_t		boot_sig;
	uint32_t	vol_id;
	uint8_t		vol_lab[11];
	uint8_t		fs_type[8];
	uint8_t		code[448];
	uint16_t	end_flag;
} __attribute__((packed)) dbr_t;

typedef 
struct fat_t {
	uint8_t		value[3];
} __attribute__((packed)) fat_t;

typedef
struct file_t {
	uint8_t		name[8];
	uint8_t		extend[3];
	uint8_t		attribute;
	uint8_t		reserved[10];
	uint16_t	time;
	uint16_t	date;
	uint16_t	cluster;
	uint32_t	length;
} __attribute__((packed)) file_t;

#define fat_next(c) ({ \
	uint16_t _v = *(uint16_t *)(fat_tab[(c)>>1].value+((c)&1)); \
	_v = ((c)&1) ? (_v>>4) : (_v&0x0FFF); \
	_v; \
})

#define first_byte(clus) ((char *)(data_area + ((clus)-3) * dbr->bytes_per_sec))

static dbr_t * dbr;
static fat_t * fat_tab;
static file_t * root_dir;
static char * data_area;

static uint32_t bytes_per_clus;

extern multiboot_t * glb_mboot_ptr;

void fat_read(file_t * file, char * buf, int count, int offset)
{
	uint16_t clus = file->cluster;
	while (offset > bytes_per_clus && clus < 0xFF8) {
		clus = fat_next(clus);
		offset -= bytes_per_clus;
	}
	char * src = first_byte(clus);
	if (offset > 0 && clus < 0xFF8) {
		src += offset;
		int len = count > bytes_per_clus ? bytes_per_clus - offset : count;
		memcpy(buf, src, len);
		clus = fat_next(clus);
		src = first_byte(clus);
		buf += len;
		count -= len;
	}
	while (count >= bytes_per_clus && clus < 0xFF8) {
		memcpy(buf, src, bytes_per_clus);
		clus = fat_next(clus);
		src = first_byte(clus);
		buf += bytes_per_clus;
		count -= bytes_per_clus;
	}
	if (count > 0 && clus < 0xFF8) {
		memcpy(buf, src, count);
		buf += count;
	}
	if (clus >= 0xFF8) {
		printk("Fat Error: Bad cluster in fat.");
	}
	*buf = '\0';
}

void fat_ls(file_t * dir)
{
	char buf[16];
	char * p = NULL;
	char * q = NULL;
	int i;
	for (file_t * file = dir; *file->name != '\0'; file++) {
		if (file->attribute == 0x10 || file->attribute == 0x20) {
			p = buf;
			q = (char *)file->name;
			for (i = 0; i < 8; i++, q++) {
				if (*q != ' ') {
					*p++ = *q;
				}
			}
			if (*q != ' ') {
				*p++ = '.';
			}
			for (; i < 11; i++, q++) {
				if (*q != ' ') {
					*p++ = *q;
				}
			}
			*p = '\0';
			printk("%s\n", buf);
		}
	}
}

void fat_init()
{
	uint32_t initrd_start = *((uint32_t *)glb_mboot_ptr->mods_addr+0xC0000000) + 0xC0000000;
	uint8_t * dbr_base = (uint8_t *)initrd_start;
	dbr = (dbr_t *)dbr_base;
	uint8_t * fat_tab_base = dbr_base + dbr->resvd_sec_cnt * dbr->bytes_per_sec;
	fat_tab = (fat_t *)fat_tab_base;
	uint8_t * root_dir_base = fat_tab_base + dbr->num_fats * dbr->fat_size * dbr->bytes_per_sec;
	root_dir = (file_t *)root_dir_base;	
	uint8_t * data_area_base = root_dir_base + ((dbr->root_ent_cnt * sizeof(file_t)) / dbr->bytes_per_sec + 1) * dbr->bytes_per_sec;
	data_area = (char *)data_area_base;
	bytes_per_clus = dbr->bytes_per_sec * dbr->sec_per_clus;
}
