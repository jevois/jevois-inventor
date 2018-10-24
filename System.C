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

#include "System.H"

#include "JeVoisInventor.H"

#include <QFormLayout>
#include <QMessageBox>

// ##############################################################################################################
System::System(JeVoisInventor * inv, Serial * serial, Camera * cam, QWidget * parent) :
    QWidget(parent),
    m_inv(inv),
    m_serial(serial),
    m_cam(cam),
    m_stream(tr("Go Headless")),
    m_usbsd(tr("Enable")),
    m_date(tr("Set Now")),
    m_headless(false)
{
  QFormLayout * lay = new QFormLayout(this);
  
  // Video streaming button:
  connect(&m_stream, &QPushButton::clicked, [this]() {
      if (QMessageBox::question(this, tr("Switch to headless mode?"),
         tr("<p>Before you disable video streaming and switch to headless mode, "
            "you should add commands to initscript.cfg to load and start a vision module "
            "with no video output. For example:</p>"
            "<p><tt>setmapping2&nbsp;YUYV&nbsp;640&nbsp;480&nbsp;30.0&nbsp;JeVois&nbsp;DemoArUco</tt></p>"
            "<p><tt>setpar serout USB</tt></p>"
            "<p><tt>streamon</tt></p>"
            "<p>Also note that to go headless and run the init script, we "
            "will restart the camera and disable the vision module selection menu. "
            "To get out of headless operation, you will "
            "have to quit and restart the Inventor.</p><p>&nbsp;</p>"
            "<p>Switch to headless mode now?</p>")) != QMessageBox::Yes)
        return;
      m_stream.setEnabled(false);
      m_usbsd.setEnabled(false);
      m_headless = true;
      m_inv->rebootJeVois();
    });
  lay->addRow(tr("Video streaming:"), &m_stream);
  
  // usbsd toggle:
  m_usbsd.setCheckable(true);
  connect(&m_usbsd, &QPushButton::toggled, [this](bool checked) {
      if (checked)
      {
        m_stream.setEnabled(false);
        m_cam->setStreaming(false);
        m_serial->command("usbsd");
        QMessageBox::information(this, tr("JeVois microSD exported to host computer"),
                                 tr("The microSD card inside JeVois was exported to your host computer. "
                                    "A new USB flash drive should appear momentarily on your desktop. Video "
                                    "streaming is not possible while the microSD card is exported.<br><br> "
                                    "When you are done, properly eject the virtual flash drive from your desktop "
                                    "and JeVois will restart."), QMessageBox::Ok);
        
        // Freeze the UI
        m_inv->enableUI(false);
        
        // Reset m_stream as it will be true after we reboot
        m_stream.setChecked(true);
      }
    });
  lay->addRow(tr("Export microSD card inside JeVois to host computer:"), &m_usbsd);
  
  // date/time:
  connect(&m_date, &QPushButton::clicked, [this]() {
      std::time_t t = std::time(nullptr); char str[100];
      std::strftime(str, sizeof(str), "%m%d%H%M%Y.%S", std::localtime(&t));
      m_serial->
        command("date " + QString(str),
                [this](QStringList const & ans) {
                  if (ans.isEmpty()) return;
                  QMessageBox::information(this, tr("JeVois date/time set"),
                                           tr("Date and time have been successfully set on JeVois:\n\n") + ans[0] +
                                           tr("\n\nRemember that this will be lost next time JeVois restarts "
                                              "or is powered off, as JeVois has no clock battery."), QMessageBox::Ok);
                });
    });
  lay->addRow(tr("Set current date/time on JeVois:"), &m_date);
  
}

// ##############################################################################################################
System::~System()
{ }

// ##############################################################################################################
bool System::isHeadless() const
{ return m_headless; }

