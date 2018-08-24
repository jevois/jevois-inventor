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

#include "PythonEdit.H"
#include "Serial.H"

#include <QTextOption>

// ##############################################################################################################
PythonEdit::PythonEdit(Serial * serport, QWidget * parent) :
    BaseEdit(serport, parent),
    m_hi(document())
{
  // Show white space as this is important for python:
  QTextOption option = document()->defaultTextOption();
  option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
  document()->setDefaultTextOption(option);
}

// ##############################################################################################################
PythonEdit::~PythonEdit()
{ }

// ##############################################################################################################
void PythonEdit::setData(QString const & txt)
{
  setPlainText(txt);
}

// ##############################################################################################################
void PythonEdit::keyPressEvent(QKeyEvent * event)
{

  switch (event->key())
  {
  case Qt::Key_Return:
  case Qt::Key_Enter:
  case Qt::Key_Tab:
  case Qt::Key_Backtab:
  {
    QString const text = textCursor().block().text();

    switch (event->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
      // Count number of leading spaces in current line:
      int const siz = text.size();
      int count = 0; for (int i = 0; i < siz; ++i) if (text[i] == ' ') ++count; else break;
      
      // Switch to the next indentation level if the line ends with ':', unless we have 'else':
      QString const mid = text.mid(count);
      if (mid.startsWith("else ") || mid.startsWith("else:"))
      {
	// Decrease indent by smaller of count or our tab size on the current line:
	int const count2 = std::min(count, JVINV_TAB);
      
	// Delete some spaces in front of the line:
	QTextCursor c = textCursor();
	c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
	c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, count2);
	c.removeSelectedText();

	// Now that we dedented the current line (with the else), next line should be indented by count,
	// after we execute the return below.
      }
      else if (mid.endsWith(":")) count += JVINV_TAB;
      
      // Execute the RETURN:
      BaseEdit::keyPressEvent(event);
      
      // Insert count spaces:
      if (count > 0) insertPlainText(QString(count, ' '));
    }
    break;

    case Qt::Key_Tab:
    {
      // Insert some spaces in front of the line and swallow the event:
      QTextCursor c = textCursor();
      c.movePosition(QTextCursor::StartOfLine);
      c.insertText(QString(JVINV_TAB, ' '));
    }
    break;

    case Qt::Key_Backtab:
    {
      // Count number of leading spaces in current line:
      int const siz = text.size();
      int count = 0; for (int i = 0; i < siz; ++i) if (text[i] == ' ') ++count; else break;

      // Decrease indent by smaller of count or our tab size:
      count = std::min(count, JVINV_TAB);
      
      // Delete some spaces in front of the line and swallow the event:
      QTextCursor c = textCursor();
      c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
      c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, count);
      c.removeSelectedText();
    }
    break;
    
    default: // should never happen
      BaseEdit::keyPressEvent(event);
      break;
    }
    
  }
  break;

  default:
    BaseEdit::keyPressEvent(event);
    break;
  }
}

