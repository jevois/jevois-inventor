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
#include <QSerialPort>
#include <QLabel>
#include <QSerialPortInfo>

#include <functional>

class Serial : public QWidget
{
    Q_OBJECT
    
  public:
    explicit Serial(QWidget * parent = 0);
    ~Serial();
    
    bool detect();
    void closedown();
    
    QSharedPointer<QSerialPort> port() const;

    //! Set camera device name, for display purposes only
    void setCamDev(QString const & dev);
    
    //! Write a string, after converting it to Latin1 and adding a newline
    /*! Used for commands typed in by users */
    void write(QString const & str);
    
    //! Get received data, only from modules or hand-written commands
    QStringList readAll();

    //! Set a camera parameter
    void setCam(QString const & name, QString const & value,
                std::function<void(QStringList const &)> callback = std::function<void(QStringList const &)>(),
		std::function<void(QStringList const &)> errcallback = std::function<void(QStringList const &)>() );

    //! Set a camera parameter
    void setCam(QString const & name, int value,
                std::function<void(QStringList const &)> callback = std::function<void(QStringList const &)>(),
		std::function<void(QStringList const &)> errcallback = std::function<void(QStringList const &)>() );

    //! Set a parameter
    void setPar(QString const & name, QString const & value,
                std::function<void(QStringList const &)> callback = std::function<void(QStringList const &)>(),
		std::function<void(QStringList const &)> errcallback = std::function<void(QStringList const &)>() );
    
    //! Send a buffer to JeVois as a file
    void sendBuffer(QString const & fname, QByteArray const & data,
		    std::function<void(QStringList const &)> callback = std::function<void(QStringList const &)>(),
		    std::function<void(QStringList const &)> errcallback = std::function<void(QStringList const &)>());
    
    //! Send a local file to JeVois
    void sendTextFile(QString const & fname, QString const & localfname,
		      std::function<void(QStringList const &)> callback =
		      std::function<void(QStringList const &)>(),
		      std::function<void(QStringList const &)> errcallback =
		      std::function<void(QStringList const &)>());

    //! Send a local file to JeVois
    void sendBinaryFile(QString const & fname, QString const & localfname,
			std::function<void(QStringList const &)> callback =
			std::function<void(QStringList const &)>(),
			std::function<void(QStringList const &)> errcallback =
			std::function<void(QStringList const &)>());

    //! Receive a buffer from JeVois
    void receiveTextBuffer(QString const & fname,
                           std::function<void(QStringList const &)> callback =
                           std::function<void(QStringList const &)>(),
			   std::function<void(QStringList const &)> errcallback =
			   std::function<void(QStringList const &)>());

    //! Receive a binary buffer from JeVois
    void receiveBinaryBuffer(QString const & fname,
                             std::function<void(QByteArray const &)> callback =
                             std::function<void(QByteArray const &)>(),
			     std::function<void(QStringList const &)> errcallback =
			     std::function<void(QStringList const &)>());

    //! Send a command and send the results to an optional callback
    void command(QString const & cmd,
		 std::function<void(QStringList const &)> callback = std::function<void(QStringList const &)>(),
		 std::function<void(QStringList const &)> errcallback = std::function<void(QStringList const &)>() );
  public slots:
    void serialPing();

  private slots:
    void readDataReady();
    void writeDataDone(qint64 bytes);
    
  signals:
    void readyRead();
    void writeError();
    
  private:
    QLabel m_portlabel;
    QLabel m_camlabel;
    QLabel m_statuslabel;
    QSharedPointer<QSerialPort> m_serial;
    QString m_portname;
    QByteArray m_todo; // data to be parsed but contained incomplete text lines
    QStringList m_data; // data not related to our jobs
    
    enum JobType { JobNone, JobCommand, JobFileGet, JobFilePut };
    class JobData
    {
      public:
        JobType type;
        std::function<void(QStringList const &)> callback;
        QString cmd;
        QByteArray data;
        QStringList cmdret;
        int datasize;
        bool sent;
        std::function<void(QByteArray const &)> bincallback;
        std::function<void(QStringList const &)> errcallback;
    };
    
    QList<JobData> m_wq; // Our work queue
    
    // split off serout/serlog vs command outputs, return true if complete (OK or ERR):
    bool parseReceived(QByteArray & ret, JobData * jd);

    bool createPort(QSerialPortInfo const & portinfo);

#ifdef Q_OS_WIN
    QSerialPortInfo m_serinfo; // remember user selection when manually selecting a device
#endif
};
