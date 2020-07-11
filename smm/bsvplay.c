/*
 *	bsvplay.c - BASICA binary music format interpreter
 *	Copyright © Jan Engelhardt, 2002-2010,2020
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/init.h>
#include <libHX/option.h>
#include "pcspkr.h"
#define TICKSPERSEC 1086

/*
 * based on findings from pvt collection
 * entertan.exe 1989 (size:29527 md5:13caa8100e15ad65d900f6a12b9ddf24)
 * ihold.exe    1989 (size:21317 md5:5cf7aef915539c60a9a98c96a3a8aefb)
 * maplleaf.exe 1989 (size:27865 md5:f234797cbd1dba47d90da8a94dcb088f)
 * mnty.exe     1989 (size:19441 md5:507997fa70b89f9a9ed5430e833cb6c0)
 * willtell.exe 1989 (size:33799 md5:5d5405a5ff063476deec618dcd3d990d)
 */
struct bsv_insn {
	uint16_t divisor, duration, af_pause;
};

/*
 * based on findings in FOGDOS.118, FOGDOS.119, FOGDOS.120:
 * pianoman.com 1986-05-06 01:36:02 (md5:cba939868e35b2742984246aa9317984) (Neil J. Rubenking)
 * entertan.mus 1984-12-06 03:51:24 (size:24280 md5:994a5130d91b5b1395e4f9653f06c2e9)
 * (Scott Joplin - The Entertainer)
 * ihold.mus    1986-05-03 18:10:36 (size:17440 md5:d1ed36909bd4db1e4903181e57486c19)
 * (Tom Lehrer - I hold your hand in mine)
 * maplleaf.mus 1984-12-07 03:56:00 (size:23080 md5:f070f2b417065c322434ea2af613d464)
 * (Scott Joplin - The maple leaf rag)
 * mntpythn.mus 1984-12-29 20:19:40 (size:18700 md5:e10ba993a9d3179af0b890f453ddc93f)
 * (John Philip Sousa - The liberty bell)
 * willtell.mus 1985-01-10 15:23:26 (size:40105 md5:bd0e216ce89080a1e633465f57516b6a)
 * (Gioachino Rossini - William Tell)
 */
struct pianoman_insn {
	/* 0..255 */
	uint8_t octave;
	/* c(1), c#(2), d(3), d#(4), e(5), f(6), f#(7), g(8), g#(9),
	 * a(10), a#(11), b(12), rest(13) */
	uint8_t note;
	/* 0..9 */
	uint8_t staccato;
	/* 1280=whole note, 640=half, etc. */
	uint16_t len;
} __attribute__((packed));

static struct pcspkr pcsp = {
	.sample_rate = 48000,
	.prop_square = 1,
	.prop_sine   = 1,
	.volume      = 0.1,
};

static unsigned int filter_lo = 0, filter_hi = ~0U;
static unsigned int tick_groupsize, tick_filter;
static unsigned int no_zero_ticks, flg_pianoman;

static void parse_basica(int fd)
{
	unsigned int count = 0, ticks = 0;
	struct bsv_insn tone;

	while (read(fd, &tone, sizeof(struct bsv_insn)) ==
	    sizeof(struct bsv_insn))
	{
		long frequency = 0x1234DD / tone.divisor;
		bool silenced;

		++count;

		silenced = frequency < filter_lo || frequency > filter_hi;
		if (tick_groupsize != 0)
			silenced |= (count % tick_groupsize) != tick_filter;

		fprintf(stderr, "(%5u) %5hu %5ldHz%c %5hu %5hu\n",
			count, tone.divisor, frequency,
			silenced ? '*' : ' ', tone.duration,
		        tone.af_pause);
		/*
		 * It seems that in the sample BSV executables from 1989
		 * calculate the cpu speed and then do around 1086 ticks/sec.
		 * entertan.exe: 199335 / 183s = 1089
		 * ihold.exe:     73248 /  68s = 1077
		 * maplleaf.exe: 170568 / 157s = 1086
		 * mnty.exe:     119680 / 110s = 1088
		 * willtell.exe: 225350 / 206s = 1093
		 */
		ticks += tone.duration + tone.af_pause;
		if (silenced && no_zero_ticks)
			;
		else if (silenced)
			pcspkr_silence(&pcsp, (tone.duration + tone.af_pause) *
				pcsp.sample_rate / TICKSPERSEC);
		else
			pcspkr_output(&pcsp, frequency,
			              tone.duration * pcsp.sample_rate / TICKSPERSEC,
			              tone.af_pause * pcsp.sample_rate / TICKSPERSEC);
	}

	fprintf(stderr, "Total ticks: %u\n", ticks);
}

static void parse_pianoman(int fd)
{
	unsigned int count = 0, ticks = 0;
	struct pianoman_insn tone;
	double notemap[255*12];

	for (int n = 0; n < sizeof(notemap); ++n)
		notemap[n] = 440.0 * pow(2, (n - 45) / 12.0);

	while (read(fd, &tone, sizeof(tone)) == sizeof(tone)) {
		int frequency = notemap[tone.octave*12+tone.note];
		unsigned int af_pause = tone.len * tone.staccato / 10;
		unsigned int duration = tone.len - af_pause;
		bool silenced;

		++count;
		silenced = tone.note == 13;
		if (tick_groupsize != 0)
			silenced |= (count % tick_groupsize) != tick_filter;

		fprintf(stderr, "(%5u) O%uN%02u %5dHz%c %5u %5u\n",
			count, tone.octave, tone.note, frequency,
			silenced ? '*' : ' ', duration,
		        af_pause);
		/*
		 * Pianoman runs through
		 * entertan.mus: 194240/182s=1067
		 * ihold.mus:     69760/ 66s=1056
		 * maplleaf.mus: 138480/130s=1065
		 * mntpythn.mus: 112200/105s=1068
		 * willtell.mus: 169407/159s=1065
		 */
		ticks += tone.len;
		if (silenced && no_zero_ticks)
			;
		else if (silenced)
			pcspkr_silence(&pcsp, (duration + af_pause) *
				pcsp.sample_rate / TICKSPERSEC);
		else
			pcspkr_output(&pcsp, frequency,
			              duration * pcsp.sample_rate / TICKSPERSEC,
			              af_pause * pcsp.sample_rate / TICKSPERSEC);
	}
	fprintf(stderr, "Total ticks: %u\n", ticks);
}

static void parse_file(const char *file)
{
	int fd;
	if (strcmp(file, "-") == 0)
		fd = STDIN_FILENO;
	else
		fd = open(file, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n", file, strerror(errno));
		return;
	}
	if (flg_pianoman)
		parse_pianoman(fd);
	else
		parse_basica(fd);
	close(fd);
}

int main(int argc, const char **argv)
{
	static const struct HXoption options_table[] = {
		{.sh = 'H', .type = HXTYPE_UINT, .ptr = &filter_hi,
		 .help = "High frequency cutoff (low-pass filter)"},
		{.sh = 'L', .type = HXTYPE_UINT, .ptr = &filter_lo,
		 .help = "Low frequency cutoff (high-pass filter)"},
		{.sh = 'M', .type = HXTYPE_UINT, .ptr = &tick_groupsize,
		 .help = "Size of a tick block"},
		{.sh = 'T', .type = HXTYPE_UINT, .ptr = &tick_filter,
		 .help = "Play only this tick in a tick block"},
		{.sh = 'Z', .type = HXTYPE_NONE, .ptr = &no_zero_ticks,
		 .help = "Skip over silenced ticks"},
		{.sh = 'i', .type = HXTYPE_DOUBLE, .ptr = &pcsp.prop_sine,
		 .help = "Proportion of sine-wave calculation mixed in"},
		{.sh = 'q', .type = HXTYPE_DOUBLE, .ptr = &pcsp.prop_square,
		 .help = "Proportion of square-wave calculation mixed in"},
		{.sh = 'r', .type = HXTYPE_UINT, .ptr = &pcsp.sample_rate,
		 .help = "Sample rate (default: 48000)"},
		{.ln = "pianoman", .type = HXTYPE_NONE, .ptr = &flg_pianoman,
		 .help = "Assume input is in Pianoman .MUS file"},
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

	pcsp.file_ptr = fdopen(STDOUT_FILENO, "wb");
	if (argc == 1)
		parse_file("-");
	else
		while (--argc > 0)
			parse_file(*++argv);

	HX_exit();
	return EXIT_SUCCESS;
}
