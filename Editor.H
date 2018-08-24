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
#include <QPushButton>
#include <QVBoxLayout>

#include "BaseEdit.H"

class Serial;

class Editor : public QWidget
{
    Q_OBJECT

  public:
    enum class SaveAction { None, Reload, Reboot };
    
    //! Constructor
    /*! Note that we take ownership of "edit" and will delete it on destruction */
    explicit Editor(Serial * serport, QString const & fname, bool pasteButtons, QString const & deftext,
		    BaseEdit * edit, SaveAction sa, bool margin, QWidget * parent = 0);
    virtual ~Editor();
    
    void tabselected();
    void setFile(QString const & fname);
    void reset();
    void afterSave(QStringList const & data);
    BaseEdit * baseEdit();
    void setBaseEdit(BaseEdit * be);
    void pasteCam(QStringList const & ci);
    void pastePar(QStringList const & pi);
    void edited(QStringList & filenames) const;
			 
  public slots:
    void loadFile(bool noask);
    void saveFile(bool noask);
    void loadFileLocal(bool noask);
    void saveFileLocal(bool noask);

  signals:
    void askReboot();
    
  private:
    BaseEdit * m_edit;
    Serial * m_serial;
    QString m_fname;
    QString const & m_deftext;
    SaveAction m_saveaction;
    QPushButton m_load;
    QPushButton m_save;
    QPushButton m_locload;
    QPushButton m_locsave;
    QPushButton m_pastecam;
    QPushButton m_pastepar;
    QVBoxLayout m_vlayout;
};
