// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
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
/*! \file */

#pragma once

#include "Config.H"

#include <QString>

#define JEVOIS_MODULE_PATH "/jevois/modules"

//! Simple struct to hold video mapping definitions for the processing Engine
/*! Keep this in sync with VideoMapping in jevois repo. Here we use QString throughout, no dependencies on V4L2, QList
    instead of std::vector, etc. */
class VideoMapping
{
  public:
    //! Parse from a string
    VideoMapping(QString const & str);

    QString ofmt;      //!< output pixel format
    unsigned int ow;   //!< output width
    unsigned int oh;   //!< output height
    float ofps;        //!< output frame rate in frames/sec
    
    QString cfmt;      //!< camera pixel format
    unsigned int cw;   //!< camera width
    unsigned int ch;   //!< camera height
    float cfps;        //!< camera frame rate in frames/sec
    
    QString vendor; //!< Module creator name, used as a directory to organize the modules
    
    QString modulename; //!< Name of the Module that will process this mapping
    
    bool ispython;      //!< True if the module is written in Python; affects behavior of sopath() only
    
    //! Return the full absolute directory of the module
    QString path() const;

    //! Return the full absolute path and file name of the module's .so or .py file
    QString sopath() const;

    //! Return the full absolute path and file name of the module's .C or .py file
    QString srcpath() const;

    //! Convenience function to print out FCC WxH @ fps Vendor Module
    QString ostr() const;

    //! Convenience function to print the whole mapping
    QString str() const;
};

