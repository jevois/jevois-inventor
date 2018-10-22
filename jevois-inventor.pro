# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#
# JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
# California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
#
# This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
# redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.  You should have received a copy of the GNU General Public License along with this program;
# if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
# Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VERSION = 0.3.0

QT       += core gui 

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia multimediawidgets serialport

TARGET = jevois-inventor
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.C \
        Camera.C \
        JeVoisInventor.C \
        TopPanel.C \
        Serial.C \
        Console.C \
        CamControls.C \
        CfgEdit.C \
        CfgHighlighter.C \
        Editor.C \
        CfgStack.C \
        Utils.C \
        VideoMapping.C \
        Parameters.C \
        ParamInfo.C \
        HistoryLineEdit.C \
        PythonSyntaxHighlighter.C \
        CxxHighlighter.C \
        BaseEdit.C \
        PythonEdit.C \
        CxxEdit.C \
        System.C \
        RangeSlider.C \
        SpinSlider.C \
        SpinRangeSlider.C

        
#        VideoWidget.C


HEADERS += \
        Camera.H \
        JeVoisInventor.H \
        TopPanel.H \
        Serial.H \
        Console.H \
        CamControls.H \
        CfgEdit.H \
        CfgHighlighter.H \
        Editor.H \
        CfgStack.H \
        Utils.H \
        VideoMapping.H \
        Parameters.H \
        ParamInfo.H \
        HistoryLineEdit.H \
        PythonSyntaxHighlighter.H \
        CxxHighlighter.H \
        BaseEdit.H \
        PythonEdit.H \
        CxxEdit.H \
        System.H \
        Config.H \
        RangeSlider.H \
        SpinSlider.H \
        SpinRangeSlider.H
        
#        VideoWidget.H

        
FORMS += \

RESOURCES = jevois-inventor.qrc

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11

CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17

# icon for windows
win32:RC_ICONS += jvinv-icon.ico

QMAKE_TARGET_COMPANY="JeVois.org"
QMAKE_TARGET_DESCRIPTION="JeVois Inventor"
QMAKE_TARGET_COPYRIGHT="Copyright (C) 2018 by Laurent Itti, the University of Southern California (USC), and iLab at USC."
QMAKE_TARGET_PRODUCT="JeVois Inventor"
RC_LANG=1033
RC_CODEPAGE=1200

QMAKE_BUNDLE_DATA += README.md COPYING CREDITS
