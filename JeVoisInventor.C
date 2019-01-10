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

#include "JeVoisInventor.H"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QDesktopServices>
#include <QMessageBox>
#include <QApplication>
#include <QWizard>
#include <QLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QStandardPaths>
#include <QFileDialog>
#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QSettings>

#include "Utils.H"
#include "PythonEdit.H"
#include "CxxEdit.H"
#include "ParamInfo.H"
#include "PreferencesDialog.H"

#include <thread>
#include <cmath> // std::abs is ambiguous on macOS?

namespace
{
  QString const defmap { "OUT: YUYV 640x300 @ 60fps CAM: YUYV 320x240 @ 60fps MOD: JeVois:DemoSaliency C++" };
  QString const defcode { "# Source code not available" };
}

// ##############################################################################################################
JeVoisInventor::JeVoisInventor(QWidget * parent) :
    QMainWindow(parent),
    m_camok(false),
    m_serok(false),
    m_conntimer(this),
    m_central(this),
    m_vlayout(),
    m_toppanel(),
    m_splitter(),
    m_tab(),
    m_camera(this),
    m_serial(),
    m_modinfo(),
    m_params(&m_serial),
    m_console(&m_serial),
    m_camcontrols(&m_serial),
    m_cfg(&m_serial),
    m_src(&m_serial, "boo", false, defcode, new CxxEdit(&m_serial), Editor::SaveAction::Reload, true),
    m_system(this, &m_serial, &m_camera, QSettings().value(SETTINGS_HEADLESS, false).toBool()),
    m_netmgr(),
    m_setMappingInProgress(false),
    m_filemenu(nullptr),
    m_modmenu(nullptr),
    m_vm(),
    m_currmapping(QSettings().value(SETTINGS_DEFMAPPING, defmap).toString()),
    m_jvmajor(1),
    m_jvminor(0),
    m_jvpatch(0),
    m_fpsn(0),
    m_fpsstart(std::chrono::high_resolution_clock::now())
{
  DEBU("Start...");
  // Create a toolbar:
  //m_toolbar = new QToolBar(this);
  
  // put some buttons and stuff, then:
  //addToolBar(m_toolbar);

  // Setup our timer that will be in charge of detecting the camera and serial:
  m_conntimer.setSingleShot(true);
  connect(&m_conntimer, &QTimer::timeout, this, &JeVoisInventor::tryconnect);
  
  // Setup our various widgets:
  m_modinfo.setReadOnly(true);
  m_modinfo.setOpenLinks(false);
  connect(&m_modinfo, &QTextBrowser::anchorClicked,
          [](QUrl const & url) {
            DEBU("clicked ["<<url<<']');
            QString ustr = url.url();
            if (ustr.startsWith("http") == false) QDesktopServices::openUrl("http://jevois.org" + ustr);
            else QDesktopServices::openUrl(url);
          });
  
  // Create the tabed pages:
  m_tab.addTab(&m_modinfo, tr("Info"));
  m_tab.addTab(&m_params, tr("Parameters"));
  m_tab.addTab(&m_console, tr("Console"));
  m_tab.addTab(&m_camcontrols, tr("Camera"));
  m_tab.addTab(&m_cfg, tr("Config"));
  m_tab.addTab(&m_src, tr("Code"));
  m_tab.addTab(&m_system, tr("System"));
  
  connect(&m_tab, SIGNAL(currentChanged(int)), this, SLOT(tabselected()));
  
  // Create the side-by-side tab and camera:
  m_splitter.setHandleWidth(5);
  m_splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_splitter.addWidget(&m_tab);
  m_splitter.addWidget(&m_camera);
  m_splitter.setSizes({ 20000, 80000 });
  
  m_vlayout.setSpacing(0); m_vlayout.setMargin(0);

  m_vlayout.addWidget(&m_toppanel);
  m_vlayout.addWidget(&m_splitter);

  statusBar()->addPermanentWidget(&m_serial);
  statusBar()->showMessage("JeVois Inventor " JVINV_VERSION_STRING);
  
  setCentralWidget(&m_central);
  m_central.setLayout(&m_vlayout);
  
  // Setup the menus:
  m_filemenu = menuBar()->addMenu(tr("&File"));
  
  m_filemenu->addAction(tr("&About"), [this]() {
      QMessageBox::about(this, tr("About JeVois Inventor"),
                         "<b>JeVois Inventor " JVINV_VERSION_STRING "</b><br> <br>Copyright (C) 2018 by Laurent Itti, "
                         "the University of Southern California (USC), and iLab at USC.<br> <br>See "
                         "<a href=http://jevois.org>http://jevois.org</a> for information about this project."); });

  m_filemenu->addAction(tr("&Preferences"), [this]() { editPreferences(); });

  m_filemenu->addSeparator();
  
  m_filemenu->addAction(tr("JeVois &Home Page"),
                        [](){ QDesktopServices::openUrl(QUrl("http://jevois.org")); });
  m_filemenu->addAction(tr("JeVois &Inventor Documentation"),
                        [](){ QDesktopServices::openUrl(QUrl("http://jevois.org/doc/JeVoisInventor.html")); });
  m_filemenu->addAction(tr("JeVois &Guided Tour"),
                        [](){ QDesktopServices::
                            openUrl(QUrl("http://jevois.org/data/jevois-guided-tour-latest.pdf")); });
  m_filemenu->addAction(tr("JeVois &Modules Page"),
                        [](){ QDesktopServices::openUrl(QUrl("http://jevois.org/doc/UserDemos.html")); });
  m_filemenu->addAction(tr("JeVois &Documentation"),
                        [](){ QDesktopServices::openUrl(QUrl("http://jevois.org/doc/index.html")); });
  m_filemenu->addAction(tr("JeVois &User Tutorials"),
                        [](){ QDesktopServices::
                            openUrl(QUrl("http://jevois.org/tutorials/UserTutorials.html")); });
  m_filemenu->addAction(tr("JeVois &Programmer Tutorials"),
                        [](){ QDesktopServices::
                            openUrl(QUrl("http://jevois.org/tutorials/ProgrammerTutorials.html")); });
  
  m_filemenu->addSeparator();
  
  m_filemenu->addAction(tr("&Quit"), [this]()
			{ if (proceedDiscardAnyEdits()) { this->enableUI(false); this->close(); } });
  m_filemenu->actions().back()->setShortcuts(QKeySequence::Quit);
  
  // Start disabled, tryconnect() will enable:
  enableUI(false);
  
  // Show welcome message for this beta:
  QMessageBox::
    about(this, tr("Welcome to JeVois Inventor Beta!"),
          "Welcome to JeVois Inventor beta " JVINV_VERSION_STRING "<br> <br>Use the menubar to select and create "
          "machine vision modules. <br> <br>Please help us improve this software by reporting bugs to "
          "<a href=\"mailto:jevois.org@gmail.com\">jevois.org@gmail.com</a><br> <br>"
          "Current limitations:"
          "<ul><li>Can only display YUYV video</li>"
          "<li>Can only edit/create Python code, C++ editing and compiling not yet supported.</li>"
          "<li>No documentation parser yet, newly created modules will not have a proper documentation page.</li>"
          "</ul>"
#ifdef Q_OS_MACOS
          "<br>Known bugs and issues on MacOS:<br>"
          "<ul><li>Display of parameter list may become garbled after repeated fast up/down scrolling.</li>"
          "<li>Switching to a module with same resolution as current one but different frames/s is buggy.</li>"
          "</ul>"
#endif
          );
  
#ifdef Q_OS_LINUX
  // On Linux, nuke ModemManager if it is running:
  DEBU("Checking for ModemManager...");
  QProcess pscmd;
  pscmd.start("ps -C ModemManager");
  if (pscmd.waitForFinished() != 0) DEBU("Error running ps!");
  QString tlo = pscmd.readAllStandardOutput();
  if (tlo.contains("ModemManager"))
  {
    QMessageBox::information(this, tr("ModemManager detected"),
                             tr("Your computer is running the program ModemManager, which interferes with JeVois.\n\n"
                                "Enter your administrator password in the next prompt to stop ModemManager\n\n"
                                "To avoid this question in the future, type this in a terminal:\n\n") +
                             "sudo apt purge modemmanager",
                             QMessageBox::Ok);
    QString const cmd = "pkexec --disable-internal-agent killall ModemManager";
    if (system(cmd.toLatin1().data()) != 0)
    {
      QMessageBox::information(this, tr("ModemManager error"),
                               tr("We could not stop ModemManager.\n\n"
                                  "Please run this in a terminal window:\n\n") +
                               "sudo killall ModemManager", QMessageBox::Ok);
    }
    QMessageBox::information(this, tr("Restart after killing ModemManager"),
                             tr("ModemManager may have put your JeVois camera in an unknown state.\n\n"
                                "Please restart your JeVois camera now (disconnect it and plug it back in)."),
                             QMessageBox::Ok);
  }
#endif
  
  // Final inits will be handled in tryconnect()
  m_conntimer.start(0);
  
  DEBU("Ready");
  
  // Let editors request reboot:
  connect(&m_src, &Editor::askReboot, this, &JeVoisInventor::rebootJeVois);
  connect(&m_cfg, &CfgStack::askReboot, this, &JeVoisInventor::rebootJeVois);
  
  // Get the latest software version information (actual request will be sent after we get camera jevois version):
  connect(&m_netmgr, &QNetworkAccessManager::finished, this, &JeVoisInventor::versionCheck);
}

// ##############################################################################################################
void JeVoisInventor::versionCheck(QNetworkReply * reply)
{
  // Just ignore network failures:
  if (reply->error() != QNetworkReply::NoError) return;
  
  // Parse the version file:
  QString const ver = QString(reply->readAll());
  QStringList const vl = ver.split('\n');
  for (QString const & s : vl)
  {
    QStringList const vec = s.split(' ');
    if (vec.size() > 1)
    {
      QString const soft = vec[0];
      QStringList const vv = vec[1].split('.');
      if (vv.size() == 3)
      {
        int const major = vv[0].toInt();
        int const minor = vv[1].toInt();
        int const patch = vv[2].toInt();
        
        if (soft == "jevois" &&
            (m_jvmajor < major ||
             (m_jvmajor == major && m_jvminor < minor) ||
             (m_jvmajor == major && m_jvminor == minor && m_jvpatch < patch)))
          QMessageBox::information(this, tr("New version available"),
                                   tr("A new JeVois microSD image, version ") + vec[1] +
                                   tr(", is available.<br> <br>Your camera is now running JeVois ") +
                                   QString::number(m_jvmajor) + '.' + QString::number(m_jvminor) + '.' +
                                   QString::number(m_jvpatch) +
                                   tr("<br> <br>Please download the new image from "
                                      "<a href=\"http:/jevois.org/start\">http://jevois.org/start</a>"),
                                   QMessageBox::Ok);
        if (soft == "jevois-inventor" &&
            (JVINV_VERSION_MAJOR < major ||
             (JVINV_VERSION_MAJOR == major && JVINV_VERSION_MINOR < minor) ||
             (JVINV_VERSION_MAJOR == major && JVINV_VERSION_MINOR == minor && JVINV_VERSION_PATCH < patch)))
          QMessageBox::information(this, tr("New version available"),
                                   tr("A new version of JeVois Inventor (") + vec[1] +
                                   tr(") is available.<br> <br>Please download it from "
                                      "<a href=\"http:/jevois.org/start\">http://jevois.org/start</a>"),
                                   QMessageBox::Ok);
      }
    }
  }
}

// ##############################################################################################################
void JeVoisInventor::tryconnect()
{
#ifdef Q_OS_WIN
  static int nretry = 0;
#endif
  
  // Get the camera going:
  m_camok = m_camera.detect();
  if (m_camok == false)
  {
    DEBU("Camera not available...");
    enableUI(false);
    statusBar()->showMessage(" JeVois Inventor " JVINV_VERSION_STRING + tr(" waiting for JeVois Smart Camera"));
    m_conntimer.start(1000);
    return;
  }
  
  // Get the serial going:
  m_serok = m_serial.detect();
  if (m_serok == false)
  {
    DEBU("Serial not available...");
    enableUI(false);
    statusBar()->showMessage(" JeVois Inventor " JVINV_VERSION_STRING +
                             tr(", camera device ok, waiting for serial-over-USB"));
#ifdef Q_OS_WIN
    ++ nretry;
    if (nretry == 5)
      QMessageBox::information(this, tr("JeVois Serial-over-USB not detected"),
				   tr("Your JeVois Smart Camera was detected, but we could not access "
				      "its Serial-over-USB port. You may need to install a driver for it.<br> <br>"
				      "Please follow the instructions at "
				      "<a href=\"http://jevois.org/doc/USBserialWindows.html\">"
				      "http://jevois.org/doc/USBserialWindows.html</a>"),
			       QMessageBox::Ok);
#endif
    m_conntimer.start(1000);
    return;
  }
  
  // Finalize our setup:
  m_serial.setCamDev(m_camera.deviceName());
  connect(&m_serial, &Serial::writeError, this, &JeVoisInventor::serialDisconnect);
  connect(m_serial.port().data(), &QSerialPort::errorOccurred, this, &JeVoisInventor::serialError);
  connect(m_serial.port().data(), &QSerialPort::aboutToClose, this, &JeVoisInventor::serialDisconnect);
  connect(&m_camera, SIGNAL(error()), this, SLOT(cameraDisconnect()));

  // Get the info as the first thing so we can reject camera software that is too old::
  m_serial.command("info", [this](QStringList const & info) { infoUpdate(info); });
  
  // Build the module menu. This will call setMapping() and get us rolling with the saliency demo:
  m_serial.command("listmappings", [this](QStringList const & mappings) { buildModMenu(mappings); });

  // Get all the commands supported by the core engine:
  m_serial.command("cmdinfo all", [this](QStringList const & ci) { updateCmdInfo(ci, m_cmd); });

  // Get all the camera controls:
  m_serial.command("caminfo", [this](QStringList const & ci) { updateCamControls(ci); });
}

// ##############################################################################################################
void JeVoisInventor::serialDisconnect()
{
  DEBU("Received serial about to close message");

  // Just calling disconnect() here crashes sometimes on mac, maybe because we are in the error callback. So let's push
  // a disconnect event instead:
  QTimer::singleShot(0, this, &JeVoisInventor::disconnect);
}

// ##############################################################################################################
void JeVoisInventor::cameraDisconnect()
{
  DEBU("Received camera error message");

  // Just calling disconnect() here crashes sometimes on mac, maybe because we are in the error callback. So let's push
  // a disconnect event instead:
  QTimer::singleShot(0, this, &JeVoisInventor::disconnect);
}

// ##############################################################################################################
void JeVoisInventor::disconnect()
{
  enableUI(false);
  
  m_serial.setCamDev("-");

  m_camera.closedown();
  m_camok = false;

  m_serial.closedown();
  m_serok = false;

  m_conntimer.start(1000);
}

// ##############################################################################################################
void JeVoisInventor::enableUI(bool en)
{
  m_tab.setEnabled(en);
  if (m_modmenu)
  {
    if (m_system.isHeadless()) m_modmenu->setEnabled(false);
    else m_modmenu->setEnabled(en);
  }
  
  if (en == false)
  {
    // Go to a safe config:
    m_tab.setCurrentWidget(&m_modinfo);
    m_toppanel.reset();
    m_modinfo.setText(tr(JVINV_WAIT_MESSAGE));
    statusBar()->showMessage(" JeVois Inventor " JVINV_VERSION_STRING + tr(" waiting for JeVois Smart Camera"));
    m_camera.showVideo(false);
  }

  update();
}

// ##############################################################################################################
JeVoisInventor::~JeVoisInventor()
{ }

// ##############################################################################################################
void JeVoisInventor::tabselected()
{
  auto curr = m_tab.currentWidget();
  
  // Handle un-selection first as it will stop timers, etc:
  if (curr != &m_console) m_console.tabunselected();
  if (curr != &m_camcontrols) m_camcontrols.tabunselected();
  
  // Now handle selection:
  if (curr == &m_camcontrols) m_camcontrols.tabselected();
  else if (curr == &m_cfg) m_cfg.tabselected();
  else if (curr == &m_console) m_console.tabselected();
  else if (curr == &m_params) m_params.tabselected();
  else if (curr == &m_src) m_src.tabselected();
}

// ##############################################################################################################
namespace
{
  class NewModulePage : public QWizardPage
  {
    public:
      NewModulePage(JeVoisInventor * inv, QList<VideoMapping> const & vm, QWidget * parent = nullptr) :
          QWizardPage(parent), m_inv(inv), m_vm(vm)
      {
        setTitle(tr("Module Details"));
        setSubTitle(tr("Please fill all fields. Hover your mouse over each field for some tips about it."));
        
        QGridLayout * lay = new QGridLayout;
        int idx = 0;
        
        // ---------- Module name --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Name:")), idx, 0);
        QLineEdit * nameLineEdit = new QLineEdit;
        nameLineEdit->setValidator(new QRegExpValidator(QRegExp("[A-Z][A-Za-z0-9_]*"), nameLineEdit));
        nameLineEdit->setPlaceholderText("MyModule");
        nameLineEdit->
          setToolTip(splitToolTip(tr("Module name must start with an uppercase letter, then any combination of "
                                     "letters, numbers, or the underscore symbol. It is the name of the Python "
                                     "or C++ class that will be created for the module.")));
        lay->addWidget(nameLineEdit, idx, 1);
        registerField("name*", nameLineEdit);
        ++idx;
        
        // ---------- Module vendor --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Vendor:")), idx, 0);
        QLineEdit * vendorLineEdit = new QLineEdit;
        vendorLineEdit->setValidator(new QRegExpValidator(QRegExp("[A-Z][A-Za-z0-9_]*"), vendorLineEdit));
        vendorLineEdit->setPlaceholderText("ExampleVendor");
        vendorLineEdit->
          setToolTip(splitToolTip(tr("Module vendor must start with an uppercase letter, then any "
                                     "combination of letters, numbers, or the underscore symbol. It is "
                                     "the name of the parent directory that will be created for your "
                                     "module. It is used to help organize modules by their creators.")));
        lay->addWidget(vendorLineEdit, idx, 1);
        registerField("vendor*", vendorLineEdit);
        ++idx;
        
        // ---------- Module synopsis --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Synopsis:")), idx, 0);
        QLineEdit * synoLineEdit = new QLineEdit;
        synoLineEdit->setPlaceholderText("Detect Objects of type X");
        synoLineEdit->
          setToolTip(splitToolTip(tr("A short one-liner description of what the module does.")));
        lay->addWidget(synoLineEdit, idx, 1);
        registerField("syno*", synoLineEdit);
        ++idx;
        
        // ---------- Module author --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Author:")), idx, 0);
        QLineEdit * authorLineEdit = new QLineEdit;
        authorLineEdit->setPlaceholderText("John Smith");
        authorLineEdit->
          setToolTip(splitToolTip(tr("Your name. Used by the module title page of JeVois Inventor.")));
        lay->addWidget(authorLineEdit, idx, 1);
        registerField("author", authorLineEdit);
        ++idx;
        
        // ---------- Module email --------------------------------------------------
        lay->addWidget(new QLabel(tr("Author Email:")), idx, 0);
        QLineEdit * emailLineEdit = new QLineEdit;
        emailLineEdit->setPlaceholderText("you@yourcompany.com");
        emailLineEdit->
          setToolTip(splitToolTip(tr("Your email address. Used by the module title page of JeVois Inventor.")));
        QRegularExpression rx("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b",
                              QRegularExpression::CaseInsensitiveOption);
        emailLineEdit->setValidator(new QRegularExpressionValidator(rx, this));
        lay->addWidget(emailLineEdit, idx, 1);
        registerField("email", emailLineEdit);
        ++idx;
        
        // ---------- Module website --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Website:")), idx, 0);
        QLineEdit * websiteLineEdit = new QLineEdit;
        websiteLineEdit->setPlaceholderText("http://yourcompany.com");
        websiteLineEdit->
          setToolTip(splitToolTip(tr("Your website address. Used by the module title page of JeVois Inventor.")));
        // FIXME need a validator
        lay->addWidget(websiteLineEdit, idx, 1);
        registerField("website", websiteLineEdit);
        ++idx;
        
        // ---------- Module license --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module License:")), idx, 0);
        QLineEdit * licenseLineEdit = new QLineEdit;
        licenseLineEdit->setPlaceholderText("GPL v3");
        licenseLineEdit->
          setToolTip(splitToolTip(tr("License terms for your module. Used by the module title "
                                     "page of JeVois Inventor.")));
        // FIXME need a validator
        lay->addWidget(licenseLineEdit, idx, 1);
        registerField("license", licenseLineEdit);
        ++idx;
        
        // ---------- Module icon --------------------------------------------------
        lay->addWidget(new QLabel(tr("Module Icon:")), idx, 0);
        QPushButton * iconButton = new QPushButton(tr("Change Icon"));
        iconButton->setToolTip(splitToolTip(tr("Icon should be a 128x128 square image.")));
        connect(iconButton, &QPushButton::clicked, [&]() {
            QStringList locs = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
            QString loc = locs.isEmpty() ? "" : locs[0];
            
            QString const fname = QFileDialog::getOpenFileName
              (this, "Select icon file to load", loc,
               tr("Image Files (*.png *.jpg *.jpeg *.bmp * .tif)"));
            
            m_icon.load(fname);
            setPixmap(QWizard::LogoPixmap, m_icon.scaled(64, 64, Qt::KeepAspectRatio));
          });
        
        lay->addWidget(iconButton, idx, 1);
        ++idx;
        
        // ---------- Output format --------------------------------------------------
        QGroupBox * ofmtGB = new QGroupBox(tr("USB output video format"));
        QHBoxLayout  * ohlay = new QHBoxLayout;
        
        QComboBox * ofmtBox = new QComboBox;
        ofmtBox->addItem("YUYV");
        ofmtBox->addItem("GREY");
        ofmtBox->addItem("MJPG");
        ofmtBox->addItem("BAYER");
        ofmtBox->addItem("RGB565");
        ofmtBox->addItem("BGR24");
        ofmtBox->addItem("NONE");
        ofmtBox->setCurrentIndex(0);
        ofmtBox->setEnabled(false);
        ofmtBox->
          setToolTip(splitToolTip(tr("Although JeVois supports many different output pixel formats, for now "
                                     "only YUYV is supported by JeVois Inventor.")));
        registerField("ofmt", ofmtBox, "currentText", "currentTextChanged");
        ohlay->addWidget(ofmtBox);
        
        QLineEdit * owLineEdit = new QLineEdit;
        owLineEdit->setValidator(new QIntValidator(8, 4096, owLineEdit));
        owLineEdit->
          setToolTip(splitToolTip(tr("Width in pixels of the video that will be output by JeVois and sent to the "
                                     "host computer over USB. Valid values are integers from 8 to 4096. Beware "
                                     "that values which are not multiple of 8 may not work with many video "
                                     "capture software, including JeVois Inventor on some Mac.")));
        owLineEdit->setPlaceholderText("320");
        registerField("ow*", owLineEdit);
        ohlay->addWidget(owLineEdit);
        
        ohlay->addWidget(new QLabel("x"));
        
        QLineEdit * ohLineEdit = new QLineEdit;
        ohLineEdit->setValidator(new QIntValidator(8, 4096, ohLineEdit));
        ohLineEdit->
          setToolTip(splitToolTip(tr("Height in pixels of the video that will be output by JeVois and sent to the "
                                     "host computer over USB. Valid values are integers from 8 to 4096.")));
        ohLineEdit->setPlaceholderText("240");
        registerField("oh*", ohLineEdit);
        ohlay->addWidget(ohLineEdit);
        
        ohlay->addWidget(new QLabel("@"));
        
        QLineEdit * ofLineEdit = new QLineEdit;
        ofLineEdit->setValidator(new QDoubleValidator(0.1, 250.0, 1, ofLineEdit));
        ofLineEdit->setPlaceholderText(QLocale::system().toString(30.0, 'f', 1));
        ofLineEdit->setFixedWidth(60);
        ofLineEdit->
          setToolTip(splitToolTip(tr("Framerate for the video sent to the host computer over USB. In most cases, "
                                     "this should match the camera sensor framerate chosen below.")));
        registerField("of*", ofLineEdit);
        ohlay->addWidget(ofLineEdit);
        
        ohlay->addWidget(new QLabel("fps"));
        
        ofmtGB->setLayout(ohlay);
        lay->addWidget(ofmtGB, idx, 0, 1, 2);
        ++idx;
        
        // ---------- Camera format --------------------------------------------------
        QGroupBox * cfmtGB = new QGroupBox(tr("Camera sensor video format"));
        QHBoxLayout  * chlay = new QHBoxLayout;
        
        QComboBox * cfmtBox = new QComboBox;
        cfmtBox->addItem("YUYV");
        cfmtBox->addItem("BAYER");
        cfmtBox->addItem("RGB565");
        cfmtBox->setCurrentIndex(0);
        cfmtBox->
          setToolTip(splitToolTip(tr("Pixel format for images captured by the camera sensor. YUYV is the most "
                                     "common choice.")));
        registerField("cfmt", cfmtBox, "currentText", "currentTextChanged");
        chlay->addWidget(cfmtBox);
        
        QComboBox * cresBox = new QComboBox;
        cresBox->addItem("1280 x 1024");
        cresBox->addItem("640 x 480");
        cresBox->addItem("352 x 288");
        cresBox->addItem("320 x 240");
        cresBox->addItem("176 x 144");
        cresBox->addItem("160 x 120");
        cresBox->addItem("88 x 72");
        cresBox->setCurrentIndex(3);
        cresBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        cresBox->
          setToolTip(splitToolTip(tr("Video resolution captured by the camera sensor.")));
        registerField("cres", cresBox, "currentText", "currentTextChanged");
        chlay->addWidget(cresBox);
        
        chlay->addWidget(new QLabel("@"));
        
        QLineEdit * cfLineEdit = new QLineEdit;
        cfLineEdit->setPlaceholderText(QLocale::system().toString(30.0, 'f', 1));
        cfLineEdit->setValidator(new QDoubleValidator(0.1, 60.0, 1, cfLineEdit)); // default validator at 320x240
        cfLineEdit->setFixedWidth(60);
        cfLineEdit->
          setToolTip(splitToolTip(tr("Framerate for the camera sensor. Maximum allowed frame rates for each "
                                     "camera sensor resolution are: 15fps for 1280x1024, 30fps for 640x480, "
                                     "60fps for 352x288, 320x240, and 160x120, and 120fps for 176x144 "
                                     "and 88x72.")));
        registerField("cf*", cfLineEdit);
        chlay->addWidget(cfLineEdit);
        
        connect(cresBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [cfLineEdit](int idx)
                {
                  switch (idx)
                  {
                  case 0: cfLineEdit->setValidator(new QDoubleValidator(0.1, 15.0, 1, cfLineEdit)); break;
                  case 1: cfLineEdit->setValidator(new QDoubleValidator(0.1, 30.0, 1, cfLineEdit)); break;
                  case 2: cfLineEdit->setValidator(new QDoubleValidator(0.1, 60.0, 1, cfLineEdit)); break;
                  case 3: cfLineEdit->setValidator(new QDoubleValidator(0.1, 60.0, 1, cfLineEdit)); break;
                  case 4: cfLineEdit->setValidator(new QDoubleValidator(0.1, 120.0, 1, cfLineEdit)); break;
                  case 5: cfLineEdit->setValidator(new QDoubleValidator(0.1, 60.0, 1, cfLineEdit)); break;
                  case 6: cfLineEdit->setValidator(new QDoubleValidator(0.1, 120.0, 1, cfLineEdit)); break;
                  default: break;
                  }
                  
                  // Need to clear the fps as just changing the validator will not update the wizard complete status:
                  cfLineEdit->clear();
                } );
        
        chlay->addWidget(new QLabel("fps"));
        
        cfmtGB->setLayout(chlay);
        lay->addWidget(cfmtGB, idx, 0, 1, 2);
        ++idx;
        
        // ---------- Module language --------------------------------------------------
        lay->addWidget(new QLabel("Language:"), idx, 0);
        QComboBox * langBox = new QComboBox;
        langBox->addItem("Python");
        langBox->addItem("C++");
        connect(langBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
            if (m_icon.isNull())
              switch (index)
              {
              case 1: setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/cppicon.png").scaled(64, 64)); break;
              default: setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/pyicon.png").scaled(64, 64)); break;
              }
          });
        langBox->setCurrentIndex(0);
        setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/pyicon.png").scaled(64, 64));
        langBox->setEnabled(false);
        langBox->
          setToolTip(splitToolTip(tr("JeVois modules can be programmed in C++ or Python, but only Python is "
                                     "currently supported by JeVois Inventor")));
        lay->addWidget(langBox, idx, 1);
        registerField("lang", langBox, "currentText", "currentTextChanged");
        ++idx;
        
        // ---------- Module template --------------------------------------------------
        lay->addWidget(new QLabel("Template:"), idx, 0);
        QComboBox * modBox = new QComboBox;
        modBox->addItem(tr("Empty OpenCV Module"));
        modBox->addItem(tr("Minimal OpenCV Module"));
        modBox->addItem(tr("Full-featured OpenCV Module"));
        modBox->addItem(tr("Module with Raw Image access"));
        modBox->setCurrentIndex(1);
        modBox->
          setToolTip(splitToolTip(tr("If in doubt, try the Minimal OpenCV module first.")));
        lay->addWidget(modBox, idx, 1);
        registerField("mod", modBox); // register this one as an int since we localize the strings
        ++idx;
        
        setLayout(lay);
      }
      
      virtual ~NewModulePage()
      { }
      
      bool validatePage()
      {
        // Get all the data:
        QString const module = field("name").toString();
        QString const syno = field("syno").toString();
        QString const author = field("author").toString();
        QString const email = field("email").toString();
        QString const website = field("website").toString();
        QString const license = field("license").toString();
        QString const vendor = field("vendor").toString();
        QString const ofmt = field("ofmt").toString();
        unsigned int const ow = field("ow").toUInt();
        unsigned int const oh = field("oh").toUInt();
        float const of = field("of").toFloat();
        QString const cfmt = field("cfmt").toString();
        QString const cres = field("cres").toString();
        QStringList ccc = cres.split(" x ");
        if (ccc.size() != 2) { DEBU("internal error: ccc is " << ccc); return false; }
        unsigned int const cw = ccc[0].toUInt();
        unsigned int const ch = ccc[1].toUInt();
        float const cf = field("cf").toFloat();
        QString const lang = field("lang").toString();
        int const mod = field("mod").toInt(); // this one was registered as an int
        
        // First check for conflicting mappings:
        for (VideoMapping const & m : m_vm)
        {
          if (m.ofmt == ofmt && m.ow == ow && m.oh == oh && fabs(m.ofps - of) < 0.1 &&
              QMessageBox::question(this, tr("Conflicting mapping detected"),
                                    tr("Another video mapping exists which conflicts with yours:<br>&nbsp;<br>") +
                                    m.str() + tr("<br>&nbsp;<br>Frame rates will be adjusted by +/- 1 fps by "
                                                 "JeVois to resolve the conflict.<br>&nbsp;<br>Ignore or go back to "
                                                 "editing your new module's definition?"),
                                    QMessageBox::Cancel | QMessageBox::Ignore,
                                    QMessageBox::Ignore) == QMessageBox::Cancel)
            return false;
        }
        
        // Set module file name; also use a default icon if it was not uploaded by the user:
        QString filename = module;
        
        if (lang == "Python")
        { filename +=  ".py"; if (m_icon.isNull()) m_icon = QPixmap(":/resources/pyicon.png"); }
        else
        { filename +=  ".C"; if (m_icon.isNull()) m_icon = QPixmap(":/resources/cppicon.png"); }
        
        // Prepare a few strings which we will use:
        QString const dir = JEVOIS_MODULE_PATH "/" + vendor + '/' + module;
        
        // The video mapping and postinstall:
        QString const vmstr = ofmt + ' ' + QString::number(ow) + ' ' + QString::number(oh) + ' ' +
          QString::number(of) + ' ' + cfmt + ' ' + QString::number(cw) + ' ' + QString::number(ch) + ' ' +
          QString::number(cf) + ' ' + vendor + ' ' + module;
        
        QString const pidata = "jevois-add-videomapping " + vmstr + '\n';
        
        // The icon as PNG raw data:
        QByteArray icondata;
        if (m_icon.width() > 128 || m_icon.height() > 128 || m_icon.width() != m_icon.height())
          m_icon = m_icon.scaled(128, 128, Qt::KeepAspectRatio);
        QBuffer buffer(&icondata);
        buffer.open(QIODevice::WriteOnly);
        m_icon.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
        
        // The module source code file:
        QString codename;
        switch (mod)
        {
        case 3: codename = ":/resources/raw.py"; break;
        case 2: codename = ":/resources/full.py"; break;
        case 1: codename = ":/resources/mini.py"; break;
        default: codename = ":/resources/nano.py"; break;
        }
        QFile codefile(codename); codefile.open(QIODevice::ReadOnly | QIODevice::Text);
        QString filedata = codefile.readAll();
        
        filedata.replace("@MODULE@", module);
        filedata.replace("@VENDOR@", vendor);
        filedata.replace("@VIDEOMAPPING@", vmstr);
        filedata.replace("@SYNOPSIS@", syno);
        filedata.replace("@AUTHOR@", author);
        filedata.replace("@EMAIL@", email);
        filedata.replace("@WEBSITE@", website);
        filedata.replace("@LICENSE@", license);
        filedata.replace("@LANGUAGE@", lang);
        
        // A stub modinfo.html:
        QFile mifile(":/resources/modinfo.html"); mifile.open(QIODevice::ReadOnly | QIODevice::Text);
        QString midata = mifile.readAll();
        midata.replace("@MODULE@", module);
        midata.replace("@VENDOR@", vendor);
        midata.replace("@SYNOPSIS@", syno);
        midata.replace("@AUTHOR@", author);
        midata.replace("@EMAIL@", email);
        midata.replace("@WEBSITE@", website);
        midata.replace("@LICENSE@", license);
        midata.replace("@LANGUAGE@", lang);
        
        m_inv->newModule1(dir, filename, filedata.toLatin1(), pidata.toLatin1(), icondata, midata.toLatin1());
        
        return true;
      }
      
    private:
      JeVoisInventor * m_inv;
      QList<VideoMapping> const & m_vm;
      QPixmap m_icon;
  };
}

// ##############################################################################################################
void JeVoisInventor::newModuleError(QStringList const & err)
{
  QMessageBox::critical(this, tr("New module creation failed"),
			tr("Failed to create new module with error:<br>&nbsp;<br>") +
			err.join("<br>"), QMessageBox::Ok);
}

// ##############################################################################################################
void JeVoisInventor::newModule1(QString const & dir, QString const & filename, QByteArray const & filedata,
				QByteArray const & pidata, QByteArray const & icondata, QByteArray const & modinfodata)
{
  m_serial.command("shell mkdir -p " + dir,
		   [=](QStringList const &) { newModule2(dir, filename, filedata, pidata, icondata, modinfodata); },
		   [this](QStringList const & err) { newModuleError(err); }
		   );
}

// ##############################################################################################################
void JeVoisInventor::newModule2(QString const & dir, QString const & filename, QByteArray const & filedata,
				QByteArray const & pidata, QByteArray const & icondata, QByteArray const & modinfodata)
{
  m_serial.sendBuffer(dir + '/' + filename, filedata,
		      [=](QStringList const &) { newModule3(dir, pidata, icondata, modinfodata); },
		      [this](QStringList const & err) { newModuleError(err); }
		      );
}

// ##############################################################################################################
void JeVoisInventor::newModule3(QString const & dir, QByteArray const & pidata, QByteArray const & icondata,
				QByteArray const & modinfodata)
{
  m_serial.sendBuffer(dir + "/postinstall", pidata,
		      [=](QStringList const &) { newModule4(dir, icondata, modinfodata); },
		      [this](QStringList const & err) { newModuleError(err); }
		      );
}

// ##############################################################################################################
void JeVoisInventor::newModule4(QString const & dir, QByteArray const & icondata, QByteArray const & modinfodata)
{
  m_serial.sendBuffer(dir + "/icon.png", icondata,
		      [=](QStringList const &) { newModule5(dir, modinfodata); },
		      [this](QStringList const & err) { newModuleError(err); }
		      );
}

// ##############################################################################################################
void JeVoisInventor::newModule5(QString const & dir, QByteArray const & modinfodata)
{
  m_serial.sendBuffer(dir + "/modinfo.html", modinfodata,
		      [&](QStringList const &) { newModuleEnd(); },
		      [this](QStringList const & err) { newModuleError(err); }
		      );
}

// ##############################################################################################################
void JeVoisInventor::newModuleEnd()
{
  if (QMessageBox::question(this, tr("Module successfully created"),
			    tr("New module was successfully created. Restart JeVois so that the host computer can "
			       "now detect the new module?"), QMessageBox::Yes | QMessageBox::No,
			    QMessageBox::Yes) == QMessageBox::Yes)
    m_serial.command("restart");
}

// ##############################################################################################################
void JeVoisInventor::newModule()
{
  QWizard wizard;
  wizard.setWizardStyle(QWizard::ModernStyle); // so we can see our icon
  
  // -------------------- Create intro page
  QWizardPage * page1 = new QWizardPage;
  page1->setTitle(tr("Create new Machine Vision module"));

  QLabel * label1 = new QLabel(tr("Use this wizard to create your new machine vision module."
				  "<br>&nbsp;<br>&nbsp;<br>&nbsp;<br>"
				  "For now, only new modules written in Python are supported."
				  "<br>&nbsp;<br>&nbsp;<br>&nbsp;<br>"
				  "Likewise, for now, only YUYV output pixel format is supported."
				  ));
  label1->setWordWrap(true);
  QVBoxLayout * layout1 = new QVBoxLayout;
  layout1->addWidget(label1);
  page1->setLayout(layout1);
  wizard.addPage(page1);

  // -------------------- Create second page
  QWizardPage * page2 = new NewModulePage(this, m_vm);
  wizard.addPage(page2);

  // -------------------- run the wizard
  wizard.setWindowTitle(tr("New Machine Vision Module"));
  wizard.exec();
}

// ##############################################################################################################
void JeVoisInventor::buildModMenu(QStringList const & mappings)
{
  m_vm.clear();
  
  if (m_modmenu) m_modmenu->clear();
  else m_modmenu = menuBar()->addMenu(tr("&Vision Module"));

  auto newAct = new QAction(tr("&New Python module..."), this);
  newAct->setShortcuts(QKeySequence::New);
  newAct->setStatusTip(tr("Create a new module in Python"));
  connect(newAct, &QAction::triggered, this, &JeVoisInventor::newModule);
  m_modmenu->addAction(newAct);

  m_modmenu->addSeparator();

  bool started = false;
  for (QString const & s : mappings)
  {
    if (s.contains("OUT: NONE")) continue;
    
    QStringList sl = s.split(" - ");

    if (sl.size() != 2) continue; // Ignore OK and such
    
    VideoMapping vm(sl[1]);
    m_vm.push_back(vm);
    int const num = m_vm.size() - 1;

    // Build each menu entry:
    m_modmenu->addAction(m_vm.back().ostr(), [this,num]() { setMapping(num); });

    if (vm.ispython)
    {
      QAction * a = m_modmenu->actions().back();
#ifdef Q_OS_MACOS
      // Setting bold menu font does not work on macos, so instead we request a default font and make it bold:
      //QFont f(".SF NS Text", 13, QFont::Bold); // works but bold is ignored
      QFont f("Helvetica Bold", 14); // works ok
#else
      // Set bold font for python modules
      QFont f = a->font();
      f.setWeight(QFont::Bold);
#endif
      a->setFont(f);
    }
    
    // FIXME: for now, disable modes which do not output YUYV:
    if (vm.ofmt != "YUYV") m_modmenu->actions().back()->setEnabled(false);

    // Grey out the flipped 640x480 JeVoisIntro version as it is confusing:
    if (vm.vendor == "JeVois" && vm.modulename == "JeVoisIntro" && vm.ow == 640 && vm.oh == 480)
      m_modmenu->actions().back()->setEnabled(false);

    // Launch our default module, unless user turned off streaming in the system tab:
    if (m_system.isHeadless() == false &&
        vm.vendor == m_currmapping.vendor &&
        vm.modulename == m_currmapping.modulename &&
        vm.ow == m_currmapping.ow &&
        vm.oh == m_currmapping.oh)
    { setMapping(vm); started = true; }
  }

  // If we did not start our default module per preferences (maybe it was deleted), just start the first available
  // module, unless user turned off streaming:
  if (started == false && m_vm.isEmpty() == false && m_system.isHeadless() == false) setMapping(0);

  // If user disabled streaming, we still want to get some info about the module that is running:
  if (m_system.isHeadless())
  {
    m_setMappingInProgress = true;
    newCameraFrame();
    m_camera.showVideo(false, true);
    m_modmenu->setEnabled(false);
  }
  
  // Enable our menu and our widget:
  enableUI(true);
}

// ##############################################################################################################
void JeVoisInventor::setMapping(int num)
{
  if (proceedDiscardAnyEdits() == false) return;

  DEBU("setMapping " << num);
  if (num >= m_vm.size()) return;
  setMapping(m_vm[num]);
}

// ##############################################################################################################
void JeVoisInventor::setMapping(VideoMapping const & v)
{
  m_setMappingInProgress = true;
  
  // Store the mapping for future use:
  m_currmapping = v;

  // If the camera is active, stop it first, otherwise start it:
  if (m_camera.status() == QCamera::ActiveStatus)
  {
    m_camera.showVideo(false);
    stopCamera(); // updateAfterSetMapping() will be called with a stopping status
  }
  else
    startCamera(); // updateAfterSetMapping() will be called with a starting status
}

// ##############################################################################################################
void JeVoisInventor::stopCamera()
{
  m_camera.stop();
}

// ##############################################################################################################
void JeVoisInventor::startCamera()
{
  // Decode the format:
  QVideoFrame::PixelFormat fmt;
  if (m_currmapping.ofmt == "YUYV") fmt = QVideoFrame::Format_YUYV;
  else if (m_currmapping.ofmt == "RGB565") fmt = QVideoFrame::Format_RGB565;
  else if (m_currmapping.ofmt == "BGR24")  fmt = QVideoFrame::Format_BGR24;
  else if (m_currmapping.ofmt == "MJPG") fmt = QVideoFrame::Format_Jpeg;
  else if (m_currmapping.ofmt == "BAYER") fmt = QVideoFrame::Format_CameraRaw;
  else if (m_currmapping.ofmt == "GREY") fmt = QVideoFrame::Format_Y8;
  else { DEBU("Invalid format " << m_currmapping.ofmt); return; }
  
  // Launch the camera:
  m_camera.start(fmt, m_currmapping.ow, m_currmapping.oh, m_currmapping.ofps);
}

// ##############################################################################################################
void JeVoisInventor::updateAfterSetMapping(QCamera::Status status)
{
  // This gets triggered when users manually stop the video feed from the System tab:
  if (m_setMappingInProgress == false) return;
  
  // If status changed to LoadingStatus (loading for the first time) or StoppingStatus (stopping a previous module),
  // then set the new mapping:
  DEBU("got status " << status);
  
  switch (status)
  {
  case QCamera::UnavailableStatus:
  case QCamera::LoadingStatus:
  case QCamera::StoppingStatus:
    startCamera();
    // Next: The camera will get back to us here with an active status
    break;
    
  case QCamera::ActiveStatus:
    m_fpsn = 0;
    m_fpsstart = std::chrono::high_resolution_clock::now();
    m_camera.requestSignalFrame(true);
    // Next: the camera will call newCameraFrame() when it actually starts streaming
    break;
    
  default: break;
  }
}

// ##############################################################################################################
void JeVoisInventor::newCameraFrame()
{
  if (m_setMappingInProgress == false)
  {
    // Just report fps once in a while:
    ++ m_fpsn;
    if (m_fpsn >= 30)
    {
      std::chrono::duration<double> const dur = std::chrono::high_resolution_clock::now() - m_fpsstart;
      double const secs = dur.count();
      if (secs) statusBar()->showMessage(m_statusstr + " - " + QString::number(30.0/secs, 'g', 4) + " fps");

      m_fpsn = 0;
      m_fpsstart = std::chrono::high_resolution_clock::now();
    }
    
    return;
  }
  
  // This is called by the camera after it starts streaming video; we are now ready to update modinfo, parameters, etc
  // for the new module now that it is loaded and operational:
  m_modinfo.clear();
  m_serial.receiveTextBuffer("modinfo.html", [this](QStringList const & mi) { modInfoUpdate(mi); });
  m_tab.setCurrentWidget(&m_modinfo);
  m_cfg.reset();
  m_src.reset();
  m_src.setFile(m_currmapping.srcpath());

  // Also grab the module commands:
  m_serial.command("modcmdinfo", [this](QStringList const & ci) { updateCmdInfo(ci, m_modcmd); });
	
  // Also grab the module parameters. Keep this the last one. It will update the highlighters:
  m_serial.command("paraminfo", [this](QStringList const & ci) { updateParamInfo(ci); });

  // Our setmapping is now complete:
  m_setMappingInProgress = false;
}

// ##############################################################################################################
void JeVoisInventor::infoUpdate(QStringList const & info)
{
  if (info.size() < 2)
  {
    QMessageBox::critical(this, tr("Communication error"),
                          tr("Cannot obtain JeVois software version\nfrom JeVois Smart Camera\n\nDisconnecting..."));
    disconnect();
    return;
  }

  QStringList sl = info[0].mid(13).split('.');
  if (sl.size() != 3)
  {
    QMessageBox::critical(this, tr("Communication error"),
                          tr("Malformed JeVois software version\nfrom JeVois Smart Camera\n\nDisconnecting..."));
    disconnect();
    return;
  }

  m_jvmajor = sl[0].toInt();
  m_jvminor = sl[1].toInt();
  m_jvpatch = sl[2].toInt();
  DEBU("Camera is running JeVois " << sl);
  
  if (m_jvmajor <= 1 && m_jvminor <= 8 && m_jvpatch <= 0)
  {
    QMessageBox::critical(this, tr("JeVois camera software too old"),
			  tr("Your JeVois camera was detected, but the software version running on your "
			     "camera is too old. JeVois Inventor requires your camera to "
			     "run JeVois software 1.8.1 or higher.<br>&nbsp;<br>"
			     "Please re-flash your microSD card with the latest software image from "
			     "<a href=\"http://jevois.org/start/start.html\">http://jevois.org/start</a>"),
			  QMessageBox::Abort);
    disconnect();
    QApplication::quit();
    return;
  }

  m_statusstr = " JeVois Inventor " JVINV_VERSION_STRING  + tr(" with camera running ") + info[0].mid(6);
  statusBar()->showMessage(m_statusstr);

  // Now is a good time to check software version:
  QNetworkRequest request(QUrl(JEVOIS_VERSION_URL));
  m_netmgr.get(request);
}

// ##############################################################################################################
void JeVoisInventor::modInfoUpdate(QStringList const & mi)
{
  QStringList sl;
  int state = 0;
  
  for (QString const & s : mi)
    switch (state)
    {
    case 0: // Looking for module display name
    {
      QString const str = extractString(s, "<td class=modinfoname>", "</td>");
      if (str.isEmpty() == false) { m_toppanel.m_modname.setText(str); ++state; }
      DEBU("Received modinfo for " << str);
      break;
    }
    
    case 1: // Looking for short synopsis
    {
      QString const str = extractString(s, "<td class=modinfosynopsis>", "</td>");
      if (str.isEmpty() == false) { m_toppanel.m_moddesc.setText(str); ++state; }
      break;
    }
    
    case 2: // Looking for author info
    {
      QString const str = extractString(s, "<table class=modinfoauth width=100%>", "</table>");
      if (str.isEmpty() == false) { m_toppanel.m_modauth.setText("<table width=100%>"+str+"</table>"); ++state; }
      break;
    }
    
    case 3: // Looking for language info
    {
      QString const str = extractString(s, "<table class=moduledata>", "</table>");
      if (str.isEmpty() == false) { m_toppanel.m_modlang.setText("<b><table width=100%>"+str+"</table></b>"); ++state; }
      break;
    }
    
    case 4: // Looking for main doc start
    {
      QString const str = extractString(s, "<td class=modinfodesc>", "");
      if (str.isEmpty() == false) { sl.push_back(str); ++state; }
      break;
    }
    
    case 5: // Extracting main doc until its end marker is encountered
    {
      if (s == "</div></td></tr>") { sl.push_back("</div>"); ++state; }
      else sl.push_back(s);
    }
    
    default:
      break;
  }

  m_modinfo.setHtml(sl.join('\n'));

  // Now also grab the icon:
  m_serial.receiveBinaryBuffer("icon.png", [this](QByteArray const & mi) { modIconUpdate(mi); });
}

// ##############################################################################################################
void JeVoisInventor::modIconUpdate(QByteArray const & data)
{
  QPixmap image;
  image.loadFromData(data);
  m_toppanel.m_icon.setPixmap(image.scaled(64, 64, Qt::KeepAspectRatio));
}

// ##############################################################################################################
void JeVoisInventor::serialError(QSerialPort::SerialPortError error)
{
  Q_UNUSED(error); // unused if debug is off
  DEBU("Received serial error: " << error);

  // Just calling disconnect() here crashes sometimes on mac, maybe because we are in the error callback. So let's push
  // a disconnect event instead:
  QTimer::singleShot(0, this, &JeVoisInventor::disconnect);
}

// ##############################################################################################################
void JeVoisInventor::updateCmdInfo(QStringList const & ci,
                                   std::map<QString /* command */, QString /* description */> & cmap)
{
  cmap.clear();
  
  for (QString const & s : ci)
  {
    // We need: command - description
    if (s.count(" - ") == 0) continue;
    cmap[s.section(" - ", 0, 0)] = s.section(" - ", 1);
  }
  DEBU("Got " << cmap.size() << " JeVois commands");
}

// ##############################################################################################################
void JeVoisInventor::updateCamControls(QStringList const & ci)
{
  m_ctrlnames.clear();
  
  for (auto const & str : ci)
  {
    QStringList vec = str.split(QRegularExpression("\\s+"));
    
    if (vec[0] == "OK") continue;
    
    // Store the name, for highlighting:
    m_ctrlnames.push_back(vec[0]);
  }
}

// ##############################################################################################################
void JeVoisInventor::updateParamInfo(QStringList const & params)
{
  m_paramnames.clear();

  std::map<QString /* category */, std::map<QString /* descriptor */, ParamInfo> > pmap = parseParamInfo(params);

  // Store in our list of params for highlighting:
  for (auto const & pc : pmap)
    for (auto const & pd : pc.second)
      m_paramnames.push_back(pd.second.name);

  // Update the config file editor highlighters:
  m_cfg.setHighlighter(m_cmd, m_modcmd, m_paramnames, m_ctrlnames);

  // We got all the config info we wanted. Now is a good time to update the console tooltip as well:
  m_console.updateCmdInfo(m_cmd, m_modcmd);
}

// ##############################################################################################################
bool JeVoisInventor::proceedDiscardAnyEdits()
{
  QStringList edi;
  m_src.edited(edi);
  m_cfg.edited(edi);

  if (edi.isEmpty()) return true;

  return (QMessageBox::question(this, tr("Discard edited files?"),
				tr("The following files have been modified:\n\n") +
				edi.join('\n') + tr("\n\nDiscard all edits now?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes);
}

// ##############################################################################################################
void JeVoisInventor::rebootJeVois()
{
  // Calling disconnect() after command crashes app on windows 10
  m_serial.command("restart");

  // Do not call enableUI(false) here as it disables the frame timeout. Just a quick disable:
  m_tab.setEnabled(false);
  if (m_modmenu) m_modmenu->setEnabled(false);
}

// ##############################################################################################################
void JeVoisInventor::editPreferences()
{
  PreferencesDialog * pd = new PreferencesDialog(this);
  pd->show();
}
