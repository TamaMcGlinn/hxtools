/*
 *	pcmmix - trivial wave mixer
 *	Copyright Jan Engelhardt, 2020
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/init.h>
#include <libHX/option.h>

static char *blocklen_str;
static unsigned int mixing_mode, sample_rate = 48000;

enum {
	MIX_ARPEGGIO = 0,
	MIX_POLYPHONY,
};

static ssize_t fullread(int fd, void *buf, size_t rem)
{
	size_t pos = 0;
	while (rem > 0) {
		ssize_t ret = read(fd, (char *)buf + pos, rem);
		if (ret == 0)
			return pos;
		if (ret < 0)
			return ret;
		rem -= ret;
		pos += ret;
	}
	return pos;
}

int main(int argc, const char **argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'a', .type = HXTYPE_NONE | HXTYPE_VAL, .ptr = &mixing_mode,
		 .val = MIX_ARPEGGIO, .help = "Arpeggio mix (see -t too)"},
		{.sh = 'p', .type = HXTYPE_NONE | HXTYPE_VAL, .ptr = &mixing_mode,
		 .val = MIX_POLYPHONY, .help = "Polyphonic mix"},
		{.sh = 'r', .type = HXTYPE_UINT, .ptr = &sample_rate,
		 .help = "Input sample rate (default: 48000)", .htyp = "RATE"},
		{.sh = 't', .type = HXTYPE_STRING, .ptr = &blocklen_str,
		 .help = "Block length (default: 40/1086)", .htyp = "p/q"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	int ret;
	if ((ret = HX_init()) <= 0) {
		fprintf(stderr, "HX_init: %s\n", strerror(-ret));
		abort();
	}
	if (HX_getopt(options_table, &argc, &argv, HXOPT_USAGEONERR) !=
	    HXOPT_ERR_SUCCESS) {
		HX_exit();
		return EXIT_FAILURE;
	}
	if (blocklen_str == NULL)
		blocklen_str = "120/1086";

	char *e = NULL;
	unsigned int blocklen_p = strtoul(blocklen_str, &e, 0);
	unsigned int blocklen_q = 1;
	if (e != NULL && *e == '/')
		blocklen_q = strtoul(e + 1, NULL, 0);
	fprintf(stderr, "Block length: %u/%us", blocklen_p, blocklen_q);
	unsigned int blocksamples = sample_rate * blocklen_p / blocklen_q;
	unsigned int blockbytes = sizeof(int16_t) * blocksamples;
	fprintf(stderr, ", %u samples, %u bytes\n", blocksamples, blockbytes);

	--argc;
	++argv;

	int *inputfd = malloc(sizeof(*inputfd) * argc);
	int inputcount = 0, eofs = 0;

	for (int i = 0; i < argc; ++i) {
		inputfd[i] = open(argv[i], O_RDONLY);
		if (inputfd[i] < 0) {
			fprintf(stderr, "open %s: %s\n", argv[i], strerror(errno));
			continue;
		}
		++inputcount;
	}

	if (inputfd == NULL) {
		perror("malloc");
		HX_exit();
		return EXIT_FAILURE;
	}

	if (mixing_mode == MIX_ARPEGGIO) {
		void *inputbuf = malloc(blockbytes);
		void *dumbbuf = malloc(blockbytes);
		if (inputbuf == NULL || dumbbuf == NULL) {
			perror("malloc");
			HX_exit();
			return EXIT_FAILURE;
		}

		unsigned int fragsamples = blocksamples / inputcount;
		unsigned int fragbytes = sizeof(int16_t) * fragsamples;
		fprintf(stderr, "Slot sample count: %u\n", fragsamples);

		while (eofs < inputcount) {
			memset(inputbuf, 0, sizeof(int16_t) * blocksamples);
			for (int i = 0; i < inputcount; ++i) {
				for (int j = 0; j < inputcount; ++j) {
					void *atp = i != j ? dumbbuf : inputbuf + i * fragbytes;
					ssize_t rdret = fullread(inputfd[i], atp, fragbytes);
					if (i == j && rdret != fragbytes)
						++eofs;
				}
			}
			write(STDOUT_FILENO, inputbuf, inputcount * fragbytes);
		}

		free(dumbbuf);
		free(inputbuf);
	} else if (mixing_mode == MIX_POLYPHONY) {
		int16_t **inputbuf = malloc(sizeof(int16_t *) * inputcount);
		for (int i = 0; i < inputcount; ++i)
			inputbuf[i] = malloc(blockbytes);
		int16_t *outputbuf = malloc(blockbytes);

		while (eofs < inputcount) {
			for (int i = 0; i < inputcount; ++i) {
				memset(inputbuf[i], 0, blockbytes);
				ssize_t rdret = fullread(inputfd[i], inputbuf[i], blockbytes);
				if (rdret != blockbytes)
					++eofs;
			}
			for (int s = 0; s < blocksamples; ++s) {
				int32_t v = 0;
				for (int i = 0; i < inputcount; ++i)
					v += inputbuf[i][s];
				outputbuf[s] = v / inputcount;
			}
			write(STDOUT_FILENO, outputbuf, blockbytes);
		}
		free(outputbuf);
		for (int i = 0; i < inputcount; ++i)
			free(inputbuf[i]);
		free(inputbuf);
	}

	free(inputfd);
	return EXIT_SUCCESS;
}
