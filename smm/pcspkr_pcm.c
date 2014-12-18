/*
 *	pcspkr_pcm.c - output as PCM
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2007
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "pcspkr.h"

/**
 * pcspkr_output - produce waves
 * @state:	pcspkr state
 * @frequency:	tone frequency
 * @duration:	duration, in samples
 *		(number of samples in a second given by @state->sampling_rate)
 * @af_pause:	after-pause, in samples
 *
 * Outputs 16-bit PCM samples of a @freq Hz tone for @duration to @sp->fp,
 * with an optional pause.
 */
void pcspkr_output(const struct pcspkr *pcsp, long frequency,
    long duration, long af_pause)
{
	int16_t value;
	long sample;

	for (sample = 0; sample < duration; ++sample) {
		double q = sin(2.0 * M_PI * frequency * sample / pcsp->sample_rate);
		value = 32767 * pcsp->volume * (q > 0 ? 1 : -1);
		fwrite(&value, sizeof(value), 1, pcsp->file_ptr);
	}
	value = 0;
	for (sample = 0; sample < af_pause; ++sample)
		fwrite(&value, sizeof(value), 1, pcsp->file_ptr);
}

void pcspkr_silence(const struct pcspkr *pcsp, long duration)
{
	static const uint16_t value_16 = 0;
	long sample;

	for (sample = 0; sample < duration; ++sample)
		fwrite(&value_16, sizeof(value_16), 1, pcsp->file_ptr);
}
