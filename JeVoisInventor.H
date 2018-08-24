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

#include <QMainWindow>
#include <QWidget>
#include <QLayout>
#include <QTabWidget>
#include <QToolBar>
#include <QSplitter>
#include <QTextBrowser>
#include <QCamera>
#include <QNetworkAccessManager>

#include <map>

#include "TopPanel.H"
#include "Camera.H"
#include "Serial.H"
#include "Parameters.H"
#include "Console.H"
#include "CamControls.H"
#include "CfgStack.H"
#include "VideoMapping.H"
#include "Editor.H"
#include "System.H"

class QToolBar;
class QMenu;
class QNetworkReply;

class JeVoisInventor : public QMainWindow
{
    Q_OBJECT

  public:
    explicit JeVoisInventor(QWidget *parent = 0);
    ~JeVoisInventor();

    // Close camera and serial, invalidates m_camok and m_serok
    void disconnect();

    // Try to open camera and serial, sets m_camok and m_serok
    void tryconnect();

    // Enable the UI when camera and serial ok, or disable it
    void enableUI(bool en);
			  
    // called by camera on each new frame
    void newCameraFrame();

  public slots:
    void updateAfterSetMapping();
    void serialError(QSerialPort::SerialPortError error);
    void serialDisconnect();
    void cameraDisconnect();
    void rebootJeVois();
		       
  private slots:
    void tabselected();

    // Create a new module
    void newModule();

  private:
    bool m_camok;
    bool m_serok;
    QTimer m_conntimer;
    //QToolBar m_toolbar;
    QWidget m_central;
    QVBoxLayout m_vlayout;
    TopPanel m_toppanel;
    QSplitter m_splitter;
    QTabWidget m_tab;
    Camera m_camera;
    Serial m_serial;
    QTextBrowser m_modinfo;
    Parameters m_params;
    Console m_console;
    CamControls m_camcontrols;
    CfgStack m_cfg;
    Editor m_src;
    System m_system;
    QNetworkAccessManager m_netmgr;
    bool m_setMappingInProgress;
    
    QMenu * m_filemenu;
    QMenu * m_modmenu;

    QList<VideoMapping> m_vm;
    VideoMapping m_currmapping;
    
    // Data used for highlighting:
    std::map<QString /* command */, QString /* description */> m_cmd;
    std::map<QString /* command */, QString /* description */> m_modcmd;
    QStringList m_ctrlnames;
    QStringList m_paramnames;

    void buildModMenu(QStringList const & mappings);
    void setMapping(int num);
    void setMapping(VideoMapping const & v);
    void infoUpdate(QStringList const & info);
    void modInfoUpdate(QStringList const & mi);
    void modIconUpdate(QByteArray const & data);

    void newModuleError(QStringList const & err);
  public:
    void newModule1(QString const & dir, QString const & filename, QByteArray const & filedata,
		    QByteArray const & pidata, QByteArray const & icondata, QByteArray const & modinfodata);
  private:
    void newModule2(QString const & dir, QString const & filename, QByteArray const & filedata,
		    QByteArray const & pidata, QByteArray const & icondata, QByteArray const & modinfodata);
    void newModule3(QString const & dir, QByteArray const & pidata, QByteArray const & icondata,
		    QByteArray const & modinfodata);
    void newModule4(QString const & dir, QByteArray const & icondata, QByteArray const & modinfodata);
    void newModule5(QString const & dir, QByteArray const & modinfodata);
    void newModuleEnd();

    void stopCamera();
    void startCamera();

    void updateCmdInfo(QStringList const & ci, std::map<QString /* command */, QString /* description */> & cmap);
    void updateCamControls(QStringList const & ci);
    void updateParamInfo(QStringList const & ci);

    void versionCheck(QNetworkReply * reply);

    bool proceedDiscardAnyEdits();
    
    int m_jvmajor, m_jvminor, m_jvpatch; // Version of JeVois running on the camera
};
