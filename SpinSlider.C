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

#include "SpinSlider.H"

#include <QLabel>

// ##############################################################################################################
SpinSlider::SpinSlider(int mini, int maxi, int step, Qt::Orientation ori, QWidget * parent) :
    QWidget(parent), m_layout(), m_slider(ori), m_spinbox()
{
  m_slider.setRange(mini, maxi);
  m_slider.setSingleStep(step);
  m_slider.setPageStep(step);
  m_slider.setValue(mini);
  m_slider.setTracking(true);
  m_slider.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  m_spinbox.setRange(mini, maxi);
  m_spinbox.setSingleStep(step);
  m_spinbox.setValue(mini);
  m_spinbox.setFixedWidth(80);

  m_layout.setMargin(0);
  m_layout.setSpacing(0);
  m_layout.addWidget(&m_slider);
  m_layout.addWidget(new QLabel(" "));
  m_layout.addWidget(&m_spinbox);

  m_layout.setAlignment(&m_slider, Qt::AlignVCenter);
  m_layout.setAlignment(&m_spinbox, Qt::AlignVCenter);
  m_slider.setAttribute(Qt::WA_LayoutUsesWidgetRect);
  m_spinbox.setAttribute(Qt::WA_LayoutUsesWidgetRect);
 
  setLayout(&m_layout);
  
  connect(&m_slider, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
  connect(&m_spinbox, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
}

// ##############################################################################################################
SpinSlider::~SpinSlider()
{ }

// ##############################################################################################################
void SpinSlider::setValue(int val)
{
  m_spinbox.setValue(val);
  m_slider.setValue(val);
  emit valueChanged(val);
}

// ##############################################################################################################
int SpinSlider::value() const
{ return m_slider.value(); }
