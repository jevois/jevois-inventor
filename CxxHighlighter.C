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

// This file modified from: https://github.com/Xazax-hun/CppQuery

/* Copyright (c) 2014, Gábor Horváth
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:
   
   * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
   
   * Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "CxxHighlighter.H"

// ##############################################################################################################
CxxHighlighter::CxxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

  HighlightRule rule;

  // Keywords
  keywordFormat.setForeground(Qt::darkBlue);
  keywordFormat.setFontWeight(QFont::Bold);

  QStringList keywords;
  keywords << R"(\balignas\b)" << R"(\balignof\b)" << R"(\band\b)" <<
      R"(\band_eq\b)" << R"(\basm\b)" << R"(\bauto\b)" << R"(\bbitand\b)" <<
      R"(\bbitor\b)" << R"(\bbool\b)" << R"(\bbreak\b)" << R"(\bcase\b)"
           << R"(\bcatch\b)" << R"(\bchar\b)" << R"(\bchar16_t\b)" <<
      R"(\bchar32_t\b)" << R"(\bclass\b)" << R"(\bcompl\b)" << R"(\bconst\b)"
           << R"(\bconstexpr\b)" << R"(\bconst_cast\b)" << R"(\bcontinue\b)"
           << R"(\bdecltype\b)" << R"(\bdefault\b)" << R"(\bdelete\b)" <<
      R"(\bdo\b)" << R"(\bdouble\b)" << R"(\bdynamic_cast\b)" << R"(\belse\b)"
           << R"(\benum\b)" << R"(\bexplicit\b)" << R"(\bexport\b)" <<
      R"(\bextern\b)" << R"(\bfalse\b)" << R"(\bfinal\b)" << R"(\bfloat\b)"
           << R"(\bfor\b)" << R"(\bfriend\b)" << R"(\bgoto\b)" <<
      R"(\bif\b)" << R"(\binline\b)" << R"(\bint\b)" << R"(\blong\b)"
           << R"(\bmutable\b)" << R"(\bnamespace\b)" << R"(\bnew\b)" <<
      R"(\bnoexcept\b)" << R"(\bnot\b)" << R"(\bnot_eq\b)" << R"(\bnullptr\b)"
           << R"(\boperator\b)" << R"(\bor\b)" << R"(\bor_eq\b)" <<
      R"(\boverride\b)" << R"(\bprivate\b)" << R"(\bprotected\b)" <<
      R"(\bpublic\b)" << R"(\bregister\b)" << R"(\breinterpret_cast\b)"
           << R"(\breturn\b)" << R"(\bshort\b)" << R"(\bsigned\b)"
           << R"(\bsizeof\b)" << R"(\bstatic\b)" << R"(\bstatic_assert\b)"
           << R"(\bstatic_cast\b)" << R"(\bstruct\b)" << R"(\bswitch\b)" <<
      R"(\btemplate\b)" << R"(\bthis\b)" << R"(\bthread_local\b)" <<
      R"(\bthrow\b)" << R"(\btrue\b)" << R"(\btry\b)" << R"(\btypedef\b)"
           << R"(\btypeid\b)" << R"(\btypename\b)" << R"(\bunion\b)" <<
      R"(\bunsigned\b)" << R"(\busing\b)" << R"(\bvirtual\b)" << R"(\bvoid\b)"
           << R"(\bvolatile\b)" << R"(\bwchar_t\b)" << R"(\bwhile\b)" <<
      R"(\bxor\b)" << R"(\bxor_eq \b)" << R"(\bslots\b)" << R"(\bsignals\b)";

  for (const QString &pattern : keywords) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = keywordFormat;
    highlightRules.push_back(rule);
  }

  // Sigils
  sigilsFormat.setForeground(Qt::red);
  sigilsFormat.setFontWeight(QFont::Bold);

  QStringList sigils;
  sigils << "\\(" << "\\)" << "\\[" << "\\]" << "&" << "\\|"
         << "\\<" << "\\>" << "!" << ";" << "\\{" << "\\}"
         << "\\?" << ":" << "\\+" << "\\*" << "-" << "/"
         << "\\^" << "=" << "~" << "%";

  for (const QString &pattern : sigils) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = sigilsFormat;
    highlightRules.push_back(rule);
  }

  // Comments
  singleLineCommentFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegularExpression("//[^\n]*");
  rule.format = singleLineCommentFormat;
  highlightRules.push_back(rule);

  multiLineCommentFormat.setForeground(Qt::darkGreen);

  commentStartExpression = QRegularExpression("/\\*");
  commentEndExpression = QRegularExpression("\\*/");

  // String literals
  quotationFormat.setForeground(Qt::gray);
  rule.pattern = QRegularExpression("\"(\\.|[^\"])*\"");
  rule.format = quotationFormat;
  highlightRules.push_back(rule);

  // Directives
  directiveFormat.setForeground(Qt::darkGray);
  rule.pattern = QRegularExpression("#[^\n]*");
  rule.format = directiveFormat;
  highlightRules.push_back(rule);
}

// ##############################################################################################################
void CxxHighlighter::highlightBlock(const QString &text) {

  for (const HighlightRule &rule : highlightRules) {
    const QRegularExpression &expression(rule.pattern);
    QRegularExpressionMatch m = expression.match(text);
    while (m.hasMatch()) {
      int index = m.capturedStart();
      int length = m.capturedLength();
      setFormat(index, length, rule.format);
      m = expression.match(text, index + length);
    }
  }

  // Multiline comments
  setCurrentBlockState(0);

  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = commentStartExpression.match(text).capturedStart();

  while (startIndex >= 0) {
    QRegularExpressionMatch m = commentEndExpression.match(text, startIndex);
    int endIndex = m.capturedStart();
    int commentLength;
    if (endIndex == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    } else {
      commentLength = endIndex - startIndex + m.capturedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = commentStartExpression.match(text, startIndex + commentLength)
                     .capturedStart();
  }
}
