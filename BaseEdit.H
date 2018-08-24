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

#include <QPlainTextEdit>

class Serial;

class BaseEdit : public QPlainTextEdit
{
    Q_OBJECT

  public:
    BaseEdit(Serial * serport, QWidget * parent = 0);
    virtual ~BaseEdit();

    virtual void setData(QString const & txt) = 0;
    
    bool empty() const;

    //! Return true if document was edited since the last setData
    bool edited() const;

    //! Grab CTRL-S to save to JeVois
    void keyPressEvent(QKeyEvent * event) override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

  signals:
    void saveRequest(bool noask);
    void localSaveRequest(bool noask);

  private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(QRect const &, int);
    
  protected:
    void resizeEvent(QResizeEvent * event) override;
    QWidget * lineNumberArea;
    Serial * m_serial;
};

//! Auxiliary widget to show line numbers
/*! Pretty much straight from http://doc.qt.io/qt-5/qtwidgets-widgets-codeeditor-example.html */
class LineNumberArea : public QWidget
{
  public:
    LineNumberArea(BaseEdit * editor);
    
    QSize sizeHint() const override;
    
  protected:
    void paintEvent(QPaintEvent *event) override;
    
  private:
    BaseEdit * m_editor;
};
