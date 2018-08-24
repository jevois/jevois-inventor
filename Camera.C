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

#include "Camera.H"
#include "JeVoisInventor.H"

#include <QCameraInfo>
#include <QScrollArea>
#include <QMessageBox>

#include <QCameraViewfinder>
#include <QCameraViewfinderSettings>
//#include <QCameraImageCapture>
#include <QThread>


// ##############################################################################################################
Camera::Camera(JeVoisInventor * inv, QWidget * parent) :
    QWidget(parent),
    m_inventor(inv),
    m_layout(this),
    m_nocam(tr(JVINV_WAIT_MESSAGE)),
    m_nostream(tr(JVINV_NOSTREAM_MESSAGE)),
    m_video(this),
    m_camera(nullptr),
    m_probe(this),
    m_signalframe(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(640, 480);

  m_nocam.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_nocam.setAlignment(Qt::AlignCenter);
  m_nostream.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_nostream.setAlignment(Qt::AlignCenter);

  m_layout.setMargin(0);
  //m_nocam.setBackgroundRole(QPalette::Shadow);
  //m_nocam.setForegroundRole(QPalette::Light);
  m_layout.addWidget(&m_nocam);

  //m_nostream.setBackgroundRole(QPalette::Shadow);
  //m_nostream.setForegroundRole(QPalette::Light);
  m_layout.addWidget(&m_nostream);

  auto scrollarea = new QScrollArea;
  scrollarea->setWidgetResizable(true);

  m_layout.addWidget(scrollarea);
  scrollarea->setWidget(&m_video);

  connect(&m_probe, SIGNAL(videoFrameProbed(QVideoFrame)), this, SLOT(cameraFrame(QVideoFrame)));
 
  // Show the nocam message by default:
  showVideo(false);

#ifdef Q_OS_WIN
  // Connect our timeout timer so we can detect camera disconnect on windows:
  m_tout.setSingleShot(true);
  connect(&m_tout, &QTimer::timeout, this, &Camera::camerror);
#endif
}

// ##############################################################################################################
Camera::~Camera()
{ }

// ##############################################################################################################
void Camera::showVideo(bool enable, bool suspended)
{
#ifdef Q_OS_WIN
  if (suspended || enable == false) m_tout.stop();
#endif
  
  if (enable) m_layout.setCurrentIndex(2);
  else if (suspended) m_layout.setCurrentIndex(1);
  else m_layout.setCurrentIndex(0);
}

// ##############################################################################################################
QCamera::Status Camera::status()
{
  if (m_camera) return m_camera->status();
  else return QCamera::UnavailableStatus;
}

// ##############################################################################################################
bool Camera::detect()
{
  if (m_camera) { stop(); m_camera.reset(); }

  // Any camera at all?
  size_t numcam = QCameraInfo::availableCameras().count();
  if (numcam == 0) return false;

#ifdef Q_OS_WIN
  QStringList detections;
#endif

  // Get a list of cameras:
  QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
  for (QCameraInfo const & cameraInfo : cameras)
  {
#ifdef Q_OS_WIN
    detections << cameraInfo.description();
#endif
    if (cameraInfo.description().contains("JeVois"))
    {
      if (createCamera(cameraInfo)) return true; else break;
    }
    else DEBU("Skipping camera: " << cameraInfo.description());
  }

#ifdef Q_OS_WIN
  // On Windows, allow user to select a camera. First, check whether we have already done it:
  if (m_caminfo.isNull() == false)
  {
    // We first need to check whether the camera is actually plugged in:
    bool listed = false;
    for (QCameraInfo const & ci : cameras)
      if (ci.description() == m_caminfo.description()) { listed = true; break; }

    if (listed)
    {
      // Got it, create the camera handler:
      if (createCamera(m_caminfo)) return true;
    
      // Ooops, failed to open, nuke our remembered data and fall through:
      m_caminfo = QCameraInfo();
    }
  }
  else
  {
    // No previous selection, let user select from a list:
    bool ok;
    QString item = QInputDialog::
      getItem(this, tr("Select camera device of JeVois"),
	      tr("Sometimes, a camera driver which you previously installed\n"
		 "on your computer is selected by Windows to access JeVois.\n"
		 "\n"
		 "If JeVois has not started yet, click Cancel to wait more.\n"
		 "\n"
		 "Otherwise, select a camera port from the list below.\n"
		 "\n"
		 "If in doubt, first make a note of the entries in the list,\n"
		 "then disconnect JeVois and click Cancel to refresh the list.\n"
		 "Note which device disappeared from the list. Connect JeVois and\n"
		 "wait for Windows to detect it, click Cancel again to refresh.\n"
		 "Finally, select the camera device name assigned to JeVois.\n"),
	      detections, 0, false, &ok);
    if (ok && !item.isEmpty())
    {
      for (QCameraInfo const & cameraInfo : cameras)
	if (item.startsWith(cameraInfo.description()))
	{
	  if (createCamera(cameraInfo))
	  {
	    m_caminfo = cameraInfo;
	    return true;
	  }
	  else break;
	}
    }
  }
#endif
  
  // Switch to showing the nocam message:
  showVideo(false);
  
  return false;
}

// ##############################################################################################################
bool Camera::createCamera(QCameraInfo const & caminfo)
{
  // Instantiate the camera:
  m_camera.reset(new QCamera(caminfo));
  //m_camera->load(); // do not load as it also starts...
      
  // Get its error messages:
  connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error), this, &Camera::camerror);

  // Send a signal when we start grabbing, to update the modinfo:
  connect(m_camera.data(), &QCamera::statusChanged,
	  [this](QCamera::Status status)
	  {
	    Q_UNUSED(status); // unused if debug is off
	    DEBU("Camera status changed to " << status);
	    // Note: a lambda singleshot does not work here, gives rise to threading issues
	    QTimer::singleShot(0, m_inventor, &JeVoisInventor::updateAfterSetMapping);
	  });
  
  // Connect our probe for fps info and detection of stream start:
  m_probe.setSource(m_camera.data());
  
  // Switch to showing the video:
  showVideo(true);
  
  // Remember the device name:
  m_device = caminfo.deviceName();
  
  // Run the camera in its own thread:
  m_thread = new QThread();
  m_camera->moveToThread(m_thread);
  m_thread->start();
  
  return true;
}

// ##############################################################################################################
void Camera::closedown()
{
  showVideo(false);
  if (m_camera) { stop(); m_camera.reset(); }
}

// ##############################################################################################################
void Camera::start(QVideoFrame::PixelFormat fmt, int w, int h, float fps)
{
  if (m_camera.isNull() && detect() == false) return;
  if (m_camera && m_camera->state() != QCamera::UnloadedState) { DEBU("Camera already started"); return; }
  
  m_camera->setViewfinder(&m_video);
  m_camera->setCaptureMode(QCamera::CaptureViewfinder);

  QCameraViewfinderSettings settings = m_camera->viewfinderSettings();
  settings.setPixelFormat(fmt);
  settings.setResolution(QSize(w, h));
  settings.setMaximumFrameRate(fps);
  settings.setMinimumFrameRate(fps);
  m_camera->setViewfinderSettings(settings);

  setMinimumSize(128, 128);
  m_video.setMinimumSize(w, h);

  m_camera->start();  

  m_video.show();
}

// ##############################################################################################################
void Camera::stop()
{
  if (m_camera.isNull()) return;

#ifdef Q_OS_WIN
  m_tout.stop();
#endif

  m_camera->stop();

  if (m_thread)
  {
    m_thread->quit();
    while (m_thread->isRunning()) ; ////qApp->processEvents();
    delete m_thread;
    m_thread = nullptr;
  }
  
  m_camera->unload();
  
  m_camera.reset();
}

// ##############################################################################################################
void Camera::camerror()
{
#ifdef Q_OS_WIN
  m_tout.stop();
#endif

  if (m_camera) DEBU("Camera Error: " << m_camera->errorString());
  //QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
  emit error();
}

// ##############################################################################################################
QString const & Camera::deviceName() const
{
  return m_device;
}

// ##############################################################################################################
void Camera::setStreaming(bool streaming)
{
  if (m_camera)
  {
    if (streaming) { m_camera->start(); showVideo(true); }
    else { m_camera->stop(); showVideo(false, true); }
  }
}

// ##############################################################################################################
void Camera::cameraFrame(QVideoFrame const & frame)
{
  Q_UNUSED(frame);

  // FIXME: compute FPS here
  
  if (m_signalframe) { QTimer::singleShot(0, m_inventor, &JeVoisInventor::newCameraFrame); m_signalframe = false; }

  if (m_camera && m_camera->status() == QCamera::ActiveStatus) m_tout.start(3456);
}

// ##############################################################################################################
void Camera::requestSignalFrame(bool enable)
{
  m_signalframe = enable;
}

