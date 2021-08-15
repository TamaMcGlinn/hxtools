/*
 *	declone.c - break hardlink off a file
 *	written by Jan Engelhardt, 2021
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the WTF Public License version 2 or
 *	(at your option) any later version.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/defs.h>
#include <libHX/string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

static void dofile(const char *file)
{
	struct stat sb;
	int infd = open(file, O_RDONLY);
	if (infd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        file, strerror(errno));
		return;
	}
	if (fstat(infd, &sb) < 0) {
		fprintf(stderr, "Could not stat %s: %s\n",
		        file, strerror(errno));
		goto out;
	}
	void *inmap = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, infd, 0);
	if (inmap == MAP_FAILED) {
		fprintf(stderr, "mmap %s: %s\n", file, strerror(errno));
		goto out;
	}

	char *cont_dir = HX_dirname(file);
	if (cont_dir == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto out2;
	}
	hxmc_t *outname = HXmc_strinit(cont_dir);
	if (outname == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto out3;
	}
	if (HXmc_strcat(&outname, "/decloneXXXXXX") == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto out4;
	}
	if (mkstemp(outname) < 0) {
		fprintf(stderr, "mkstemp: %s\n", strerror(errno));
		goto out4;
	}
	int outfd = open(outname, O_RDWR, S_IRUGO | S_IWUGO);
	if (outfd < 0) {
		fprintf(stderr, "open %s: %s\n", outname, strerror(errno));
		goto out4;
	}
	if (ftruncate(outfd, sb.st_size) < 0) {
		fprintf(stderr, "ftruncate %s: %s\n", outname, strerror(errno));
		goto out5;
	}
	if (fchown(outfd, sb.st_uid, sb.st_gid) < 0 ||
	    fchmod(outfd, sb.st_mode) < 0) {
		fprintf(stderr, "fchown/fchmod %s: %s\n", outname, strerror(errno));
		goto out5;
	}
	void *outmap = mmap(NULL, sb.st_size, PROT_WRITE, MAP_SHARED, outfd, 0);
	if (outmap == MAP_FAILED) {
		fprintf(stderr, "mmap %s: %s\n", outname, strerror(errno));
		goto out5;
	}
	memcpy(outmap, inmap, sb.st_size);
	if (msync(outmap, sb.st_size, MS_ASYNC) < 0) {
		fprintf(stderr, "msync: %s\n", strerror(errno));
		goto out6;
	}
	if (rename(outname, file) < 0) {
		fprintf(stderr, "Could not replace %s: %s\n",
		        file, strerror(errno));
		goto out6;
	}
	printf("* %s\n", file);
 out6:
	munmap(outmap, sb.st_size);
 out5:
	close(outfd);
	unlink(outname);
 out4:
	HXmc_free(outname);
 out3:
	free(cont_dir);
 out2:
	munmap(inmap, sb.st_size);
 out:
	close(infd);
}

int main(int argc, const char **argv)
{
	++argv;
	for (; --argc > 0 && *argv != NULL; ++argv)
		dofile(*argv);
	return EXIT_SUCCESS;
}
