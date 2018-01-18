#!/bin/bash 

set -e

if [ ! -f download-cache/openssl-${OPENSSL_VERSION}.tar.gz ]; then wget -P download-cache/ https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz; fi

mkdir image
tar zxf download-cache/openssl-${OPENSSL_VERSION}.tar.gz && cd openssl-${OPENSSL_VERSION}
if [ ${CHOST} == "i686-w64-mingw32" ]; then export target=mingw; fi
if [ ${CHOST} == "x86_64-w64-mingw32" ]; then export target=mingw64; fi
./Configure --prefix=/ --cross-compile-prefix=${CHOST}- shared $target no-capieng --openssldir=/etc/ssl --libdir=/lib -static-libgcc >build.log 2>&1 || (cat build.log && exit 1)
make install $([[ ${OPENSSL_VERSION} == "1.0."* ]] && echo "INSTALL_PREFIX" || echo "DESTDIR")="${HOME}/image" INSTALLTOP="/" MANDIR="/tmp" >build.log 2>&1 || (cat build.log && exit 1)
cd .. 

export OPENSSL_CRYPTO_CFLAGS="-I${HOME}/image/include"
export OPENSSL_CRYPTO_LIBS="-L${HOME}/image/lib -lcrypto"

