# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libHX_CFLAGS}
AM_CFLAGS   = ${regular_CFLAGS}

bin_PROGRAMS = \
	bin2c

dist_bin_SCRIPTS = \
	checkbrack \
	cwdiff \
	git-author-stat \
	git-export-patch \
	git-forest \
	git-track \
	man2html \
	spec-beautifier

pkglibexec_PROGRAMS = \
	cctypeinfo

dist_pkglibexec_SCRIPTS = \
	diff2php \
	doxygen-kerneldoc-filter \
	git-blame-stats \
	git-new-root \
	git-revert-stats \
	paddrspacesize \
	proc_stat_signal_decode \
	sourcefuncsize

bin2c_LDADD = ${libHX_LIBS}
