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

#include "SpinRangeSlider.H"

#include <QLabel>

// ##############################################################################################################
SpinRangeSlider::SpinRangeSlider(int mini, int maxi, QWidget * parent) :
    QWidget(parent), m_layout(), m_slider(), m_lowerspinbox(), m_upperspinbox()
{
  m_slider.SetRange(mini, maxi);
  m_slider.setLowerValue(mini);
  m_slider.setUpperValue(mini);
  m_slider.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  m_lowerspinbox.setRange(mini, maxi);
  m_lowerspinbox.setSingleStep(1);
  m_lowerspinbox.setValue(mini);
  m_lowerspinbox.setFixedWidth(50);
  
  m_upperspinbox.setRange(mini, maxi);
  m_upperspinbox.setSingleStep(1);
  m_upperspinbox.setValue(mini);
  m_upperspinbox.setFixedWidth(50);

  m_layout.setMargin(0);
  m_layout.setSpacing(0);
  m_layout.addWidget(&m_lowerspinbox);
  m_layout.addWidget(new QLabel(" "));
  m_layout.addWidget(&m_slider);
  m_layout.addWidget(new QLabel(" "));
  m_layout.addWidget(&m_upperspinbox);

  m_layout.setAlignment(&m_slider, Qt::AlignVCenter);
  m_layout.setAlignment(&m_lowerspinbox, Qt::AlignVCenter);
  m_layout.setAlignment(&m_upperspinbox, Qt::AlignVCenter);
  m_slider.setAttribute(Qt::WA_LayoutUsesWidgetRect);
  m_lowerspinbox.setAttribute(Qt::WA_LayoutUsesWidgetRect);
  m_upperspinbox.setAttribute(Qt::WA_LayoutUsesWidgetRect);

  setLayout(&m_layout);
  
  connect(&m_slider, SIGNAL(lowerValueChanged(int)), &m_lowerspinbox, SLOT(setValue(int)));
  connect(&m_slider, SIGNAL(upperValueChanged(int)), &m_upperspinbox, SLOT(setValue(int)));
  connect(&m_lowerspinbox, SIGNAL(valueChanged(int)), this, SLOT(setLowerValue(int)));
  connect(&m_lowerspinbox, SIGNAL(valueChanged(int)), &m_slider, SLOT(setLowerValue(int)));
  connect(&m_upperspinbox, SIGNAL(valueChanged(int)), this, SLOT(setUpperValue(int)));
  connect(&m_upperspinbox, SIGNAL(valueChanged(int)), &m_slider, SLOT(setUpperValue(int)));
}

// ##############################################################################################################
SpinRangeSlider::~SpinRangeSlider()
{ }

// ##############################################################################################################
void SpinRangeSlider::setLowerValue(int val)
{
  m_lowerspinbox.setValue(val);
  m_upperspinbox.setMinimum(val);
  emit lowerValueChanged(val);
}

// ##############################################################################################################
void SpinRangeSlider::setUpperValue(int val)
{
  m_upperspinbox.setValue(val);
  m_lowerspinbox.setMaximum(val);
  emit upperValueChanged(val);
}

// ##############################################################################################################
int SpinRangeSlider::GetLowerValue() const
{ return m_slider.GetLowerValue(); }

// ##############################################################################################################
int SpinRangeSlider::GetUpperValue() const
{ return m_slider.GetUpperValue(); }

