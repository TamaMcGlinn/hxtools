# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libHX_CFLAGS}
AM_CFLAGS   = ${regular_CFLAGS}
AM_CXXFLAGS = ${regular_CXXFLAGS}

bin_PROGRAMS = clt2bdf hcdplay
if HAVE_XBRZ
bin_PROGRAMS += cltxscale
endif

dist_bin_SCRIPTS = aumeta gpsh mkvappend ssa2srt

pkglibexec_PROGRAMS = \
	bsvplay \
	clt2pbm \
	pcmdiff \
	qplay \
	vfontas

dist_pkglibexec_SCRIPTS  = \
	extract_d3pkg \
	extract_dxhog \
	extract_f3pod \
	extract_qupak \
	fnt2bdf \
	mod2opus \
	shared.pm \
	stxdb

bsvplay_SOURCES = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD   = ${libHX_LIBS} -lm
clt2bdf_LDADD   = ${libHX_LIBS}
clt2pbm_LDADD   = ${libHX_LIBS}
cltxscale_SOURCES = cltxscale.c xbrz_call.cpp
cltxscale_LDADD   = -lxbrz ${libHX_LIBS}
hcdplay_LDADD   = ${libHX_LIBS}
pcmdiff_LDADD   = ${libHX_LIBS}
qplay_SOURCES   = qplay.c pcspkr_pcm.c
qplay_LDADD     = ${libHX_LIBS} -lm
vfontas_SOURCES = vfontas.cpp vfalib.cpp vfalib.hpp
vfontas_LDADD   = ${libHX_LIBS}

EXTRA_DIST = pcspkr.h
