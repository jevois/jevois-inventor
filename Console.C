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

#include "Console.H"
#include "Serial.H"
#include "Utils.H"

#include <QLayout>
#include <QScrollBar>
#include <QRegularExpression>

#include <QDebug>

// ##############################################################################################################
Console::Console(Serial * serport, QWidget * parent) :
    QWidget(parent),
    m_serial(serport),
    m_serlogusb("USB"),
    m_serloghard("4-pin"),
    m_seroutusb("USB"),
    m_serouthard("4-pin"),
    m_serstyle(),
    m_log(),
    m_input(),
    m_enter("Enter"),
    m_cmdinfo(" ?"),
    m_timer(this)
{
  m_log.setReadOnly(true);
  m_log.setLineWrapMode(QPlainTextEdit::NoWrap);
  m_log.setMaximumBlockCount(1000); // max number of lines
  m_serlogusb.setChecked(true);
  m_serstyle.addItem("Terse");
  m_serstyle.addItem("Normal");
  m_serstyle.addItem("Detail");
  m_serstyle.addItem("Fine");

#ifdef Q_OS_MACOS
  // Nasty box size bug...
  m_serstyle.setFixedWidth(110);
  m_serstyle.setAttribute(Qt::WA_LayoutUsesWidgetRect); // this has no effect, darn...
#endif

  QFont f("unexistent");
  f.setStyleHint(QFont::Monospace);
  m_input.setFont(f);
  m_log.setFont(f);

  cf_cmd = m_log.currentCharFormat();
  cf_cmd.setForeground(Qt::blue);
  cf_serout = cf_cmd;
  cf_serout.setForeground(Qt::black);
  cf_ok = cf_cmd;
  cf_ok.setForeground(Qt::darkGreen);
  cf_dbg = cf_cmd;
  cf_dbg.setForeground(Qt::darkYellow);
  cf_inf = cf_cmd;
  cf_inf.setForeground(Qt::darkCyan);
  cf_err = cf_cmd;
  cf_err.setForeground(Qt::darkRed);
  cf_ftl = cf_cmd;
  cf_ftl.setForeground(Qt::darkBlue);
  
  auto vlayout = new QVBoxLayout(this);
  vlayout->setMargin(JVINV_MARGIN); vlayout->setSpacing(JVINV_SPACING);
  
  auto htop = new QHBoxLayout();
  htop->setMargin(0); htop->setSpacing(2);
  htop->addWidget(new QLabel("Log messages:"));
  m_serlogusb.setCheckable(true);
  htop->addWidget(&m_serlogusb);
  m_serloghard.setCheckable(true);
  htop->addWidget(&m_serloghard);
  htop->addStretch(10);
  htop->addWidget(new QLabel("     "));
  htop->addWidget(new QLabel("Module output:"));
  m_seroutusb.setCheckable(true);
  htop->addWidget(&m_seroutusb);
  m_serouthard.setCheckable(true);
  htop->addWidget(&m_serouthard);
  htop->addWidget(&m_serstyle);
  vlayout->addLayout(htop);
  
  vlayout->addWidget(&m_log);

  auto hbot = new QHBoxLayout();
  hbot->setMargin(0); hbot->setSpacing(0);
  m_input.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  hbot->addWidget(&m_input);
  hbot->addWidget(&m_enter);
  hbot->addWidget(&m_cmdinfo);
  vlayout->addLayout(hbot);

  connect(&m_enter, SIGNAL(released()), this, SLOT(enterPressed()));
  connect(&m_input, SIGNAL(lineExecuted(QString)), this, SLOT(lineExecuted(QString)));
  connect(m_serial, SIGNAL(readyRead()), this, SLOT(readDataReady()));

  connect(&m_seroutusb, &QPushButton::toggled,
          [this](bool checked) {
            if (checked)
            {
              if (m_serouthard.isChecked()) m_serial->command("setpar serout All");
              else m_serial->command("setpar serout USB");
            }
            else
            {
              if (m_serouthard.isChecked()) m_serial->command("setpar serout Hard");
              else m_serial->command("setpar serout None");
            }
          });
  
  connect(&m_serouthard, &QPushButton::toggled,
          [this](bool checked) {
            if (checked)
            {
              if (m_seroutusb.isChecked()) m_serial->command("setpar serout All");
              else m_serial->command("setpar serout Hard");
            }
            else
            {
              if (m_seroutusb.isChecked()) m_serial->command("setpar serout USB");
              else m_serial->command("setpar serout None");
            }
          });
  
  connect(&m_serlogusb, &QPushButton::toggled,
          [this](bool checked) {
            if (checked)
            {
              if (m_serloghard.isChecked()) m_serial->command("setpar serlog All");
              else m_serial->command("setpar serlog USB");
            }
            else
            {
              if (m_serloghard.isChecked()) m_serial->command("setpar serlog Hard");
              else m_serial->command("setpar serlog None");
            }
          });
  
  connect(&m_serloghard, &QPushButton::toggled,
          [this](bool checked) {
            if (checked)
            {
              if (m_serlogusb.isChecked()) m_serial->command("setpar serlog All");
              else m_serial->command("setpar serlog Hard");
            }
            else
            {
              if (m_serlogusb.isChecked()) m_serial->command("setpar serlog USB");
              else m_serial->command("setpar serlog None");
            }
          });


  connect(&m_serstyle, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this]() { m_serial->setPar("serstyle", m_serstyle.currentText()); });

  // Setup our timer to refresh the serout, serlog, serstyle widgets once in a while:
  connect(&m_timer, &QTimer::timeout, [this]() { tabselected(); } );
}

// ##############################################################################################################
void Console::tabselected()
{
  // First get the serout and serlog values from the camera; in the callbacks, we will connect these buttons to actions
  // after we have set the initial button state:
  m_serial->command("serinfo", [this](QStringList const & data) { updateUI(data); });
  m_timer.start(std::chrono::milliseconds(800));

  update(); // needed for macOS to not mess-up the QComboBox sizes?
}

// ##############################################################################################################
void Console::tabunselected()
{
  m_timer.stop();
}

// ##############################################################################################################
Console::~Console()
{
  m_timer.stop();
}

// ##############################################################################################################
void Console::enterPressed()
{
  m_input.execute();
}

// ##############################################################################################################
void Console::lineExecuted(QString str)
{
  m_serial->write(str);

  bool bottom = (m_log.verticalScrollBar()->value() == m_log.verticalScrollBar()->maximum());
  m_log.setCurrentCharFormat(cf_cmd);
  m_log.appendPlainText(str);
  if (bottom) m_log.verticalScrollBar()->setValue(m_log.verticalScrollBar()->maximum());
}

// ##############################################################################################################
void Console::readDataReady()
{
  QStringList sl = m_serial->readAll();
  bool bottom = (m_log.verticalScrollBar()->value() == m_log.verticalScrollBar()->maximum());

  for (QString const & s : sl)
  {
    if (s == "OK") m_log.setCurrentCharFormat(cf_ok);
    else if (s.startsWith("DBG ")) m_log.setCurrentCharFormat(cf_dbg);
    else if (s.startsWith("INF ")) m_log.setCurrentCharFormat(cf_inf);
    else if (s.startsWith("ERR ")) m_log.setCurrentCharFormat(cf_err);
    else if (s.startsWith("FTL ")) m_log.setCurrentCharFormat(cf_ftl);
    else m_log.setCurrentCharFormat(cf_serout);
    m_log.appendPlainText(s);
  }
  if (bottom) m_log.verticalScrollBar()->setValue(m_log.verticalScrollBar()->maximum());
}

// ##############################################################################################################
void Console::updateUI(QStringList const & data)
{
  if (data.size() == 0) return;

  if (data[0].startsWith("ERR ")) return;

  QStringList sl = data[0].split(' ');
  if (sl.size() != 5) return;

  for (int i = 0; i < 2; ++i)
  {
    bool hard, usb;
    if (sl[i] == "Hard") { hard = true; usb = false; }
    else if (sl[i] == "USB") { hard = false; usb = true; }
    else if (sl[i] == "All") { hard = true; usb = true; }
    else if (sl[i] == "None") { hard = false; usb = false; }
    else return;

    if (i == 0)
    {
      // serout:
      if (hard) m_serouthard.setChecked(true); else m_serouthard.setChecked(false);
      if (usb) m_seroutusb.setChecked(true); else m_seroutusb.setChecked(false);
    }
    else
    {
      // serlog:
      if (hard) m_serloghard.setChecked(true); else m_serloghard.setChecked(false);
      if (usb) m_serlogusb.setChecked(true); else m_serlogusb.setChecked(false);
    }
  }

  if (sl[2] == '-')
    m_serstyle.setEnabled(false);
  else
  {
    m_serstyle.setEnabled(true);
    m_serstyle.setCurrentText(sl[2]);
  }

  // sl[3] is serprec (not used in the UI for now)
  // sl[4] is serstamp (not used in the UI for now)
}

// ##############################################################################################################
void Console::updateCmdInfo(std::map<QString /* command */, QString /* description */> cmd,
                            std::map<QString /* command */, QString /* description */> modcmd)
{
  QString tt("<table><tr><td colspan=2><strong>Module-specific commands</strong></td></tr>");
  if (modcmd.empty())
    tt += "<tr><td colspan=2>None</td></tr>";
  else
    for (auto const & cm : modcmd)
      tt += "<tr><td>" + cm.first.toHtmlEscaped() + "</td><td>" + cm.second.toHtmlEscaped() + "</tr>";

  tt += "<tr><td colspan=2> </td></tr><tr><td colspan=2><strong>JeVois commands</strong></td></tr>";
  for (auto const & cm : cmd)
  {
    // Temporary? We blacklist a bunch of machine-oriented commands:
    if (cm.first.startsWith("caminfo")) continue;
    if (cm.first.startsWith("cmdinfo")) continue;
    if (cm.first.startsWith("modcmdinfo")) continue;
    if (cm.first.startsWith("paraminfo")) continue;
    if (cm.first.startsWith("serinfo")) continue;
    if (cm.first.startsWith("fileget")) continue;
    if (cm.first.startsWith("fileput")) continue;
    
    tt += "<tr><td>" + cm.first.toHtmlEscaped() + "</td><td>" + cm.second.toHtmlEscaped() + "</tr>";
  }
  tt += "</table>";

  m_cmdinfo.setToolTip(tt);
}

