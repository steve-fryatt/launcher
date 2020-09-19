# Copyright 2012-2020, Stephen Fryatt
#
# This file is part of Launcher:
#
#   http://www.stevefryatt.org.uk/software/
#
# Licensed under the EUPL, Version 1.2 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := launcher

APP := !Launcher

PACKAGE := Launcher
PACKAGELOC := Desktop

OBJS =  appdb.o		\
	choices.o	\
	edit_button.o	\
	edit_panel.o	\
	filing.o	\
	icondb.o	\
	main.o		\
	objutil.o	\
	panel.o		\
	paneldb.o	\
	proginfo.o

include $(SFTOOLS_MAKE)/CApp

