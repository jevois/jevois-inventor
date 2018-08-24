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

#include "BaseEdit.H"
#include "Serial.H"

#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>

// ##############################################################################################################
BaseEdit::BaseEdit(Serial * serport, QWidget * parent) :
    QPlainTextEdit(parent),
    m_serial(serport)
{
  QFont font;
  font.setFamily("Courier");
  font.setFixedPitch(true);
  //font.setPointSize(10);
  
  setFont(font);
  setLineWrapMode(QPlainTextEdit::NoWrap);

  lineNumberArea = new LineNumberArea(this);
  connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
  connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
  updateLineNumberAreaWidth(0);
  highlightCurrentLine();
}

// ##############################################################################################################
BaseEdit::~BaseEdit()
{ }

// ##############################################################################################################
bool BaseEdit::empty() const
{
  return document()->isEmpty();
}

// ##############################################################################################################
bool BaseEdit::edited() const
{
  return document()->isModified();
}

// ##############################################################################################################
void BaseEdit::keyPressEvent(QKeyEvent * event)
{
  if (event->matches(QKeySequence::Save))
    emit saveRequest(true);
  else if (event->matches(QKeySequence::SaveAs))
    emit localSaveRequest(true);

  QPlainTextEdit::keyPressEvent(event);
}

// ##############################################################################################################
int BaseEdit::lineNumberAreaWidth()
{
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10) { max /= 10; ++digits; }
  int space = 5 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
  return space;
}

// ##############################################################################################################
void BaseEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

// ##############################################################################################################
void BaseEdit::updateLineNumberArea(const QRect &rect, int dy)
{
  if (dy)
    lineNumberArea->scroll(0, dy);
  else
    lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
  
  if (rect.contains(viewport()->rect()))
    updateLineNumberAreaWidth(0);
}

// ##############################################################################################################
void BaseEdit::resizeEvent(QResizeEvent *e)
{
  QPlainTextEdit::resizeEvent(e);
  
  QRect cr = contentsRect();
  lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

// ##############################################################################################################
void BaseEdit::highlightCurrentLine()
{
  QList<QTextEdit::ExtraSelection> extraSelections;
  
  if (!isReadOnly())
  {
    QTextEdit::ExtraSelection selection;
    
    QColor lineColor = QColor(Qt::yellow).lighter(185);
    
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
  }
  
  setExtraSelections(extraSelections);
}

// ##############################################################################################################
void BaseEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
  QPainter painter(lineNumberArea);
  painter.fillRect(event->rect(), QColor(Qt::lightGray).lighter(120));
  
  
  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
  int bottom = top + (int) blockBoundingRect(block).height();
  
  while (block.isValid() && top <= event->rect().bottom())
  {
    if (block.isVisible() && bottom >= event->rect().top())
    {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(Qt::black);
      painter.drawText(0, top, lineNumberArea->width()-2, fontMetrics().height(), Qt::AlignRight, number);
    }
    
    block = block.next();
    top = bottom;
    bottom = top + (int) blockBoundingRect(block).height();
    ++blockNumber;
  }
}

// ##############################################################################################################
// ##############################################################################################################
LineNumberArea::LineNumberArea(BaseEdit * editor) :
    QWidget(editor), m_editor(editor)
{ }

QSize LineNumberArea::sizeHint() const
{
  return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent * event)
{
  m_editor->lineNumberAreaPaintEvent(event);
}
