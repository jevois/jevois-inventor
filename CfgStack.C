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

#include "CfgStack.H"
#include "Editor.H"
#include "CfgEdit.H"

#include <QLayout>
#include <QLabel>

// ##############################################################################################################
namespace
{
  QString cleanName(QString const & fname)
  {
    if (fname.startsWith("/jevois/config/")) return "JeVois " + fname.mid(15);
    else return "Module's " + fname;
  }

  struct edata
  {
      QString const fname;
      Editor::SaveAction sa;
      QString const deftext;
  };
  
  edata const editordata[] =
  {

    {
      // initscript
      "/jevois/config/initscript.cfg",
      Editor::SaveAction::Reboot,
      "# JeVois initialization script\n"
      "#\n"
      "# This script is run upon startup of the JeVois main engine.\n"
      "# You can here specify commands (like you would type them to\n"
      "# the JeVois command-line interface) to execute upon startup,\n"
      "# even BEFORE a module is loaded.\n"
      "#\n"
      "# Example: load the SaveVideo module with no USB out, start\n"
      "# streaming, and start saving:\n"
      "#\n"
      "# setmapping2 YUYV 320 240 30.0 JeVois SaveVideo\n"
      "# setpar serlog Hard\n"
      "# setpar serout Hard\n"
      "# streamon\n"
    },
    {
      // videomappings
      "/jevois/config/videomappings.cfg",
      Editor::SaveAction::Reboot,
      "# JeVois smart camera operation modes and video mappings\n"
      "#\n"
      "# Format is: <USBmode> <USBwidth> <USBheight> <USBfps> <CAMmode> <CAMwidth> <CAMheight> <CAMfps> <Vendor> <Module> [*]\n"
      "#\n"
      "# CamMode can be only one of: YUYV, BAYER, RGB565\n"
      "# USBmode can be only one of: YUYV, GREY, MJPG, BAYER, RGB565, BGR24, NONE\n"
      "\n"
      "YUYV 320 240 60.0 YUYV 320 240 60.0 JeVois SaveVideo\n"
      "YUYV 640 312 50.0 YUYV 320 240 50.0 JeVois DemoSalGistFaceObj\n"
    },      
    {
      // params.cfg
      "params.cfg",
      Editor::SaveAction::Reload,
      "# Optional parameter setting file to run each time the\n"
      "# module is loaded, BEFORE it is started.\n"
      "#\n"
      "# Only entries of the form \"param = value\" are allowed,\n"
      "# and you can only set the module's parameters here (not\n"
      "# any of the system parameters).\n"
      "#\n"
      "# Example:\n"
      "#\n"
      "# serstyle = Detail\n"
    },
    {
      // script.cfg:
      "script.cfg",
      Editor::SaveAction::Reload,
      "# Optional script to run each time the module is loaded,\n"
      "# AFTER it is started.\n"
      "#\n"
      "# Commands in this file should be exactly as you would type\n"
      "# interactively into the JeVois command-line interface.\n"
      "#\n"
      "# Example:\n"
      "#\n"
      "# setcam brightness 1\n"
      "# setpar cpumax 1200\n"
      "# info\n"
    },
  };
}

// ##############################################################################################################
void CfgStack::reset()
{
  for (int i = 0; i < m_stack.count(); ++i)
  {
    auto wi = dynamic_cast<Editor *>(m_stack.widget(i));
    if (wi) wi->reset();
  }
}

// ##############################################################################################################
CfgStack::CfgStack(Serial * serport, QWidget * parent) :
    QWidget(parent)
{
  for (auto const & ed : editordata)
  {
    Editor * editor = new Editor(serport, ed.fname, true, ed.deftext, new CfgEdit(serport), ed.sa, false);
    int n = m_stack.addWidget(editor);
    m_qbox.addItem(cleanName(ed.fname), n);
    connect(editor, &Editor::askReboot, [this]() { emit askReboot(); });
  }

#ifdef Q_OS_MACOS
  // Nasty box size bug...
  m_qbox.setFixedWidth(300);
#endif
  
  QVBoxLayout * vl = new QVBoxLayout(this);
  vl->setMargin(JVINV_MARGIN); vl->setSpacing(JVINV_SPACING);

  QHBoxLayout * hl = new QHBoxLayout;
  hl->setMargin(0); hl->setSpacing(0);
  hl->addWidget(new QLabel("JeVois configuration editor for "));
  hl->addWidget(&m_qbox);
  hl->addStretch(10);
  vl->addLayout(hl);
  vl->addLayout(&m_stack);

  connect(&m_qbox, SIGNAL(activated(int)), this, SLOT(boxselect(int)));
}

// ##############################################################################################################
void CfgStack::edited(QStringList & filenames) const
{
  for (int i = 0; i < m_stack.count(); ++i)
  {
    auto wi = dynamic_cast<Editor *>(m_stack.widget(i));
    if (wi) wi->edited(filenames);
  }
}

// ##############################################################################################################
void CfgStack::tabselected()
{
  QWidget * w = m_stack.currentWidget();
  Editor * cfg = dynamic_cast<Editor*>(w);
  if (cfg) cfg->tabselected();

  update(); // needed for macOS to not mess-up the QComboBox size?
}

// ##############################################################################################################
void CfgStack::boxselect(int idx)
{
  int n = m_qbox.itemData(idx).toInt();
  m_stack.setCurrentIndex(n);
  tabselected();
}

// ##############################################################################################################
void CfgStack::setHighlighter(std::map<QString /* command */, QString /* description */> cmd,
			      std::map<QString /* command */, QString /* description */> modcmd,
			      QStringList const & paramNames,
			      QStringList const & ctrlNames)
{
  for (int i = 0; i < m_stack.count(); ++i)
  {
    Editor * ed = qobject_cast<Editor *>(m_stack.widget(i));
    if (ed)
    {
      CfgEdit * cfg = qobject_cast<CfgEdit *>(ed->baseEdit());
      if (cfg) cfg->setHighlighter(cmd, modcmd, paramNames, ctrlNames);
    }
  }
}

