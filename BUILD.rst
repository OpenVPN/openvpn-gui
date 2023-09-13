How to build with MSVC
======================

This is the recommended way of building openvpn-gui on Windows, which is also used when doing OpenVPN Windows releases.

Prerequisites
-------------

 - Visual Studio 2022 (see below for details)
 - CMake
 - vcpkg (add the environment variable ``VCPKG_ROOT`` which points to vcpkg installation)

Visual Studio Components
------------------------

Having the Build Tools is enough, you do not need the actual IDE.
In addition to the Microsoft.VisualStudio.Workload.VCTools workload,
you might also need the following additional components dependending
on which architectures you build for:

* Microsoft.VisualStudio.Component.VC.ATL.Spectre
* Microsoft.VisualStudio.Component.VC.ATLMFC.Spectre
* Microsoft.VisualStudio.Component.UWP.VC.ARM64
* Microsoft.VisualStudio.Component.VC.Tools.ARM64
* Microsoft.VisualStudio.Component.VC.ATL.ARM64.Spectre
* Microsoft.VisualStudio.Component.VC.MFC.ARM64.Spectre
* Microsoft.VisualStudio.Component.VC.Runtimes.ARM64.Spectre
* Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre

Build steps
-----------

Run inside MSVC command prompt:

.. code-block::

    c:\Temp\openvpn-gui>cmake -S . --preset x64
    c:\Temp\openvpn-gui>cmake --build --preset x64-release

To see all presets, run:

.. code-block::

  C:\Users\lev\Projects\openvpn-gui>cmake -S c:\Users\lev\Projects\openvpn-gui --list-presets
  Available configure presets:
  "x64"
  "arm64"
  "x86"
  "x64-ovpn3"
  "arm64-ovpn3"
  "x86-ovpn3"

You could also open CMake project from MSVC IDE and build from there.

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


How to build using MSYS2
========================

One-time preperation
--------------------

Install MSYS2. Instructions and prerequisites can be found on the official website: https://msys2.github.io/

Once installed use the ``mingw64.exe`` provided by MSYS2.

Update the base MSYS2 system until no further updates are available using:

.. code-block:: bash

	pacman -Syu

You may have to restart your MINGW64 prompt between those updates.

Now install the required development packages:

.. code-block:: bash

    pacman -S base-devel mingw-w64-x86_64-{toolchain}

Build
-----

You can build using these commands:

.. code-block:: bash

    autoreconf -iv
    ./configure
    make

32-bit or 64-bit
----------------

The above describes how to build the 64-bit version of openvpn-gui.
If you want to build the 32-bit version, use the ``mingw32.exe`` and in the package names simply replace ``x86_64`` with ``i686``.
