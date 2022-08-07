# :ts=8 bk=0
#
# Makefile for CastleHopper.  Intended for use on a Sun workstation.
#
# Leo L. Schwab						9305.12
############################################################################
# Copyright (c) 1993 New Technologies Group, Inc.
#
# This document is proprietary and confidential.  Divulge it and DIE.
############################################################################
#
.SUFFIXES:	.c .asm .o

CASTLE_CO =	ctst.o rend.o shoot.o objects.o ob_zombie.o ob_george.o \
		ob_head.o ob_spider.o ob_powerup.o ob_exit.o ob_trigger.o \
		clip.o levelfile.o leveldef.o genmessage.o statscreen.o \
		thread.o titleseq.o imgfile.o loadloaf.o file.o timing.o \
		sound.o soundinterface.o cinepak.o map.o option.o \
		elkabong.o
FONT_O =	font.co font.so
CASTLE_AO =	misc.o project.o
CASTLE_O =	$(CASTLE_CO) $(CASTLE_AO)


# UPDATE =	/other/opera/dragontail6
# LIB =		$(UPDATE)/developer/libs
# INCLUDES =	-I$(UPDATE)/includes -I$(DSSUPPORT)
# DSSUPPORT =	dssupport
UPDATE =	$(HOME)/hackery/3do-devkit
LIB =		$(UPDATE)/lib/3do
CINCLUDES =	-J$(UPDATE)/include/3dosdk/1p3 -I$(DSSUPPORT)
ASMINCLUDES =	-I$(UPDATE)/include/3dosdk/1p3
DSSUPPORT =	dssupport

# Improved music.lib fetched locally for 9311.11 build.
LIBS =		$(DSSUPPORT)/Subscriber.lib \
		$(LIB)/lib3do.lib $(LIB)/graphics.lib $(LIB)/filesystem.lib \
		$(LIB)/input.lib $(LIB)/audio.lib music.lib \
		$(LIB)/clib.lib $(LIB)/swi.lib $(LIB)/operamath.lib \
		$(DSSUPPORT)/DataAcq.lib $(DSSUPPORT)/DS.lib \
		$(DSSUPPORT)/codec.lib

BIN =		$(UPDATE)/bin/compiler/linux
STARTUP =	$(LIB)/cstartup.o
COMPILE =	$(BIN)/armcc
ASSEMBLE =	$(BIN)/armasm
LINK =		$(BIN)/armlink
MAKELIB =	$(BIN)/armlib

COPTS = -Wanp -c $(CINCLUDES) -bigend -fc -zps0 -za1
SOPTS = -PD '|_LITTLE_END_| SETL {FALSE};' $(ASMINCLUDES) -bigend -Apcs 3/32bit
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
all: ctst
	@echo "Nice software!"

##ctst: $(STARTUP) $(LIBS) $(CASTLE_O) $(FONT_O)
##	$(LINK) $(LOPTS) -Debug -S $@.catsym -o $@.dbg $(STARTUP) $(CASTLE_O) $(FONT_O) $(LIBS)
##	stripaif $@.dbg -o $@ -s $@.sym
##	modbin > /dev/null $@ -stack 4096 -debug
##	modbin > /dev/null $@.dbg -stack 4096 -debug


ctst: $(LIBS) $(CASTLE_O) $(FONT_O)
	$(LINK) $(LOPTS) -S $@.catsym -o $@ -libpath $(LIB) -noscanlib -verbose $(STARTUP) $(CASTLE_O) $(FONT_O) $(LIBS)
	modbin > /dev/null $@ --stack 10240


############################################################################
# Automagic prototype generator.
#
proto:
	cproto $(INCLUDES) $(CASTLE_CO:.o=.c) | sed -e "s/register //g" > app_proto.h


############################################################################
# Include dependencies.
#
ctst.o:			castle.h objects.h imgfile.h loaf.h sound.h flow.h \
			app_proto.h
levelfile.o:		castle.h objects.h app_proto.h
timing.o:		castle.h app_proto.h
imgfile.o:		castle.h imgfile.h app_proto.h
rend.o:			castle.h imgfile.h loaf.h anim.h app_proto.h
textstuff.o:		castle.h objects.h imgfile.h loaf.h textstuff.h \
			app_proto.h
clip.o:			castle.h app_proto.h
shoot.o:		castle.h objects.h imgfile.h sound.h app_proto.h
objects.o:		castle.h objects.h anim.h imgfile.h sound.h \
			app_proto.h
loadloaf.o:		castle.h loaf.h app_proto.h
ob_zombie.o:		castle.h objects.h anim.h imgfile.h sound.h \
			app_proto.h
leveldef.o:		castle.h objects.h
zombietitle.o:		castle.h app_proto.h
sound.o:		castle.h sound.h soundinterface.h app_proto.h
thread.o:		castle.h app_proto.h
genmessage.o:		castle.h objects.h imgfile.h app_proto.h font.h
ob_exit.o:		castle.h objects.h app_proto.h
ob_powerup.o:		castle.h objects.h loaf.h anim.h sound.h \
			ob_powerup.h app_proto.h
soundinterface.o:	soundinterface.h
map.o:			castle.h objects.h imgfile.h ob_powerup.h \
			app_proto.h
titleseq.o:		castle.h imgfile.h flow.h app_proto.h
ob_trigger.o:		castle.h objects.h sound.h app_proto.h
ob_head.o:		castle.h objects.h anim.h imgfile.h app_proto.h
ob_george.o:		castle.h objects.h anim.h imgfile.h app_proto.h
ob_spider.o:		castle.h objects.h anim.h imgfile.h app_proto.h
statscreen.o:		castle.h objects.h imgfile.h font.h sound.h flow.h \
			option.h app_proto.h
cinepak.o:		castle.h app_proto.h
option.o:		castle.h objects.h imgfile.h font.h option.h \
			app_proto.h
