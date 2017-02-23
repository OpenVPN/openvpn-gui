
autoreconf -iv
unset CC
unset CXX

if [ "${ARCH}" = "x86" ]; then 
	export ABBR="i686" 
	export CHOST="i686-w64-mingw32"
fi

if [ "${ARCH}" = "x86_64" ]; then 
        export ABBR="x86_64"
	export CHOST="x86_64-w64-mingw32"
fi

export CC=${ABBR}-w64-mingw32-gcc

./configure --prefix=/ --libdir=/lib --host=$CHOST --build=i686-pc-cygwin --program-prefix=''
make
