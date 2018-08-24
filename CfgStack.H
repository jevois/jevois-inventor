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

#include "Config.H"

#include <QComboBox>
#include <QStackedLayout>

class Serial;

class CfgStack : public QWidget
{
    Q_OBJECT

  public:
    CfgStack(Serial * serport, QWidget * parent = 0);
    void tabselected();
    void reset();
    void setHighlighter(std::map<QString /* command */, QString /* description */> cmd,
			std::map<QString /* command */, QString /* description */> modcmd,
			QStringList const & paramNames,
			QStringList const & ctrlNames);
    void edited(QStringList & filenames) const;
	
  public slots:
    void boxselect(int idx);

  signals:
    void askReboot();
    
  private:
    QComboBox m_qbox;
    QStackedLayout m_stack;
};

