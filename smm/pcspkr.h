#ifndef _PCSPKR_H
#define _PCSPKR_H 1

#ifndef __cplusplus
#	include <stdio.h>
#else
#	include <cstdio>
extern "C" {
#endif

struct pcspkr {
	double prop_square, prop_sine, volume;
	FILE *file_ptr;
	unsigned int sample_rate;
};

extern void pcspkr_output(const struct pcspkr *, long, long, long);
extern void pcspkr_silence(const struct pcspkr *, long);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PCSPKR_H */
