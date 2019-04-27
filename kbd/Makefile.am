# -*- Makefile -*-

x11fonts     = mux_9x16.pcf.gz mux_17x32.pcf.gz rhx_9x16.pcf.gz \
               rhx_17x32.pcf.gz tosh_9x16.pcf.gz tosh_17x32.pcf.gz
x11miscdir   = ${x11fontdir}/misc
x11misc_DATA = ${x11fonts}
CLEANFILES   = ${x11fonts}

qwertydir         = ${kbddatadir}/keymaps/i386/qwerty
dist_qwerty_DATA  = us_jng.map us_jng_vaiou3.map

qwertzdir         = ${kbddatadir}/keymaps/i386/qwertz
dist_qwertz_DATA  = de_jng.map

vgafontdir        = ${kbddatadir}/consolefonts
dist_vgafont_DATA = A1.fnt B1.fnt ati.fnt gotik-textura.fnt ital.fnt mux.fnt \
               neuropol.fnt nvidia.fnt \
               rhx.fnt tosh.fnt \
               ega437_8x16.fnt ega437_8x14.fnt ega437_8x8.fnt

unimapdir        = ${kbddatadir}/unimaps
dist_unimap_DATA = cp437AB.uni

fontgen1 = ${AM_V_GEN} ../smm/vfontas -loadfnt $< -loadmap ${srcdir}/cp437x.uni -canvas 9 16 -lge -savebdf -
fontgen2 = ${AM_V_GEN} ../smm/vfontas -loadfnt $< -loadmap ${srcdir}/cp437x.uni -upscale 2 2 -canvas 17 32 -lge -savebdf -
fontout  = bdftopcf | gzip -9 >$@

mux_9x16.pcf.gz: mux.fnt
	${fontgen1} mux | ${fontout}
mux_17x32.pcf.gz: mux.fnt
	${fontgen2} mux | ${fontout}
rhx_9x16.pcf.gz: rhx.fnt
	${fontgen1} rhx | ${fontout}
rhx_17x32.pcf.gz: rhx.fnt
	${fontgen2} rhx | ${fontout}
tosh_9x16.pcf.gz: tosh.fnt
	${fontgen1} tosh | ${fontout}
tosh_17x32.pcf.gz: tosh.fnt
	${fontgen2} tosh | ${fontout}
