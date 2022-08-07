# :ts=8 bk=0
#
# Makefile for Loafit.  Intended for use on a Sun workstation.
# (Probably overkill for this...)
#
# Leo L. Schwab						9305.12
############################################################################
# Copyright (c) 1992 New Technologies Group, Inc.
#
# This document is proprietary and confidential.  Divulge it and DIE.
############################################################################
#
.SUFFIXES:	.c .asm .o

LOAFIT_CO =	loafit.o
LOAFIT_O =	$(LOAFIT_CO)

LOAFANIM_CO =	loafanim.o
LOAFANIM_O =	$(LOAFANIM_CO)


UPDATE =	/other/opera/dragontail6
LIB =		$(UPDATE)/developer/libs
INCLUDES =	-I$(UPDATE)/includes

LIBS =		$(LIB)/Lib3DO.lib $(LIB)/graphics.lib $(LIB)/filesystem.lib \
		$(LIB)/input.lib $(LIB)/audio.lib $(LIB)/music.lib \
		$(LIB)/clib.lib $(LIB)/swi.lib $(LIB)/operamath.lib

BIN =		/usr/local/armbin
STARTUP =	$(LIB)/cstartup.o
COMPILE =	$(BIN)/ncc
ASSEMBLE =	$(BIN)/armasm
LINK =		$(BIN)/armlink
MAKELIB =	$(BIN)/armlib

COPTS = -Wanp -c $(INCLUDES) -bigend -fc -zps0 -za1 -g
SOPTS = -PD '|_LITTLE_END_| SETL {FALSE};' $(INCLUDES) -bigend -Apcs 3/32bit -g
LOPTS = -AIF -B 0x00 -R


############################################################################
# Rules for building files.
#
.c.o:
	$(COMPILE) $(COPTS) -o $@ $*.c

.asm.o:
	$(ASSEMBLE) $(SOPTS) -o $@ $*.asm


############################################################################
# Application builder.
#
all: loafit loafanim
	@echo "Nice software!"

loafit: $(STARTUP) $(LIBS) $(LOAFIT_O)
	$(LINK) $(LOPTS) -S $@.sym -o $@ $(STARTUP) $(LOAFIT_O) $(LIBS)
	$(LINK) $(LOPTS) -Debug -S $@.sym -o $@.dbg $(STARTUP) $(LOAFIT_O) $(LIBS)

loafanim: $(STARTUP) $(LIBS) $(LOAFANIM_O)
	$(LINK) $(LOPTS) -S $@.sym -o $@ $(STARTUP) $(LOAFANIM_O) $(LIBS)
	$(LINK) $(LOPTS) -Debug -S $@.catsym -o $@.dbg $(STARTUP) $(LOAFANIM_O) $(LIBS)
	stripaif $@.dbg -o /dev/null -s $@.sym
	modbin > /dev/null $@ -stack 4096
	modbin > /dev/null $@.dbg -stack 4096


############################################################################
# Automagic prototype generator.
#
proto:
	cproto $(INCLUDES) $(LOAFIT_CO:.o=.c) | sed -e "s/register //g" > app_proto.h


############################################################################
# Include dependencies.
#
loafit.o:	loafit.h
loafanim.o:	loafit.h
