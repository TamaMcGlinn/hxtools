/*
 *	declone.c - unlink but preserve a file
 *	written by Jan Engelhardt, 2004-2007
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the WTF Public License version 2 or
 *	(at your option) any later version.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BLOCKSIZE 4096

static void dofile(const char *file)
{
	struct stat sb;
	ssize_t ret;
	char buffer[BLOCKSIZE];
	int in, out;

	if ((in = open(file, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        file, strerror(errno));
		return;
	}
	if (fstat(in, &sb) < 0) {
		fprintf(stderr, "Could not stat %s: %s\n",
		        file, strerror(errno));
		return;
	}
	if (unlink(file) < 0) {
		fprintf(stderr, "Could not unlink %s: %s\n",
		        file, strerror(errno));
		return;
	}
	if ((out = open(file, O_WRONLY | O_TRUNC | O_CREAT,
	    sb.st_mode)) < 0) {
		fprintf(stderr, "Could not recreate/open file %s: %s\n",
		        file, strerror(errno));
		fgets(buffer, 4, stdin);
		return;
	}

	printf("* %s\n", file);
	fchown(out, sb.st_uid, sb.st_gid);
	fchmod(out, sb.st_mode);
	while ((ret = read(in, buffer, BLOCKSIZE)) > 0)
		if (write(out, buffer, ret) < 0) {
			fprintf(stderr, "Error during write to %s: "
			        "%s\n", file, strerror(errno));
			break;
		}

	if (ret < 0)
		fprintf(stderr, "Error during read on %s: %s\n",
		        file, strerror(errno));
	close(in);
	close(out);
}

int main(int argc, const char **argv)
{
	++argv;
	for (; --argc > 0 && *argv != NULL; ++argv)
		dofile(*argv);
	return EXIT_SUCCESS;
}
