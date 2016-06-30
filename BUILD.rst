How to build using Cygwin
=========================

Cygwin provides ports of many GNU/Linux tools and a POSIX API layer. This is
the most complete way to get the GNU/Linux terminal feel under Windows.
Cygwin has a setup that helps you install all the tools you need.

This document describes how to build openvpn-gui using Cygwin. It cross-compiles
a native Windows executable, using the MinGW-w64 compilers that are available
as packages in the Cygwin repository.


Required packages
-----------------

To build openvpn-gui you need to have these packages installed, including
their dependencies. You can install these packages using the standard
``setup.exe`` of Cygwin.

- autoconf
- automake
- pkg-config
- make
- mingw64-x86_64-gcc-core
- mingw64-x86_64-g++
- mingw64-x86_64-openssl


Build
-----

To build use these commands:

.. code-block:: bash

	autoreconf -iv
	./configure --host=x86_64-w64-mingw32
	make


32-bit or 64-bit
----------------

The above describes how to build the 64-bit version of openvpn-gui. If you
want to build the 32-bit version, simply replace ``x86_64`` with ``i686``.

Both 32-bit and 64-bit version of Cygwin can build the 32-bit and 64-bit
version of ``openvpn-gui.exe``. Just install the packages you need and use
the right ``--host`` option.

How to build using openvpn-build
================================

The `OpenVPN cross-compile buildsystem
<https://github.com/OpenVPN/openvpn-build>`_ builds OpenVPN GUI along all the
other OpenVPN dependencies. Instructions and automated scripts for setting up
the buildsystem are available on the
`Building OpenVPN using the generic buildsystem <https://community.openvpn.net/openvpn/wiki/BuildingUsingGenericBuildsystem>`_
page on the OpenVPN community Wiki.
