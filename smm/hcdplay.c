/*
 *	hcdplay - Command-line interface to autonomous background CD playback
 *	written by Jan Engelhardt, 2013
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the WTF Public License version 2 or
 *	(at your option) any later version.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libHX/option.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#define DEFAULT_ROM "/dev/sr0"

static char *cdev;
static unsigned int dump_toc, do_start, do_pause, do_resume;
static unsigned int do_play, do_stop, do_eject;

static bool cdp_get_options(int *argc, const char ***argv)
{
	static const struct HXoption option_table[] = {
		{.sh = 'D', .ln = "device", .type = HXTYPE_STRING, .ptr = &cdev,
		 .help = "CD-ROM device path", .htyp = "FILE"},
		{.sh = 'E', .ln = "eject",  .type = HXTYPE_NONE, .ptr = &do_eject,
		 .help = "Eject the CD-ROM"},
		{.sh = 'P', .ln = "pause",  .type = HXTYPE_NONE, .ptr = &do_pause,
		 .help = "Pause playback"},
		{.sh = 'R', .ln = "resume", .type = HXTYPE_NONE, .ptr = &do_resume,
		 .help = "Resume playback"},
		{.sh = 'S', .ln = "stop",   .type = HXTYPE_NONE, .ptr = &do_stop,
		 .help = "Stop playback"},
		{.sh = 'T', .ln = "toc",    .type = HXTYPE_NONE, .ptr = &dump_toc,
		 .help = "Show TOC information"},
		{.sh = 'p', .ln = "play",   .type = HXTYPE_NONE, .ptr = &do_play,
		 .help = "Playback track # or tracks #-#"},
		{.sh = 's', .ln = "start",  .type = HXTYPE_NONE, .ptr = &do_start,
		 .help = "Start playback (?)"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	int ret;

	ret = HX_getopt(option_table, argc, argv, HXOPT_USAGEONERR);
	if (ret != HXOPT_ERR_SUCCESS)
		return false;
	if (cdev == NULL)
		cdev = DEFAULT_ROM;
	return true;
}

int main(int argc, const char **argv)
{
	struct cdrom_tochdr toc;
	int fd;

	if (!cdp_get_options(&argc, &argv))
		return EXIT_FAILURE;

	fd = open(cdev, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
		        cdev, strerror(errno));
		return EXIT_FAILURE;
	}

	if (ioctl(fd, CDROMREADTOCHDR, &toc) < 0) {
		perror("ioctl CDROMREADTOCHDR");
		return EXIT_FAILURE;
	}
	if (dump_toc)
		printf("Tracks: %u-%u\n", toc.cdth_trk0, toc.cdth_trk1);
	if (do_start)
		if (ioctl(fd, CDROMSTART) < 0)
			perror("ioctl CDROMSTART");
	if (do_pause)
		if (ioctl(fd, CDROMPAUSE) < 0)
			perror("ioctl CDROMPAUSE");
	if (do_resume)
		if (ioctl(fd, CDROMRESUME) < 0)
			perror("ioctl CDROMRESUME");
	if (do_play) {
		struct cdrom_ti t;
		t.cdti_trk0 = (argc >= 2) ? strtoul(argv[1], NULL, 0) :
		              toc.cdth_trk0;
		t.cdti_trk1 = (argc >= 3) ? strtoul(argv[2], NULL, 0) :
		              toc.cdth_trk1;
		t.cdti_ind0 = 0;
		t.cdti_ind1 = 0;
		if (ioctl(fd, CDROMPLAYTRKIND, &t) < 0)
			perror("ioctl CDROMPLAYTRKIND");
	}
	if (do_stop)
		if (ioctl(fd, CDROMSTOP) < 0)
			perror("ioctl CDROMSTOP");
	if (do_eject)
		if (ioctl(fd, CDROMEJECT) < 0)
			perror("ioctl CDROMEJECT");
	close(fd);
	return EXIT_SUCCESS;
}
