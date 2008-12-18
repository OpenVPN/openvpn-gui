# This makefile builds OpenVPN-GUI using the mingw environment.

OPENSSL = /c/OpenSSL

RES_LANG = en
GUI_VERSION = 1.0.3

EXE = openvpn-gui-$(GUI_VERSION)-$(RES_LANG).exe

HEADERS = main.h openvpn.h openvpn_monitor_process.h tray.h viewlog.h \
          service.h options.h passphrase.h openvpn-gui-res.h proxy.h \
          ieproxy.h registry.h openvpn_config.h chartable.h scripts.h

OBJS =	main.o tray.o openvpn.o openvpn_monitor_process.o viewlog.o \
        service.o options.o passphrase.o proxy.o ieproxy.o registry.o \
        openvpn_config.o scripts.o openvpn-gui-$(RES_LANG).res

INCLUDE_DIRS = -I. -I${OPENSSL}/include
LIB_DIRS = -L${OPENSSL}/lib/MinGW

WARNS = -W -Wall -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast \
	-Wcast-align -Wwrite-strings -Wconversion -Wsign-compare \
	-Waggregate-return -Wmissing-noreturn -Wmissing-format-attribute \
	-Wredundant-decls -Winline -Wdisabled-optimization \
        -Wno-unused-function -Wno-unused-variable 

CC = gcc
CFLAGS = -g -O2 ${WARNS} -mno-cygwin
LDFLAGS = -mwindows -s
#LDFLAGS = -mwindows
WINDRES = windres.exe

all : ${OBJS}
	${CC} -o ${EXE} ${OBJS} ${LIB_DIRS} -leay32 -lWinInet ${LDFLAGS}
#	${CC} -o ${EXE} ${OBJS} ${LIB_DIRS} -lWinInet ${LDFLAGS}

clean :
	rm -f *.o *.exe *.res

%.o : %.c ${HEADERS}
	${CC} ${CFLAGS} ${INCLUDE_DIRS} -c $< -o $@

openvpn-gui-$(RES_LANG).res : openvpn-gui-$(RES_LANG).rc openvpn-gui-res.h
	$(WINDRES) -i openvpn-gui-$(RES_LANG).rc -I rc -o openvpn-gui-$(RES_LANG).res -O coff 
