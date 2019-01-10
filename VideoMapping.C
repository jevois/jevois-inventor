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

#include "VideoMapping.H"

#include <QTextStream>
#include <QRegularExpression>

// ####################################################################################################
QString VideoMapping::path() const
{
  return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename;
}

// ####################################################################################################
QString VideoMapping::sopath() const
{
  if (ispython) return path() + '/' + modulename + ".py";
  else return path() + '/' + modulename + ".so";
}

// ####################################################################################################
QString VideoMapping::srcpath() const
{
  if (ispython) return path() + '/' + modulename + ".py";
  else return path() + '/' + modulename + ".C";
}

// ####################################################################################################
QString VideoMapping::ostr() const
{
  QString s; QTextStream ss(&s);
  ss << ofmt << ' ' << ow << 'x' << oh << " @ " << ofps << "fps - " << vendor << ' ' << modulename <<
    " (" << (ispython ? "Python" : "C++") << ')';
  return s;
}

// ####################################################################################################
QString VideoMapping::str() const
{
  QString s; QTextStream ss(&s);
  ss << "OUT: " << ofmt << ' ' << ow << 'x' << oh << " @ " << ofps << "fps CAM: " <<
    cfmt << ' ' << cw << 'x' << ch << " @ " << cfps << "fps MOD: " <<
    vendor << ':' << modulename << ' ' << (ispython ? "Python" : "C++");
  return s;
}

// ####################################################################################################
VideoMapping::VideoMapping(QString const & str)
{
  QStringList sl = str.split(QRegularExpression("\\s+"));
  if (sl.size() != 13) throw std::runtime_error(("Malformed videomapping: " + str).toStdString());
  
  ofmt = sl[1];
  QStringList qs = sl[2].split('x'); ow = qs[0].toUInt(); oh = qs[1].toUInt();
  sl[4] = sl[4].mid(0, sl[4].size() - 3); ofps = sl[4].toFloat();

  cfmt = sl[6];
  qs = sl[7].split('x'); cw = qs[0].toUInt(); ch = qs[1].toUInt();
  sl[9] = sl[9].mid(0, sl[9].size() - 3); cfps = sl[9].toFloat();

  qs = sl[11].split(':'); vendor = qs[0]; modulename = qs[1];

  if (sl[12] == "Python") ispython = true; else ispython = false;
}

