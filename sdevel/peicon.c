/*
 *	peicon - extract icons from Portable Executable (PE) files
 *	Copyright by Jan Engelhardt, 2013
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	For details, see the file named "LICENSE.GPL2".
 *
 *	See http://msdn.microsoft.com/en-us/library/ms997538.aspx for a
 *	description of the ICO and PE formats.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <libHX/defs.h>
#include <libHX/map.h>
#include <libHX/option.h>

struct mzhdr {
	uint8_t magic[2];
	uint16_t unused[13];
	uint8_t reserved[32];
	uint32_t pe_offset;
};

struct pecoffhdr {
	uint8_t magic[4];
	uint16_t machine;
	uint16_t num_sections;
	uint32_t xtime;
	uint32_t symtab_ptr;
	uint32_t num_symbols;
	uint16_t opthdr_size;
	uint16_t flags;
};

struct pesecthdr {
	char name[8];
	uint32_t virt_size, virt_addr;
	uint32_t size, offset;
	uint32_t reloctbl_ofs, lntbl_ofs;
	uint16_t reloc_count, ln_count;
	uint32_t flags;
};

struct resdir {
	uint32_t unused[2];
	uint16_t unused2[2], name_count, id_count;
};

struct resdirent {
	uint32_t name, offset;
};

struct resdataent {
	uint32_t offset, size, codepage, reserved;
};

struct icondir {
	uint16_t reserved, type, count;
} __attribute__((packed));
#define grpicondir icondir

struct icondirentry {
	uint8_t width, height, colors, reserved;
	uint16_t planes, bpp;
	uint32_t bytes, image_offset;
} __attribute__((packed));

struct grpicondirentry {
	uint8_t width, height, colors, reserved;
	uint16_t planes, bpp;
	uint32_t bytes;
	uint16_t id;
} __attribute__((packed));

/**
 * @re_inum:	offset of this RE from the file
 * @data_inum:	offset of the data from the file
 * @fs_name:	display name
 */
struct Z_resdirent {
	const char *base;
	unsigned int re_from_base, re_from_section;
	unsigned int data_from_base, data_from_section;
	unsigned int re_name;
	char fs_name[64], fs_type;
};

struct Z_resdir {
	const char *base;
	unsigned int section_from_base, rd_from_section, re_from_section;
	unsigned int end_from_section;
	struct Z_resdirent dentry;
};

struct mblock {
	const char *data;
	unsigned int size;
};

enum {
	RT_ICON = 0x03,
	RT_GROUP_ICON = 0x0e,
};

static int global_shift;
static unsigned int verbose, verbose_dirlist, verbose_filelist;
static struct HXmap *icon_list;

static const struct HXoption options_table[] = {
	{.sh = 'L', .type = HXTYPE_NONE, .ptr = &verbose_dirlist},
	{.sh = 'l', .type = HXTYPE_NONE, .ptr = &verbose_filelist},
	{.sh = 'o', .type = HXTYPE_INT, .ptr = &global_shift},
	{.sh = 'v', .type = HXTYPE_NONE | HXOPT_INC, .ptr = &verbose},
	HXOPT_AUTOHELP,
	HXOPT_TABLEEND,
};

static bool verb_show(const struct Z_resdirent *rde)
{
	if (rde->fs_type == 'd' && verbose_dirlist)
		return true;
	if (rde->fs_type == 'f' && verbose_filelist)
		return true;
	return false;
}

static bool re_isdir(const struct resdirent *re)
{
	return re->offset & (1 << 31);
}

static unsigned int re_offset(const struct resdirent *re)
{
	return re->offset & ~(1 << 31);
}

static struct Z_resdir *
resdir_open(const char *base, unsigned int sfb, unsigned int dfs)
{
	const struct resdir *rd;
	struct Z_resdir *dh;

	dh = malloc(sizeof(*dh));
	if (dh == NULL)
		return NULL;

	rd = reinterpret_cast(const struct resdir *, base + sfb + dfs);
	dh->base = base;
	dh->section_from_base = sfb;
	dh->rd_from_section   = dfs;
	dh->re_from_section   = 0;
	dh->end_from_section  = dfs + sizeof(*rd) + sizeof(struct resdirent) *
	                        (rd->name_count + rd->id_count);
	return dh;
}

static const struct Z_resdirent *resdir_walk(struct Z_resdir *dh)
{
	const struct resdirent *re;
	struct Z_resdirent *de = &dh->dentry;

	memset(de, 0, sizeof(*de));
	de->base = dh->base;
	if (dh->re_from_section == 0) {
		/* dir itself */
		de->fs_type = 'd';
		de->re_from_section   = dh->rd_from_section;
		de->re_from_base      = dh->section_from_base + de->re_from_section;
		de->data_from_section = de->re_from_section + sizeof(struct resdir);
		de->data_from_base    = dh->section_from_base + de->data_from_section;
		snprintf(de->fs_name, sizeof(de->fs_name), ".");

		dh->re_from_section = dh->rd_from_section + sizeof(struct resdir);
		return de;
	} else if (dh->re_from_section >= dh->end_from_section) {
		return NULL;
	}

	re = reinterpret_cast(const struct resdirent *,
	     dh->base + dh->section_from_base + dh->re_from_section);
	de->fs_type = re_isdir(re) ? 'd' : 'f';
	de->re_name = re->name;
	de->re_from_section   = dh->re_from_section;
	de->re_from_base      = dh->section_from_base + de->re_from_section;
	de->data_from_section = re_offset(re);
	de->data_from_base    = dh->section_from_base + de->data_from_section;
	snprintf(de->fs_name, sizeof(de->fs_name), "%x@%x",
	         re->name, de->data_from_base);

	dh->re_from_section += sizeof(*re);
	return de;
}

static void resdir_close(struct Z_resdir *dh)
{
	free(dh);
}

static int record_icon(uintptr_t icon_id, const struct mblock *mblk)
{
	const void *icon_key = reinterpret_cast(void *, icon_id);
	int ret;

	ret = HXmap_add(icon_list, icon_key, mblk);
	if (ret <= 0) {
		fprintf(stderr, "HXmap_add: %s\n", strerror(-ret));
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static const struct grpicondirentry *
grpicondirentry_next(const struct grpicondirentry *e)
{
	return reinterpret_cast(const struct grpicondirentry *,
	       reinterpret_cast(const char *, e) + sizeof(*e));
}

static void print_iconsubdir_entry(const struct Z_resdirent *icon_de,
    const struct Z_resdirent *de, const struct resdataent *rm)
{
	printf("%x %c", de->re_from_base, de->fs_type);
	if (de->fs_type == 'd')
		printf(" %5u /icons/%s/%s (unhandled)",
		       0, icon_de->fs_name, de->fs_name);
	else
		printf(" %5u /icons/%s/%s:%x",
		       rm->size, icon_de->fs_name, de->fs_name,
		       rm->offset - global_shift);
	printf("\n");
}

static int parse_iconsubdir(const struct Z_resdirent *icon_de,
    const char *base, unsigned int section_from_base)
{
	struct Z_resdir *dh;
	const struct Z_resdirent *de;
	const struct resdataent *rm;
	struct mblock mblk;
	int ret = EXIT_SUCCESS;

	dh = resdir_open(base, section_from_base, icon_de->data_from_section);
	if (dh == NULL) {
		perror("resdir_open");
		return EXIT_FAILURE;
	}

	while ((de = resdir_walk(dh)) != NULL) {
		if (strcmp(de->fs_name, ".") == 0)
			continue;
		rm = reinterpret_cast(const struct resdataent *,
		     de->base + de->data_from_base);
		if (verb_show(de))
			print_iconsubdir_entry(icon_de, de, rm);
		if (de->fs_type == 'd')
			continue;
		mblk.data = de->base + rm->offset - global_shift;
		mblk.size = rm->size;
		ret = record_icon(icon_de->re_name, &mblk);
		if (ret != EXIT_SUCCESS)
			break;
	}

	resdir_close(dh);
	return ret;
}

static void print_icondir_entry(const struct Z_resdirent *de)
{
	printf("%x %c", de->re_from_base, de->fs_type);
	printf(" %5u /icons/%s", 0, de->fs_name);
	if (de->fs_type != 'd')
		printf(" (unhandled)");
	printf("\n");
}

static int parse_icondir(const char *base, unsigned int section_from_base,
    unsigned int rd_from_section)
{
	const struct Z_resdirent *de;
	struct Z_resdir *dh;
	int ret = EXIT_SUCCESS;

	dh = resdir_open(base, section_from_base, rd_from_section);
	if (dh == NULL) {
		perror("resdir_open");
		return EXIT_FAILURE;
	}

	while ((de = resdir_walk(dh)) != NULL) {
		if (strcmp(de->fs_name, ".") == 0)
			continue;
		if (verb_show(de))
			print_icondir_entry(de);
		if (de->fs_type != 'd')
			continue;
		ret = parse_iconsubdir(de, dh->base, dh->section_from_base);
		if (ret != EXIT_SUCCESS)
			break;
	}

	resdir_close(dh);
	return ret;
}

static void print_giconent(const struct Z_resdirent *gicon_de,
    const struct Z_resdirent *grs_de, const struct mblock *mblk,
    unsigned int i, const struct grpicondirentry *ge)
{
	printf("0 l /group_icon/%s/%s:%lx/%x (%ux%ux%u) -> /icons/%x\n",
	       gicon_de->fs_name, grs_de->fs_name,
	       static_cast(long, mblk->data - gicon_de->base), i,
	       ge->width, ge->height, ge->bpp, ge->id);
}

static void gide_to_ide(struct icondirentry *i,
    const struct grpicondirentry *g)
{
	i->width = g->width;
	i->height = g->height;
	i->colors = g->colors;
	i->reserved = g->reserved;
	i->planes = g->planes;
	i->bpp = g->bpp;
	i->bytes = g->bytes;
}

static int parse_giconent(const struct Z_resdirent *gicon_de,
    const struct Z_resdirent *grs_de, const struct mblock *mblk)
{
	const struct mblock *icon_mblk;
	const struct grpicondir *gd;
	const struct grpicondirentry *ge;
	struct icondir id;
	struct icondirentry ie;
	unsigned int i;
	char filename[64];
	int fd;

	gd = reinterpret_cast(const struct icondir *, mblk->data);
	if (gd->type != 1)
		return EXIT_SUCCESS;

	id = *gd;
	id.count = 1;
	ge = reinterpret_cast(const struct grpicondirentry *,
	     mblk->data + sizeof(*gd));
	for (i = 0; i < gd->count; ++i, ge = grpicondirentry_next(ge)) {
		if (verbose_filelist)
			print_giconent(gicon_de, grs_de, mblk, i, ge);
		icon_mblk = HXmap_get(icon_list,
		            reinterpret_cast(const void *,
		            static_cast(uintptr_t, ge->id)));
		if (icon_mblk == NULL) {
			fprintf(stderr, "No such icon %u\n", ge->id);
			return EXIT_FAILURE;
		}

		snprintf(filename, sizeof(filename), "pe-%x:%x.ico",
		         gicon_de->re_name, i);
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		     S_IRUGO | S_IWUGO);
		gide_to_ide(&ie, ge);
		ie.image_offset = sizeof(id) + sizeof(ie);
		printf("x %s (%u bytes, %ux%ux%u)\n", filename,
		       ie.image_offset + icon_mblk->size,
		       ie.width, ie.height, ie.bpp);
		if (fd < 0) {
			fprintf(stderr, "Could not open %s for writing: %s\n",
			        filename, strerror(errno));
			return EXIT_FAILURE;
		}
		write(fd, &id, sizeof(id));
		write(fd, &ie, sizeof(ie));
		write(fd, icon_mblk->data, icon_mblk->size);
		close(fd);
	}
	return EXIT_SUCCESS;
}

static void print_giconsubdir_entry(const struct Z_resdirent *gicon_de,
    const struct Z_resdirent *de, const struct resdataent *rm)
{
	printf("%x %c", de->re_from_base, de->fs_type);
	if (de->fs_type == 'd')
		printf(" %5u /group_icons/%s/%s (unhandled)",
		       0, gicon_de->fs_name, de->fs_name);
	else
		printf(" %5u /group_icons/%s/%s:%x",
		       rm->size, gicon_de->fs_name, de->fs_name,
		       rm->offset - global_shift);
	printf("\n");
}

static int parse_giconsubdir(const struct Z_resdirent *gicon_de,
    const char *base, unsigned int section_from_base)
{
	struct Z_resdir *dh;
	const struct Z_resdirent *de;
	const struct resdataent *rm;
	int ret = EXIT_SUCCESS;
	struct mblock mblk;

	dh = resdir_open(base, section_from_base, gicon_de->data_from_section);
	if (dh == NULL) {
		perror("resdir_open");
		return EXIT_FAILURE;
	}

	while ((de = resdir_walk(dh)) != NULL) {
		if (strcmp(de->fs_name, ".") == 0)
			continue;
		rm = reinterpret_cast(const struct resdataent *,
		     de->base + de->data_from_base);
		if (verb_show(de))
			print_giconsubdir_entry(gicon_de, de, rm);
		if (de->fs_type == 'd')
			continue;
		mblk.data = de->base + rm->offset - global_shift;
		mblk.size = rm->size;
		ret = parse_giconent(gicon_de, de, &mblk);
		if (ret != EXIT_SUCCESS)
			break;
	}

	resdir_close(dh);
	return ret;
}

static int parse_gicondir(const char *base, unsigned int section_from_base,
    unsigned int rd_from_section)
{
	const struct Z_resdirent *de;
	struct Z_resdir *dh;
	int ret = EXIT_SUCCESS;

	dh = resdir_open(base, section_from_base, rd_from_section);
	if (dh == NULL) {
		perror("resdir_open");
		return EXIT_FAILURE;
	}

	while ((de = resdir_walk(dh)) != NULL) {
		if (strcmp(de->fs_name, ".") == 0)
			continue;

		if (verb_show(de)) {
			printf("%x %c", de->re_from_base, de->fs_type);
			printf(" %5u /group_icons/%s", 0, de->fs_name);
			if (de->fs_type != 'd')
				printf(" (unhandled)");
			printf("\n");
		}
		if (de->fs_type != 'd')
			continue;
		ret = parse_giconsubdir(de, dh->base, dh->section_from_base);
		if (ret != EXIT_SUCCESS)
			break;
	}

	resdir_close(dh);
	return ret;
}

static int parse_resdir(const char *base, unsigned int section_from_base,
    unsigned int rd_from_section)
{
	const struct Z_resdirent *de;
	struct Z_resdir *dh;
	int ret = EXIT_SUCCESS;

	dh = resdir_open(base, section_from_base, rd_from_section);
	if (dh == NULL) {
		perror("resdir_open");
		return EXIT_FAILURE;
	}

	while ((de = resdir_walk(dh)) != NULL) {
		if (verb_show(de)) {
			printf("%x %c", de->re_from_base, de->fs_type);
			printf(" %5u /%s", 0, de->fs_name);
			if (de->re_name == RT_ICON)
				printf(" => icons");
			else if (de->re_name == RT_GROUP_ICON)
				printf(" => group_icons");
			printf("\n");
		}

		ret = EXIT_SUCCESS;
		if (de->re_name == RT_ICON)
			ret = parse_icondir(dh->base, dh->section_from_base,
			      de->data_from_section);
		else if (de->re_name == RT_GROUP_ICON)
			ret = parse_gicondir(dh->base, dh->section_from_base,
			      de->data_from_section);
		if (ret != EXIT_SUCCESS)
			break;
	}

	resdir_close(dh);
	return ret;
}

static int parse_pecoffhdr(const char *base, unsigned int pe_offset)
{
	const struct pecoffhdr *pe;
	const struct pesecthdr *sh;
	unsigned int i;
	int ret;
	unsigned int sh_offset;

	pe = reinterpret_cast(const struct pecoffhdr *, base + pe_offset);
	if (pe->magic[0] != 'P' || pe->magic[1] != 'E' ||
	    pe->magic[2] != 0x00 || pe->magic[3] != 0x00) {
		printf("ERROR: Not a PE file\n");
		return EXIT_FAILURE;
	}
	if (verbose >= 2) {
		printf("PE magic checked out.\n");
		printf("PE: Number of sections: %u\n", pe->num_sections);
		printf("PE: Optional header size 0x%x\n", pe->opthdr_size);
	}

	sh_offset = pe_offset + sizeof(*pe) + pe->opthdr_size;

	for (i = 0; i < pe->num_sections; ++i) {
		sh = reinterpret_cast(const struct pesecthdr *, base + sh_offset);

		if (verbose >= 2)
			printf("PE: Section %u (\"%.8s\") at 0x%x\n",
			       i, sh->name, sh_offset);
		if (strncmp(sh->name, ".rsrc", sizeof(sh->name)) == 0) {
			global_shift = (intptr_t)sh->virt_addr - (intptr_t)sh->offset;
			if (verbose >= 2) {
				printf("SH: Resource section header\n");
				printf("SH: Start 0x%x size 0x%x\n", sh->offset, sh->size);
				printf("SH: VA->FA shift = %d\n\n", global_shift);
			}
			ret = parse_resdir(base, sh->offset, 0);
			if (ret != EXIT_SUCCESS)
				return ret;
		}
		sh_offset += sizeof(*sh);
	}
	return EXIT_SUCCESS;
}

static int parse_mzhdr(const void *base)
{
	const struct mzhdr *mz = base;

	if (mz->magic[0] != 'M' || mz->magic[1] != 'Z') {
		printf("ERROR: Not an MZ file\n");
		return EXIT_FAILURE;
	}
	if (verbose >= 2) {
		printf("MZ magic checked out.\n");
		printf("MZ: PE header at 0x%x\n", mz->pe_offset);
	}
	return parse_pecoffhdr(base, mz->pe_offset);
}

int main(int argc, const char **argv)
{
	struct stat sb;
	void *base;
	int fd, ret;

	ret = HX_getopt(options_table, &argc, &argv, HXOPT_USAGEONERR);
	if (ret != HXOPT_ERR_SUCCESS)
		return EXIT_FAILURE;
	icon_list = HXmap_init5(HXMAPT_DEFAULT, HXMAP_CDATA,
		NULL, 0, sizeof(struct mblock));
	if (icon_list == NULL) {
		perror("HXmap_init");
		return EXIT_FAILURE;
	}

	if (argc < 2) {
		fprintf(stderr, "Extract what file?\n");
		return EXIT_FAILURE;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s for reading: %s\n",
		        argv[1], strerror(errno));
		return EXIT_FAILURE;
	}
	if (fstat(fd, &sb) < 0) {
		perror("fstat");
		return EXIT_FAILURE;
	}
	base = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (base == reinterpret_cast(void *, -1UL)) {
		perror("mmap");
		return EXIT_FAILURE;
	}
	close(fd);
	ret = parse_mzhdr(base);
	munmap(base, sb.st_size);
	printf("Found %u icon bitmaps\n", icon_list->items);
	HXmap_free(icon_list);
	return ret;
}
