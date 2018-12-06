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


#include "TopPanel.H"


// ##############################################################################################################
TopPanel::TopPanel(QWidget * parent) :
    QWidget(parent),
    m_icon(),
    m_modname(),
    m_moddesc(),
    m_modauth(),
    m_modlang(),
    m_vlayout(this),
    m_hlayout(),
    m_vlayout2()
{
  reset();
  
  m_modname.setTextFormat(Qt::RichText);
  m_modname.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  m_modname.setStyleSheet("QLabel { font-size: 32px; font-weight: 900; }");

  m_moddesc.setTextFormat(Qt::RichText);
  m_moddesc.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  m_moddesc.setStyleSheet("QLabel { font-size: 20px; font-weight: 700; }");

  m_modauth.setTextFormat(Qt::RichText);
  m_modauth.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  m_modauth.setStyleSheet("QLabel { font-weight: 500; }");

  m_modlang.setTextFormat(Qt::RichText);
  m_modlang.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  // Select dark or light blue depending on background color (eg, MacOS Mojave dark mode):
  if (m_modlang.palette().brush(QPalette::Base).color().lightness() > 127)
    m_modlang.setStyleSheet("QLabel { color: navy; font-weight: 500; }");
  else
    m_modlang.setStyleSheet("QLabel { color: lightblue; font-weight: 500; }");

  m_icon.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  m_vlayout2.addWidget(&m_modname);
  m_vlayout2.addWidget(&m_moddesc);
  
  m_hlayout.addWidget(&m_icon);
  m_hlayout.addLayout(&m_vlayout2);
  m_hlayout.setSpacing(10);
  
  m_vlayout.addLayout(&m_hlayout);

  //QFrame * line = new QFrame();
  //line->setFrameShape(QFrame::HLine);
  //line->setFrameShadow(QFrame::Sunken);
  //m_vlayout.addWidget(line);
    
  m_vlayout.addWidget(&m_modauth);

  QFrame * line2 = new QFrame();
  line2->setFrameShape(QFrame::HLine);
  line2->setFrameShadow(QFrame::Sunken);
  m_vlayout.addWidget(line2);

  m_vlayout.addWidget(&m_modlang);

  QFrame * line3 = new QFrame();
  line3->setFrameShape(QFrame::HLine);
  line3->setFrameShadow(QFrame::Sunken);
  m_vlayout.addWidget(line3);

  m_vlayout.setSpacing(JVINV_SPACING);
}

// ##############################################################################################################
TopPanel::~TopPanel()
{ }

// ##############################################################################################################
void TopPanel::reset()
{
  m_icon.clear();
  m_modname.setText("JeVois Smart Machine Vision");
  m_moddesc.setText("Open-source machine vision for PC/Mac/Linux/Arduino");
  m_modauth.clear();
  m_modlang.clear();
}
