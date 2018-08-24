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

#include "CfgHighlighter.H"

// ##############################################################################################################
CfgHighlighter::CfgHighlighter(std::map<QString /* command */, QString /* description */> cmd,
			       std::map<QString /* command */, QString /* description */> modcmd,
			       QStringList const & paramNames,
			       QStringList const & ctrlNames,
			       QTextDocument * parent) :
    QSyntaxHighlighter(parent)
{
  HighlightingRule rule;

  // Rules for jevois commands:
  cmdFormat.setForeground(Qt::darkBlue);
  cmdFormat.setFontWeight(QFont::Bold);
  for (auto const & c : cmd)
  {
    DEBU("cmd: ["<<c<<']');
    auto vec = c.first.split(' '); 
    rule.pattern = QRegularExpression("\\b" + vec[0] + "\\b");
    rule.format = cmdFormat;
    highlightingRules.append(rule);
  }
  cmdFormat.setForeground(Qt::darkRed);
  cmdFormat.setFontWeight(QFont::Bold);
  for (auto const & c : modcmd)
  {
    auto vec = c.first.split(' '); 
    rule.pattern = QRegularExpression("\\b" + vec[0] + "\\b");
    rule.format = cmdFormat;
    highlightingRules.append(rule);
  }

  // Rules for video modes:
  vmodeFormat.setFontWeight(QFont::Bold);
  vmodeFormat.setForeground(Qt::darkMagenta);
  QStringList modes; modes << "YUYV" << "BAYER" << "RGB565" << "GREY" << "MJPG" << "BGR24" << "NONE";
  for (auto const & m : modes)
  {
    rule.pattern = QRegularExpression("\\b" + m + "\\b");
    rule.format = vmodeFormat;
    highlightingRules.append(rule);
  }

  // Rules for strings in quotes;
  quotationFormat.setForeground(Qt::red);
  rule.pattern = QRegularExpression("\".*\"");
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  // Rules for parameters:
  paramFormat.setFontItalic(true);
  paramFormat.setForeground(Qt::blue);
  for (QString const & p : paramNames)
  {
    DEBU("param: ["<<p<<']');
    rule.pattern = QRegularExpression("\\b" + p + "\\b");
    rule.format = paramFormat;
    highlightingRules.append(rule);
  }

  // Rules for camera controls:
  ctrlFormat.setFontItalic(true);
  ctrlFormat.setForeground(Qt::magenta);
  for (QString const & p : ctrlNames)
  {
    DEBU("ctrl: ["<<p<<']');
    rule.pattern = QRegularExpression("\\b" + p + "\\b");
    rule.format = ctrlFormat;
    highlightingRules.append(rule);
  }
  
  // Rules for comments:
  singleLineCommentFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegularExpression("#[^\n]*");
  rule.format = singleLineCommentFormat;
  highlightingRules.append(rule);
  
  multiLineCommentFormat.setForeground(Qt::darkGreen);
  commentStartExpression = QRegularExpression("'''");
  commentEndExpression = QRegularExpression("'''");
}

// ##############################################################################################################
void CfgHighlighter::highlightBlock(const QString &text)
{
  foreach (const HighlightingRule &rule, highlightingRules)
  {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext())
    {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }
  setCurrentBlockState(0);
  
  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = text.indexOf(commentStartExpression);
  
  while (startIndex >= 0)
  {
    QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
    int endIndex = match.capturedStart();
    int commentLength = 0;
    if (endIndex == -1)
    {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    }
    else
    {
      commentLength = endIndex - startIndex + match.capturedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
  }
}
