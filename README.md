Launcher
========

Yet Another RISC OS Application Launcher


Introduction
------------

Launcher is yet another application launcher for RISC OS, which sits to the side of the desktop and allows applications to be started from a palette of shortcuts. It can slide out of sight when not in use, and can be quickly brought back into view with a click of the mouse.

Despite being in use on my desktop since 2003, it took sixteen years and many nudges from people who saw it on my machine at RISC OS shows and WROCC meetings to make it out into the wild.



Building
--------

Launcher consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making Launcher is a matter of running

	make

from the root folder of the project. This will build everything from source, and assemble a working !Launcher application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is located. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23


Licence
-------

Launcher is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.