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

#include "Serial.H"
#include "Utils.H"

#include <QSerialPortInfo>
#include <QTextStream>
#include <QLayout>
#include <QFile>
#include <QMessageBox>
#include <QThread>
#include <QInputDialog>

#include <chrono>

// ##############################################################################################################
Serial::Serial(QWidget * parent) :
    QWidget(parent),
    m_portlabel("-"),
    m_camlabel("-"),
    m_statuslabel(tr("Disconnected")),
    m_serial(nullptr)
{
  auto layout = new QHBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);
  layout->addWidget(&m_statuslabel);
  layout->addWidget(new QLabel(" / "));
  layout->addWidget(&m_camlabel);
  layout->addWidget(new QLabel(" / "));
  layout->addWidget(&m_portlabel);
}

// ##############################################################################################################
Serial::~Serial()
{ }

// ##############################################################################################################
void Serial::serialPing()
{
  if (m_serial.data()) m_serial->isRequestToSend(); // will signal an error if was disconnected
}

// ##############################################################################################################
bool Serial::detect()
{
  m_portname = "";
  
  auto const serialPortInfos = QSerialPortInfo::availablePorts();
  const QString blankString = "N/A";

#ifdef Q_OS_WIN
  QStringList detections;
#endif
  
  for (QSerialPortInfo const & serialPortInfo : serialPortInfos)
  {
    QString description = serialPortInfo.description();
    QString manufacturer = serialPortInfo.manufacturer();
    QString serialNumber = serialPortInfo.serialNumber();
    QString portname = serialPortInfo.portName();
    /*
    out << endl
	<< "Port: " << serialPortInfo.portName() << endl
	<< "Location: " << serialPortInfo.systemLocation() << endl
	<< "Description: " << (!description.isEmpty() ? description : blankString) << endl
	<< "Manufacturer: " << (!manufacturer.isEmpty() ? manufacturer : blankString) << endl
	<< "Serial number: " << (!serialNumber.isEmpty() ? serialNumber : blankString) << endl
	<< "Vendor Identifier: " << (serialPortInfo.hasVendorIdentifier()
				     ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16)
				     : blankString) << endl
	<< "Product Identifier: " << (serialPortInfo.hasProductIdentifier()
				      ? QByteArray::number(serialPortInfo.productIdentifier(), 16)
				      : blankString) << endl
	<< "Busy: " << (serialPortInfo.isBusy() ? "Yes" : "No") << endl;		     
    */
    // Any JeVois camera here? On macOS, we want /dev/cu.usbmodemXXX rather than /dev/tty.usbmodemXXX:
#ifdef Q_OS_WIN
    detections << (portname + ": " + manufacturer + ", " + description);
#endif
    
#ifdef Q_OS_MACOS
    if ((description.contains("JeVois") ||
	 manufacturer.contains("JeVois")) &&
	(portname.contains("tty.usbmodem") == false))
#endif
#ifdef Q_OS_WIN
    if (description.contains("JeVois") ||
	manufacturer.contains("JeVois") ||
	description.contains("USB Serial Device")) // for Windows 10 built-in driver
#endif
#ifdef Q_OS_LINUX
    if (description.contains("JeVois") ||
	manufacturer.contains("JeVois"))
#endif
    {
      return createPort(serialPortInfo);
    }
  }

  // We did not detect it. On Mac and Linux, just return now to wait some more. On Windows, allow user to choose. If
  // already chosen, re-use the same choice, if indeed the device is detected again:
#ifdef Q_OS_WIN
  if (m_serinfo.isNull() == false)
  {
    // We first need to check whether the device is actually plugged in:
    bool listed = false;
    for (QSerialPortInfo const & si : serialPortInfos)
      if (si.description() == m_serinfo.description()) { listed = true; break; }

    if (listed)
    {
      // Got it, create the serial driver:
      bool ret = createPort(m_serinfo);
      if (ret) return ret;
      
      // Ooops, failed to open, nuke our remembered data:
      m_serinfo = QSerialPortInfo();
    }
  }
  else if (serialPortInfos.count() > 0)
  {
    bool ok;
    QString item = QInputDialog::
      getItem(this, tr("Select Serial-over-USB port of JeVois"),
	    tr("Sometimes, a USB-Serial driver which you previously installed\n"
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
	       "Finally, select the USB-serial device name assigned to JeVois.\n"),
	      detections, 0, false, &ok);
    if (ok && !item.isEmpty())
    {
      for (QSerialPortInfo const & serialPortInfo : serialPortInfos)
	if (item.startsWith(serialPortInfo.portName()))
	{
	  bool ret = createPort(serialPortInfo);
	  if (ret) m_serinfo = serialPortInfo;
	  return ret;
	}
    }
  }
#endif
  
  return false;
}

// ##############################################################################################################
bool Serial::createPort(QSerialPortInfo const & portinfo)
{
  m_portname = portinfo.portName();
  m_portlabel.setText(m_portname);
      
  // Create and open the port:
  m_serial.reset(new QSerialPort(portinfo));
  if (m_serial->open(QIODevice::ReadWrite) == false)
  {
    DEBU("Serial open error " << m_serial->error());
    
#ifdef Q_OS_LINUX
    // On linux, probably a permission problem since we did detect the port:
    QMessageBox::information(this, tr("JeVois Serial-over-USB access permission error"),
			     tr("The Serial-over-USB port of JeVois was detected as /dev/") +
			     m_portname + tr(" but access was denied.\n\nMake sure no other program is using /dev/") +
			     m_portname + tr(" and enter your administrator password in the next prompt to "
					     "allow access to your JeVois Smart Camera"), QMessageBox::Ok);
    
    QString const cmd = "pkexec --disable-internal-agent chmod 777 /dev/" + m_portname;
    
    if (system(cmd.toLatin1().data()) == 0)
    {
      // Chmod was ok, try to open the port again:
      if (m_serial->open(QIODevice::ReadWrite) == false)
      {
	DEBU("Still could not open serial port even after chmod");
	QMessageBox::information(this, tr("JeVois Serial-over-USB access error"),
				 tr("The Serial-over-USB port of JeVois was detected (") +
				 m_portname + tr(") but could not be accessed by JeVois Inventor.\n\nMake sure "
						 "no other program is using it."), QMessageBox::Ok);
	return false;
      }
    }
    else
    {
      QMessageBox::information(this, tr("JeVois Serial-over-USB access error"),
			       tr("We could not change the access permissions of the Serial-over-USB port of JeVois\n\n"
				  "Please run this in a terminal window:\n\n") +
			       "sudo chmod 777 " + m_portname, QMessageBox::Ok);
      return false;
    }
#endif
  }
  
  m_serial->setReadBufferSize(1024 * 1024);
  m_statuslabel.setText(tr("Connected"));
  connect(m_serial.data(), SIGNAL(readyRead()), this, SLOT(readDataReady()));
  connect(m_serial.data(), SIGNAL(bytesWritten(qint64)), this, SLOT(writeDataDone(qint64)));

  return true;
}


// ##############################################################################################################
void Serial::closedown()
{
  m_portlabel.setText("-");

  // Nuke any pending or in-progress jobs:
  m_wq.clear();
  m_todo.clear();
  m_data.clear();
  
  // Close the port and nuke it:
  if (m_serial)
  {
    m_serial->close();
    m_serial.reset();
  }
}

// ##############################################################################################################
QSharedPointer<QSerialPort> Serial::port() const
{ return m_serial; }

// ##############################################################################################################
void Serial::command(QString const & cmd, std::function<void(QStringList const &)> callback,
		     std::function<void(QStringList const &)> errcallback)
{
  m_wq.push_back( { JobCommand, callback, cmd, QByteArray(), QStringList(), 0, false,
        std::function<void(QByteArray const &)>(), errcallback } );

  DEBU("----------qlen: " << m_wq.size());

  if (m_wq.isEmpty() == false)
    DEBU("First in queue is: " << m_wq.front().cmd);

  writeDataDone(0);
}

// ##############################################################################################################
void Serial::write(QString const & str)
{
  QByteArray arr = str.toLatin1() + '\n';
  qint64 bytes = m_serial->write(arr);
  if (bytes != arr.size()) { DEBU("Only wrote " << bytes << " of " << arr.size() << " bytes");  emit writeError(); }
}

// ##############################################################################################################
QStringList Serial::readAll()
{
  QStringList ret = m_data;
  m_data.clear();
  return ret;
}

// ##############################################################################################################
void Serial::writeDataDone(qint64 bytes)
{
  Q_UNUSED(bytes);
  
  if (m_wq.isEmpty()) return; // All done, no pending commands

  // Run the first job. It will be removed on completion:
  JobData & jd = m_wq.front();
  if (jd.sent == false)
  {
    QString const cmd2 = "JVINV" + jd.cmd;
    this->write(cmd2);

    // For JobFilePut, we need to send all the data right away:
    if (jd.type == JobFilePut)
    {
      write("JEVOIS_FILEPUT " + QString::number(jd.datasize));
      m_serial->write(jd.data);
      jd.datasize = 0;
    }
    
    jd.sent = true;
    DEBU("Sent: " << jd.cmd);
  }
}

// ##############################################################################################################
void Serial::readDataReady()
{
  QByteArray ret = m_serial->readAll();
  ///DEBU("Raw received: " << ret);

  // Prepend any m_todo before we go forward:
  if (m_todo.isEmpty() == false) { ret = m_todo + ret; m_todo.clear(); }
  
  while (ret.isEmpty() == false)
  {
    if (m_wq.isEmpty() == false)
    {
      JobData * jd = &m_wq.front();
      switch (jd->type)
      {
        
      case JobCommand: // --------------------------------------------------
      {
        // Split off command results, if any, from serout/serlog messages:
        if (parseReceived(ret, jd))
        {
          DEBU("Command complete " << jd->cmdret);
	  
	  if (jd->cmdret.isEmpty() == false && jd->cmdret.front().startsWith("ERR "))
	  { if (jd->errcallback) jd->errcallback(jd->cmdret); }
          else if (jd->callback) jd->callback(jd->cmdret);
          m_wq.pop_front();
        }
      }
      break;
      
      case JobFileGet: // --------------------------------------------------
        
        //DEBU("receive datasize=" << jd->datasize << " got=" << jd->data.size());
        
        
        // If we know the datasize, then we are atomically reading the data until it is complete; otherwise, we need to
        // find the header:
        if (jd->datasize == 0)
        {
          int idx = ret.indexOf("JEVOIS_FILEGET ");
          if (idx != -1) // found it
          {
            // If we have some serlog/serout before the header, extract them:
            if (idx > 0) { m_data.append(splitLines(ret.mid(0, idx - 1))); ret = ret.mid(idx); }
            
            // Now, JEVOIS_FILEGET is at the start. Extract until the next \n:
            idx = ret.indexOf('\n');
            if (idx == -1) DEBU("oh maaaaaaaaaaaaaaaaaaaaaaaaan");
            QString lenstr = ret.mid(15, idx - 15);
            DEBU("LENSTR is ["<<lenstr<<']');
            jd->datasize = lenstr.toInt();
            ret = ret.mid(idx + 1);
          }
          else
          {
            // Otherwise, we only got serout/serlog before the file header, or maybe an error
            if (parseReceived(ret, jd))
            {
              DEBU("fileget failed " << jd->cmdret);

	      if (jd->cmdret.isEmpty() == false && jd->cmdret.front().startsWith("ERR "))
	      { if (jd->errcallback) jd->errcallback(jd->cmdret); }
	      else if (jd->callback) jd->callback(jd->cmdret);

              m_wq.pop_front();
	      continue;
            }
          }
        }
        
        if (jd->datasize)
        {
          int need = jd->datasize - jd->data.size();
          DEBU("fileget still need " << need << " bytes; just got " << ret.size());

          // If the size has been set now, then gobble up the file data:
          if (need > ret.size())
          {
            // We need more than we have received; take all we have received and wait for more:
            jd->data += ret;
            ret.clear();
          }
          else
          {
            // We need less or equal than we have received. Get all we need and we will end the transfer when we get
            // JVINVOK:
            jd->data += ret.mid(0, need);
            ret = ret.mid(need);
          }
          
          if (jd->data.size() == jd->datasize)
          {
            // We have received all the data, just need JVINVOK:
            if (parseReceived(ret, jd))
            {
	      DEBU("fileget complete " << jd->cmdret);

	      if (jd->cmdret.isEmpty() == false && jd->cmdret.front().startsWith("ERR "))
	      { if (jd->errcallback) jd->errcallback(jd->cmdret); }
	      else if (jd->callback)
              {
                QString str(jd->data); QStringList sl = splitLines(str);
                //if (sl.back().isEmpty()) sl.pop_back();
                jd->callback(sl);
              }
              else if (jd->bincallback) jd->bincallback(jd->data);

	      m_wq.pop_front();
            }
          }
        }
        break;
        
      case JobFilePut: // --------------------------------------------------
        
        // We have already sent all the data, just need JVINVOK:
        if (parseReceived(ret, jd))
        {
	  DEBU("fileput complete " << jd->cmdret);

	  if (jd->cmdret.isEmpty() == false && jd->cmdret.front().startsWith("ERR "))
	  { if (jd->errcallback) jd->errcallback(jd->cmdret); }
	  else if (jd->callback) jd->callback(jd->cmdret);

          m_wq.pop_front();
        }
        
        break;
        
      case JobNone: // --------------------------------------------------
        break;
        
      default: // --------------------------------------------------
        break;
      }
    }
    else
    {
      DEBU("no job, adding to m_data");
      // If there is no ongoing command that took the data already, just gather serout/serlog data:
      QString str(ret);
      QStringList sl = splitLines(str);
      ret.clear();
      // If last line was not terminated, keep it for later, in m_todo, otherwise, discard the last empty line:
      if (sl.back().isEmpty() == false) m_todo += sl.back();
      sl.pop_back();
      m_data.append(sl);
    }
  }
  
  // If we have received some new serout/serlog, let the console know:
  if (m_data.isEmpty() == false) emit readyRead();
  
  // Try to dequeue the next command:
  writeDataDone(0);
}

// ##############################################################################################################
void Serial::sendBuffer(QString const & fname, QByteArray const & data,
			std::function<void(QStringList const &)> callback,
			std::function<void(QStringList const &)> errcallback)
{
  m_wq.push_back( { JobFilePut, callback, "fileput " + fname, data, QStringList(), data.size(), false,
        std::function<void(QByteArray const &)>(), errcallback } );
  writeDataDone(0);
}

// ##############################################################################################################
void Serial::sendTextFile(QString const & fname, QString const & localfname,
			  std::function<void(QStringList const &)> callback,
			  std::function<void(QStringList const &)> errcallback)
{
  QByteArray data;
  QFile file(localfname);
  if (file.open(QFile::ReadOnly | QFile::Text))
  {
    data = file.readAll();
    sendBuffer(fname, data, callback, errcallback);
  }
  else
  {
    QStringList err; err.push_back("ERR could not open local file " + localfname);
    errcallback(err);
  }
}

// ##############################################################################################################
void Serial::sendBinaryFile(QString const & fname, QString const & localfname,
			    std::function<void(QStringList const &)> callback,
			    std::function<void(QStringList const &)> errcallback)
{
  QByteArray data;
  QFile file(localfname);
  if (file.open(QFile::ReadOnly))
  {
    data = file.readAll();
    sendBuffer(fname, data, callback);
  }
  else
  {
    QStringList err; err.push_back("ERR could not open local file " + localfname);
    errcallback(err);
  }
}

// ##############################################################################################################
void Serial::receiveTextBuffer(QString const & fname,
                               std::function<void(QStringList const &)> callback,
			       std::function<void(QStringList const &)> errcallback)
{
  m_wq.push_back( { JobFileGet, callback, "fileget " + fname, QByteArray(), QStringList(), 0, false,
        std::function<void(QByteArray const &)>(), errcallback } );
  writeDataDone(0);
}

// ##############################################################################################################
void Serial::receiveBinaryBuffer(QString const & fname,
                                 std::function<void(QByteArray const &)> bincallback,
				 std::function<void(QStringList const &)> errcallback)
{
  m_wq.push_back( { JobFileGet, std::function<void(QStringList const &)>(), "fileget " + fname, QByteArray(),
        QStringList(), 0, false, bincallback, errcallback } );
  writeDataDone(0);
}

// ##############################################################################################################
void Serial::setCam(QString const & name, int value,
		    std::function<void(QStringList const &)> callback,
		    std::function<void(QStringList const &)> errcallback)
{
  setCam(name, QString::number(value), callback, errcallback);
}

// ##############################################################################################################
void Serial::setCam(QString const & name, QString const & value,
		    std::function<void(QStringList const &)> callback,
		    std::function<void(QStringList const &)> errcallback)
{
  command("setcam " + name + ' ' + value, callback, errcallback);

  // Slow down the GUI so we don't pile up too many commands...
  QThread::msleep(50);
}

// ##############################################################################################################
void Serial::setPar(QString const & name, QString const & value, std::function<void(QStringList const &)> callback,
		    std::function<void(QStringList const &)> errcallback)

{
  command("setpar " + name + ' ' + value, callback, errcallback);

  // Slow down the GUI so we don't pile up too many commands...
  QThread::msleep(50);
}

// ##############################################################################################################
bool Serial::parseReceived(QByteArray & ret, JobData * jd)
{
  // Split into lines:
  QString str(ret);
  QStringList sl = splitLines(str);
  ret.clear();
  DEBU("PARSING [" << str <<']');

  // If last line was not terminated, keep it for later, in m_todo, otherwise, discard the last empty line:
  if (sl.back().isEmpty() == false) m_todo += sl.back();
  sl.pop_back();
  
  // Separate command outputs starting with JVINV vs serout/serlog messages:
  bool done = false;
  QStringList::iterator itr = sl.begin();
  while (itr != sl.end())
  {
    if (itr->startsWith("JVINV"))
    {
      QString s = itr->mid(5);

      jd->cmdret.push_back(s);

      itr = sl.erase(itr);
      DEBU("s is ["<<s<<']');

      if (s == "OK" || s.startsWith("ERR ")) { done = true; break; }
    }
    else
    {
      // This did not start with JVINV so it is serout/serlog, unless a JVINV string got cut in the middle:
      m_data.push_back(*itr);
      itr = sl.erase(itr);
    }
  }
  
  // Whatever we did not take will be handled later:
  str = sl.join("\n");
  ret = str.toLatin1();
  if (ret.isEmpty() == false) ret += '\n'; // add a line feed to signal this line is complete

  DEBU("REMAINING: [" << ret<<"]   m_todo=["<<m_todo<<']');
  
  return done;
}

// ##############################################################################################################
void Serial::setCamDev(QString const & dev)
{
  if (dev == "-") { m_camlabel.setText(dev); m_statuslabel.setText(tr("Disconnected")); return; }
  
  //if (dev.startsWith("0x")) m_camlabel.setText("JeVois USB");
  //else if (dev.startsWith("/dev/")) m_camlabel.setText(dev.mid(5));
  //else m_camlabel.setText(dev);

  if (dev.startsWith("/dev/")) m_camlabel.setText(dev.mid(5));
  else m_camlabel.setText("JeVois Camera");

  m_statuslabel.setText(tr("Connected"));
}
