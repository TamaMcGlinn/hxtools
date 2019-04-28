# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libHX_CFLAGS}
AM_CFLAGS   = ${regular_CFLAGS}
AM_CXXFLAGS = ${regular_CXXFLAGS}

bin_PROGRAMS = clt2bdf hcdplay

dist_bin_SCRIPTS = aumeta gpsh mkvappend ssa2srt

pkglibexec_PROGRAMS = \
	bsvplay \
	pcmdiff \
	qplay \
	vfontas

dist_pkglibexec_SCRIPTS  = \
	extract_d3pkg \
	extract_dxhog \
	extract_f3pod \
	extract_qupak \
	mod2opus \
	shared.pm \
	stxdb

bsvplay_SOURCES = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD   = ${libHX_LIBS} -lm
clt2bdf_LDADD   = ${libHX_LIBS}
hcdplay_LDADD   = ${libHX_LIBS}
pcmdiff_LDADD   = ${libHX_LIBS}
qplay_SOURCES   = qplay.c pcspkr_pcm.c
qplay_LDADD     = ${libHX_LIBS} -lm
vfontas_SOURCES = vfontas.cpp vfalib.cpp vfalib.hpp
vfontas_LDADD   = ${libHX_LIBS}
if HAVE_XBRZ
vfontas_LDADD  += -lxbrz
endif

EXTRA_DIST = pcspkr.h
