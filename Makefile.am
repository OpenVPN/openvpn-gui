#  OpenVPN-GUI -- A Windows GUI for OpenVPN.
#
#  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
#                2010 Heiko Hund <heikoh@users.sf.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program (see the file COPYING included with this
#  distribution); if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

RCCOMPILE = $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
    $(AM_CPPFLAGS) $(CPPFLAGS)

AUTOMAKE_OPTIONS = foreign 1.9

MAINTAINERCLEANFILES = \
	config.log config.status \
	$(srcdir)/Makefile.in  $(srcdir)/configure \
	$(srcdir)/config.h.in $(srcdir)/config.h.in~\
	$(srcdir)/install-sh $(srcdir)/missing \
	$(srcdir)/depcomp $(srcdir)/aclocal.m4 \
	$(srcdir)/config.guess $(srcdir)/config.sub \
	$(srcdir)/compile

bin_PROGRAMS = openvpn-gui

dist_doc_DATA = \
	COPYRIGHT.GPL \
	COPYING

AM_CPPFLAGS = $(OPENSSL_CRYPTO_CFLAGS) -D_UNICODE
AM_CFLAGS = -municode

openvpn_gui_RESOURCES = \
	res/openvpn-gui-res.rc \
	res/openvpn-gui-res-cs.rc \
	res/openvpn-gui-res-de.rc \
	res/openvpn-gui-res-dk.rc \
	res/openvpn-gui-res-en.rc \
	res/openvpn-gui-res-es.rc \
	res/openvpn-gui-res-fi.rc \
	res/openvpn-gui-res-fr.rc \
	res/openvpn-gui-res-it.rc \
	res/openvpn-gui-res-jp.rc \
	res/openvpn-gui-res-kr.rc \
	res/openvpn-gui-res-nl.rc \
	res/openvpn-gui-res-no.rc \
	res/openvpn-gui-res-pl.rc \
	res/openvpn-gui-res-pt.rc \
	res/openvpn-gui-res-ru.rc \
	res/openvpn-gui-res-se.rc \
	res/openvpn-gui-res-tr.rc \
	res/openvpn-gui-res-ua.rc \
	res/openvpn-gui-res-zh-hans.rc \
	res/openvpn-gui-res-zh-hant.rc \
	res/connected.ico \
	res/connecting.ico \
	res/disconnected.ico \
	res/openvpn-gui.ico \
	res/reconnecting.ico \
	res/openvpn-gui.manifest

EXTRA_DIST = $(openvpn_gui_RESOURCES)

openvpn_gui_SOURCES = \
	main.c main.h \
	openvpn.c openvpn.h \
	localization.c localization.h \
	tray.c tray.h \
	viewlog.c viewlog.h \
	service.c service.h \
	options.c options.h \
	passphrase.c passphrase.h \
	proxy.c proxy.h \
	registry.c registry.h \
	scripts.c scripts.h \
	manage.c manage.h \
	misc.c misc.h \
	openvpn_config.c \
	openvpn_config.h \
	access.c access.h \
	chartable.h \
	save_pass.c save_pass.h \
	env_set.c env_set.h \
	openvpn-gui-res.h

openvpn_gui_LDFLAGS = -mwindows
openvpn_gui_LDADD = \
	openvpn-gui-res.o \
	$(OPENSSL_CRYPTO_LIBS) \
	-lws2_32 \
	-lcomctl32 \
	-lwinhttp \
	-lwtsapi32 \
	-lcrypt32 \
	-lnetapi32 \
	-lole32 \
	-lshlwapi \
	-lsecur32

openvpn-gui-res.o: $(openvpn_gui_RESOURCES) $(srcdir)/openvpn-gui-res.h
	$(RCCOMPILE) -i $< -o $@
