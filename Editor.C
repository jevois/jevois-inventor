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

#include "Editor.H"

#include "CfgEdit.H"
#include "Serial.H"
#include "PythonEdit.H"
#include "CxxEdit.H"
#include "Utils.H"

#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFontMetrics>

#include <QDebug>

// ##############################################################################################################
Editor::Editor(Serial * serport, QString const & fname, bool pasteButtons, QString const & deftext, BaseEdit * edit,
	       Editor::SaveAction sa, bool margin, QWidget * parent) :
    QWidget(parent),
    m_edit(edit),
    m_serial(serport),
    m_fname(),
    m_deftext(deftext),
    m_saveaction(sa),
    m_load(tr("Load from JeVois")),
    m_save(tr("Save to JeVois")),
    m_locload(tr("Load from local")),
    m_locsave(tr("Save to local")),
    m_pastecam(tr("C")),
    m_pastepar(tr("P")),
    m_vlayout(this)
{
  setFile(fname);
  
  if (margin == false) m_vlayout.setMargin(0); // zero margin unless standalone source editor
  else m_vlayout.setMargin(JVINV_MARGIN);
  m_vlayout.setSpacing(JVINV_SPACING);
  m_vlayout.addWidget(m_edit);
  auto hlayout = new QHBoxLayout();
  hlayout->setMargin(2); hlayout->setSpacing(5);
  hlayout->addWidget(&m_load);
  m_save.setToolTip(QKeySequence(QKeySequence::Save).toString());
  hlayout->addWidget(&m_save);

  if (pasteButtons)
  {
    hlayout->addStretch(10);
    m_pastecam.setToolTip(tr("Paste current camera settings"));
    hlayout->addWidget(&m_pastecam);    
    m_pastepar.setToolTip(tr("Paste current module parameter values"));
    hlayout->addWidget(&m_pastepar);
    hlayout->addStretch(10);

#ifdef Q_OS_MACOS
    // Nasty box size bug...
    m_pastecam.setFixedWidth(40);
    m_pastepar.setFixedWidth(40);
#else
    QFontMetrics fm(m_pastecam.font()); 
    m_pastecam.setMinimumWidth(fm.width("AAA"));
    m_pastecam.setMaximumWidth(fm.width("AAA"));
    m_pastecam.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_pastepar.setMinimumWidth(fm.width("AAA"));
    m_pastepar.setMaximumWidth(fm.width("AAA"));
    m_pastepar.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
#endif
    
    connect(&m_pastecam, &QPushButton::clicked, [this](bool)
	    { m_serial->command("caminfo", [this](QStringList const & ci) { pasteCam(ci); }); });
    connect(&m_pastepar, &QPushButton::clicked, [this](bool)
	    { m_serial->command("paraminfo modhot", [this](QStringList const & ci) { pastePar(ci); }); });
  }
  else hlayout->addStretch(10);

  hlayout->addWidget(&m_locload);
  m_locsave.setToolTip(QKeySequence(QKeySequence::SaveAs).toString());
  hlayout->addWidget(&m_locsave);
  m_vlayout.addLayout(hlayout);

  connect(&m_load, SIGNAL(clicked(bool)), this, SLOT(loadFile(bool)));
  connect(&m_save, SIGNAL(clicked(bool)), this, SLOT(saveFile(bool)));
  connect(&m_locload, SIGNAL(clicked(bool)), this, SLOT(loadFileLocal(bool)));
  connect(&m_locsave, SIGNAL(clicked(bool)), this, SLOT(saveFileLocal(bool)));

  connect(m_edit, SIGNAL(saveRequest(bool)), this, SLOT(saveFile(bool)));
  connect(m_edit, SIGNAL(localSaveRequest(bool)), this, SLOT(saveFileLocal(bool)));
}

// ##############################################################################################################
Editor::~Editor()
{ }

// ##############################################################################################################
void Editor::edited(QStringList & filenames) const
{
  if (m_edit->edited()) filenames.push_back(m_fname);
}

// ##############################################################################################################
BaseEdit * Editor::baseEdit()
{ return m_edit; }

// ##############################################################################################################
void Editor::setBaseEdit(BaseEdit * be)
{
  // Find the widget inside m_vlayout and replace it with the new one:
  if (m_vlayout.replaceWidget(m_edit, be))
  {
    delete m_edit;
    m_edit = be;
    connect(m_edit, SIGNAL(saveRequest(bool)), this, SLOT(saveFile(bool)));
    connect(m_edit, SIGNAL(localSaveRequest(bool)), this, SLOT(saveFileLocal(bool)));
  }
}

// ##############################################################################################################
void Editor::reset()
{
  m_edit->setData("");
}

// ##############################################################################################################
void Editor::tabselected()
{
  DEBU("Editor selected: " << m_fname);
  if (m_edit->empty()) loadFile(true);
}

// ##############################################################################################################
void Editor::setFile(QString const & fname)
{
  if (fname.endsWith("script.cfg"))
  {
    m_pastecam.setEnabled(true);
    m_pastepar.setEnabled(true);
  }
  else if (fname.endsWith("params.cfg"))
  {
    m_pastecam.setEnabled(false);
    m_pastepar.setEnabled(true);
  }
  else if (fname.endsWith(".cfg"))
  {
    m_pastecam.setEnabled(false);
    m_pastepar.setEnabled(false);
  }
  else if (fname.endsWith(".C"))
  {
    setBaseEdit(new CxxEdit(m_serial));

    // We cannot yet edit C++ code...
    m_load.setEnabled(false);
    m_save.setEnabled(false);
    m_locload.setEnabled(false);
    m_locsave.setEnabled(false);

    m_edit->setReadOnly(true);
    // But still show text as black (not gray):
    auto pal = m_edit->palette();
    pal.setColor(QPalette::Text, Qt::black);
    m_edit->setPalette(pal);
  }
  else
  {
    setBaseEdit(new PythonEdit(m_serial));

    // All other files can be edited:
    m_load.setEnabled(true);
    m_save.setEnabled(true);
    m_locload.setEnabled(true);
    m_locsave.setEnabled(true);
    m_edit->setReadOnly(false);
   }
  
  reset();
  m_fname = fname;
}

// ##############################################################################################################
void Editor::loadFile(bool noask)
{
  DEBU("Loading: " << m_fname);
  
  bool doit = true;
  if (noask == false && m_edit->edited() &&
      QMessageBox::question(this, tr("Confirm load from JeVois"),
			    tr("Discard edits and load from JeVois?"), QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::Yes) != QMessageBox::Yes)
    doit = false;

  if (doit)
  {
    m_serial->receiveTextBuffer(m_fname,
				[this](QStringList const & txt) { m_edit->setData(txt.join("\n")); },
				[this](QStringList const &) { m_edit->setData(m_deftext); }
				);
  }
}

// ##############################################################################################################
void Editor::saveFile(bool noask)
{
  if (m_save.isEnabled() == false) return; // pressed ctrl-s but save is disabled?
  
  bool doit = true;
  if (noask == false &&
      QMessageBox::question(this, tr("Confirm save to JeVois"), tr("Save file to JeVois?"),
			    QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::Yes) != QMessageBox::Yes)
    doit = false;

  if (doit)
  {
    m_serial->sendBuffer(m_fname, m_edit->toPlainText().toLatin1(),
			 [this](QStringList const & txt) { afterSave(txt); },
			 [this](QStringList const & err) {
			   QMessageBox::critical(this, tr("Save file to JeVois failed!"),
						 tr("Save file to JeVois failed with error:\n\n") +
						 err.join('\n') + tr("\n\nMaybe your microSD card has errors "
								     "and you need to restart JeVois..."),
						 QMessageBox::Ok);
			 } );
    m_edit->document()->setModified(false);
  }
}

// ##############################################################################################################
void Editor::afterSave(QStringList const & data)
{
  Q_UNUSED(data);

  switch (m_saveaction)
  {
  case Editor::SaveAction::None:
    break;

  case Editor::SaveAction::Reload:
  {
    if (QMessageBox::question(this, tr("Reload module now?"),
			      tr("The machine vision module must be re-loaded for changes "
				 "to take effect.\n\nRe-load module now?"),
			      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
      m_serial->command("reload");
  }
  break;

  case Editor::SaveAction::Reboot:
  {
    if (QMessageBox::question(this, tr("Restart JeVois camera?"),
			      tr("JeVois must restart for changes to take effect.\n\nThis takes about 15 seconds.\n\n"
				 "Restart JeVois now?"),
			      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
      emit askReboot();
    break;
  }
  }
}

// ##############################################################################################################
void Editor::loadFileLocal(bool noask)
{
  bool doit = true;
  if (noask == false && m_edit->edited() &&
      QMessageBox::question(this, tr("Confirm load from local"),
			    tr("Discard edits and load from local file?"),
			    QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::Yes) != QMessageBox::Yes)
    doit = false;

  if (doit)
  {
    QStringList locs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    QString loc = locs.isEmpty() ? "" : locs[0];

    QString const localfname = QFileDialog::getOpenFileName(this, "Select file to load", loc);

    QFile file(localfname);
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
      QByteArray const data = file.readAll();
      m_edit->setData(data);
    }
  }
}

// ##############################################################################################################
void Editor::saveFileLocal(bool noask)
{
  Q_UNUSED(noask);

  if (m_save.isEnabled() == false) return; // pressed ctrl-shift-s but save is disabled?
  
  QByteArray const data =  m_edit->toPlainText().toLatin1();
  QStringList locs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
  QString loc = locs.isEmpty() ? "" : locs[0];
  loc = QDir::cleanPath(loc); // convert to forward slash
  QStringList const fn = m_fname.split('/');
  loc += '/' + fn.back();
  
  QString const localfname = QFileDialog::getSaveFileName(this, "Select file to save to", loc);
  if (localfname.isEmpty()) return;
  
  QFile file(localfname);
  if (file.open(QFile::WriteOnly | QFile::Text)) file.write(data);
  else QMessageBox::critical(this, tr("Failed to save to local file"),
			     tr("Saving to local file failed. Check filename and permissions."));
}

// ##############################################################################################################
void Editor::pasteCam(QStringList const & ci)
{
  // keep this in sync with CamControls!
  QString txt;
  bool autowb = false, autogain = false, autoexp = false;
  
  for (auto const & str : ci)
  {
    QStringList vec = str.split(QRegularExpression("\\s+"));

    if (vec[0] == "OK") continue;

    // Get the control:
    QString const name = vec[0];    
    QString val;
    if (vec[1] == "I") val = vec[6];      // name I min max step def curr
    else if (vec[1] == "B") val = vec[3]; // name B def curr
    else if (vec[1] == "M") val = vec[3]; // name M def curr 1:name1 2:name2 ...
    else { DEBU("Camera control of unknown type " << vec[1] << " ignored"); continue; }

    // The controls are assumed to be ordered so that the "auto" guys come before their dependent manual settings:
    if (name == "dowb") continue;
    else if (name == "autowb" && val == "1") autowb = true;
    else if (name == "autogain" && val == "1") autogain = true;
    else if (name == "autoexp" && val == "0") autoexp = true; // caution, 0 means auto mode

    if (autowb && (name == "redbal" || name == "bluebal")) continue;
    if (autogain && name == "gain") continue;
    if (autoexp && name == "absexp") continue;
    
    // All right, just set the value:
    txt += "setcam " + name + ' ' + val + '\n';
  }
 
  if (txt.isEmpty() == false)
  {
    m_edit->insertPlainText("# Camera controls from JeVois Inventor:\n" + txt);
    m_edit->setFocus();
  }
}

// ##############################################################################################################
void Editor::pastePar(QStringList const & params)
{
  // keep this in sync with Parameters!
  QString txt;
  bool script = m_fname.endsWith("script.cfg");
  std::map<QString /* category */, std::map<QString /* descriptor */, ParamInfo> > pmap = parseParamInfo(params);

  // Just set the value:
  if (script)
    for (auto const & pc : pmap)
      for (auto const & pd : pc.second)
	txt += "setpar " + pd.second.displayname + ' ' + pd.second.value + '\n';
  else
    for (auto const & pc : pmap)
      for (auto const & pd : pc.second)
	txt += pd.second.displayname + " = " + pd.second.value + '\n';

  if (txt.isEmpty() == false)
  {
    m_edit->insertPlainText("# Parameters from JeVois Inventor:\n" + txt);
    m_edit->setFocus();
  }
}
