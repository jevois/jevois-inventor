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

#include <QWidget>
#include <QLabel>
#include <QVideoWidget>
#include <QStackedLayout>
#include <QVideoFrame>
#include <QCamera>
#include <QVideoProbe>
#include <QTimer>
#include <QInputDialog>
#include <QCameraInfo>

class QThread;
class JeVoisInventor;

#define JVINV_WAIT_MESSAGE "Waiting for JeVois Smart Camera\n\nPlease connect JeVois to this computer"
#define JVINV_NOSTREAM_MESSAGE "No video stream from JeVois\n\nRunning in headless mode.\n\nQuit and restart JeVois Inventor to switch back to video streaming mode.\n\n(and check the headless setting in the Preferences menu)"

class Camera : public QWidget
{
    Q_OBJECT
    
  public:
    explicit Camera(JeVoisInventor * inv, QWidget * parent = 0);
    ~Camera();
    
    void showVideo(bool enable, bool suspended = false);
    
    bool detect();
    void closedown();
    
    //! Create a QCamera, connect, and start video streaming
    /*! Relevant formats are:
        QVideoFrame::Format_YUYV
        QVideoFrame::Format_RGB565
        QVideoFrame::Format_RGB24
        QVideoFrame::Format_Jpeg
        QVideoFrame::Format_CameraRaw
        QVideoFrame::Format_Y8 */
    void start(QVideoFrame::PixelFormat fmt, int w, int h, float fps);

    //! Beware that stop() also destroys the QCamera. Use setStreaming() for casual start/stop of streaming
    void stop();

    //! Enble/disable video streaming
    void setStreaming(bool streaming);

    QString const & deviceName() const;

    QCamera::Status status();
    
    void requestSignalFrame(bool enable);

  signals:
    void error();
	   
  public slots:
    void camerror();
    void cameraFrame(QVideoFrame const & frame);
    
  private:
    bool createCamera(QCameraInfo const & caminfo);
    JeVoisInventor * m_inventor;
    QStackedLayout m_layout;
    QLabel m_nocam;
    QLabel m_nostream;
    QVideoWidget m_video;
    QScopedPointer<QCamera> m_camera;
    QVideoProbe m_probe;
    QString m_device;
    QThread * m_thread;
    bool m_signalframe;
    QTimer m_tout;
#ifdef Q_OS_WIN
    QCameraInfo m_caminfo; // remember user selection when manually selecting a device
#endif
};
