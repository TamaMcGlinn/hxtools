# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libHX_CFLAGS}
AM_CFLAGS   = ${regular_CFLAGS}
AM_CXXFLAGS = ${regular_CXXFLAGS}

bin_PROGRAMS = bsvplay hcdplay pcmdiff pcmmix qplay vfontas
dist_bin_SCRIPTS = aumeta extract_d3pkg extract_dxhog extract_f3pod \
	extract_qupak extract_dfqshared.pm gpsh mod2opus mkvappend ssa2srt

bsvplay_SOURCES = bsvplay.c pcspkr_pcm.c
bsvplay_LDADD   = ${libHX_LIBS} -lm
hcdplay_LDADD   = ${libHX_LIBS}
pcmdiff_LDADD   = ${libHX_LIBS}
pcmmix_LDADD    = ${libHX_LIBS}
qplay_SOURCES   = qplay.c pcspkr_pcm.c
qplay_LDADD     = ${libHX_LIBS} -lm
vfontas_SOURCES = vfontas.cpp vfalib.cpp vfalib.hpp
vfontas_LDADD   = ${libHX_LIBS}

EXTRA_DIST = pcspkr.h
