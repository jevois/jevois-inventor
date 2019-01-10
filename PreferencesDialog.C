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

#include "PreferencesDialog.H"
#include "Config.H"
#include "JeVoisInventor.H"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QDialogButtonBox>
#include <QPushButton>

// ##############################################################################################################
PreferencesDialog::PreferencesDialog(JeVoisInventor * jvinv, QWidget * parent) :
    QDialog(parent), m_inv(jvinv)
{
  setWindowModality(Qt::ApplicationModal);

  QSettings settings;
  QVBoxLayout * vlay = new QVBoxLayout(this);
  QFormLayout * lay = new QFormLayout();

  // Headless start:
  if (settings.value(SETTINGS_HEADLESS, false).toBool()) m_headless.setCheckState(Qt::Checked);
  connect(&m_headless, &QCheckBox::stateChanged, [this](int value) {
      if (value == Qt::Checked) QSettings().setValue(SETTINGS_HEADLESS, true);
      else QSettings().setValue(SETTINGS_HEADLESS, false);
    });

  lay->addRow(tr("Start in headless mode:"), &m_headless);

  vlay->addLayout(lay);

  // Default video mapping:
  QString const currdef = settings.value(SETTINGS_DEFMAPPING, m_inv->m_currmapping.str()).toString();
  QList<VideoMapping> const & vmlist = m_inv->m_vm;
  for (VideoMapping const & vm : vmlist)
  {
    if (vm.ofmt != "YUYV") continue; // only support YUYV out for now

    m_defmap.addItem(vm.ostr());
    if (vm.str() == currdef) m_defmap.setCurrentIndex(m_defmap.count() - 1);
  }
  connect(&m_defmap, QOverload<int>::of(&QComboBox::currentIndexChanged),  [this]() {
      QString const val = m_defmap.currentText();
      QList<VideoMapping> const & vmlist = m_inv->m_vm;
      for (VideoMapping const & vm : vmlist)
        if (vm.ostr() == val) QSettings().setValue(SETTINGS_DEFMAPPING, vm.str());
    } );

  lay->addRow(tr("Default vision module:"), &m_defmap);

  // Our button box:
  QPushButton * okb = new QPushButton(tr("Ok"));
  connect(okb, &QPushButton::clicked, [this](bool) { accept(); } );
  QDialogButtonBox * bbox = new QDialogButtonBox();
  bbox->addButton(okb, QDialogButtonBox::AcceptRole);
  vlay->addWidget(bbox);
  
  setWindowTitle(tr("Preferences"));
  setMinimumHeight(200);
}

