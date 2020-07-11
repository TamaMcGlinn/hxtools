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

struct bsv_insn {
	uint16_t divisor, duration, af_pause;
};

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
		 * entertan.exe: 199335 / 183 = 1089
		 * ihold.exe:     73248 /  68 = 1077
		 * maplleaf.exe: 170568 / 157 = 1086
		 * mnty.exe:     119680 / 110 = 1088
		 * willtell.exe: 225350 / 206 = 1093
		 */
		ticks += tone.duration + tone.af_pause;
		if (silenced && no_zero_ticks)
			;
		else if (silenced)
			pcspkr_silence(&pcsp, (tone.duration + tone.af_pause) *
				pcsp.sample_rate / 1086);
		else
			pcspkr_output(&pcsp, frequency,
			              tone.duration * pcsp.sample_rate / 1086,
			              tone.af_pause * pcsp.sample_rate / 1086);
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
		ticks += duration + af_pause;
		if (silenced && no_zero_ticks)
			;
		else if (silenced)
			pcspkr_silence(&pcsp, (duration + af_pause) *
				pcsp.sample_rate / 1086);
		else
			pcspkr_output(&pcsp, frequency,
			              duration * pcsp.sample_rate / 1086,
			              af_pause * pcsp.sample_rate / 1086);
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
