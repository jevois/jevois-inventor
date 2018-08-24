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

#include "Utils.H"

#include <QRegularExpression>
#include <QFontMetrics>
#include <QToolTip>

#include <set>

// ##############################################################################################################
QStringList splitLines(QString const & str)
{
  return str.split(QRegularExpression("\\r\\n|\\r|\\n|\\0"));
}

// ##############################################################################################################
QString extractString(QString const & str, QString const & startsep, QString const & endsep)
{
  int idx = str.indexOf(startsep);
  if (idx == -1) return QString();
  idx += startsep.size();

  if (endsep.isEmpty()) return str.mid(idx);
  
  int idx2 = str.indexOf(endsep, idx);
  if (idx2 == -1) return QString();
  return str.mid(idx, idx2 - idx);
}

// ##############################################################################################################
QString splitToolTip(QString text, int width) 
{
  // code from https://stackoverflow.com/questions/4795757/
  // is-there-a-better-way-to-wordwrap-text-in-qtooltip-than-just-using-regexp
  QFontMetrics fm(QToolTip::font()); 
  QString result; 
  
  for (;;)
  { 
    int i = 0; 
    while (i < text.length())
    { 
      if (fm.width(text.left(++i + 1)) > width)
      { 
        int j = text.lastIndexOf(' ', i); 
        if (j > 0) i = j; 
        result += text.left(i) + '\n'; 
        text = text.mid(i+1); 
        break; 
      } 
    } 
    if (i >= text.length()) break; 
  } 
  return result + text; 
}

// ##############################################################################################################
std::map<QString /* category */, std::map<QString /* descriptor */, ParamInfo> >
parseParamInfo(QStringList const & params)
{
  std::map<QString /* category */, std::map<QString /* descriptor */, ParamInfo> > pmap;

  int idx = 0;
  while (idx < params.size())
  {
    if (params[idx] == "N" || params[idx] == "F")
    {
      ParamInfo ps;
      ps.frozen = (params[idx] == "F"); ++idx;
      ps.compname = params[idx++];
      ps.category = params[idx++];
      ps.name = params[idx++];
      ps.displayname = ps.name;
      ps.valuetype = params[idx++];
      ps.value = params[idx++];
      ps.defaultvalue = params[idx++];
      ps.validvalues = params[idx++];
      ps.description = params[idx++];
      
      pmap[ps.category][ps.descriptor()] = ps;
    } else ++idx;
  }
  
  // Find any duplicate names and update those to use partial descriptor instead of name:
  std::map<QString /* name */, std::set<QString /* descriptor */>> nameset;
  for (auto const & pc : pmap)
    for (auto const & pd : pc.second)
      nameset[pd.second.displayname].insert(pd.first);

  for (auto const & ns : nameset)
    if (ns.second.size() > 1)
    {
      // Let us find the minimum discriminating descriptor: for that, we just split each descriptor; then iteratively
      // try to remove the leftmost field, if it is the same across all duplicates:
      QList<QStringList> splid;
      for (QString const & s : ns.second) splid.push_back(s.split(':'));
      
      QString pfx;
      while (true)
      {
	std::set<QString> leading;
	for (QStringList const & sl : splid)
	  if (sl.isEmpty()) leading.insert("empty elem"); else leading.insert(sl[0]);

	if (leading.empty() || leading.size() > 1) break; // got different prefixes, stop here

	// All first words are the same, remove them:
	pfx += (*leading.begin()) + ':';
	for (auto & sl : splid) sl.pop_front();
      }
      int idx = pfx.length();
      
      for (auto & pc : pmap)
	for (auto & pd : pc.second)
	{
	  ParamInfo & p = pd.second;
	  if (p.displayname == ns.first) p.displayname = pd.first.mid(idx);
	}
    }

  return pmap;
}
