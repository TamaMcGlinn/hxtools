# -*- Makefile -*-

qwertydir         = ${kbddatadir}/keymaps/i386/qwerty
dist_qwerty_DATA  = us_jng.map us_jng_vaiou3.map

qwertzdir         = ${kbddatadir}/keymaps/i386/qwertz
dist_qwertz_DATA  = de_jng.map

vgafontdir        = ${kbddatadir}/consolefonts
dist_vgafont_DATA = A1.fnt B1.fnt E1.fnt neuropol.fnt

unimapdir        = ${kbddatadir}/unimaps
dist_unimap_DATA = cp437AB.uni

fontgen1 = ${AM_V_GEN} ../smm/vfontas -loadfnt $< -loadmap ${srcdir}/cp437x.uni -canvas 9 16 -lge -savebdf -
fontgen2 = ${AM_V_GEN} ../smm/vfontas -loadfnt $< -loadmap ${srcdir}/cp437x.uni -upscale 2 2 -canvas 17 32 -lge -savebdf -
fontout  = bdftopcf | gzip -9 >$@
