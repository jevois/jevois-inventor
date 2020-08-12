// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
// California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
//
// This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
// redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
// Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.  You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
// Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QDebug>

//#define DEBU(x) do { qCritical() << __FILE__ << "::" << __FUNCTION__ << ":" << x; } while (0)
#define DEBU(x) do { } while (0)

#define INFO(x) do { qCritical() << __FILE__ << "::" << __FUNCTION__ << ":" << x; } while (0)

//! Our version number
#define JVINV_VERSION_MAJOR 1
#define JVINV_VERSION_MINOR 0
#define JVINV_VERSION_PATCH 0

//! Macro to stringify a value
#define JVINV_STRING(s) JVINV_STR(s)

//! Macro to stringify a value
#define JVINV_STR(s) #s

//! Software version, as string
#define JVINV_VERSION_STRING JVINV_STRING(JVINV_VERSION_MAJOR) "." JVINV_STRING(JVINV_VERSION_MINOR) "." JVINV_STRING(JVINV_VERSION_PATCH)

#define JVINV_MARGIN 5
#define JVINV_SPACING 2

#define JVINV_TAB 4

#define JEVOIS_VERSION_URL "http://jevois.org/version"

//! Setting:String: default directory for load/save of local code
#define SETTINGS_LOCAL_DIR "default_local_dir"

//! Setting:bool: start in headless mode
#define SETTINGS_HEADLESS "headless"

//! Setting:String: default video mapping to load on startup (when not headless)
#define SETTINGS_DEFMAPPING "defmapping"
