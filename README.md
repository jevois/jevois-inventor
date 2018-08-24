
JeVois Inventor is a graphical frontend to interact with a JeVois Smart Camera.

See http://jevois.org for more information.

[![JeVois Inventor Youtube Intro](https://img.youtube.com/vi/XVMk8lRvm8k/0.jpg)](https://www.youtube.com/watch?v=XVMk8lRvm8k)

Installation and running
------------------------

Linux:
- download jevois-inventor_XXX.deb from http://jevois.org/start (XXX will vary depending on version)
- sudo dpkg -i jevois-inventor_XXX.deb   # (XXX will vary depending on version)
- sudo killall ModemManager  # ModemManager interferes with JeVois
- jevois-inventor

MacOS:
- download jevois-inventor_XXX.dmg from http://jevois.org/start (XXX will vary depending on version)
- double-ckick on the DMG file to open it
- drag jevois-inventor to your desktop or Applications folder
- double-click on jevois-inventor

Windows:
- download jevois-inventor_XXX.zip from http://jevois.org/start (XXX will vary depending on version)
- windows 7 (not 8 or 10) users need to install a driver, see http://jevois.org/doc/USBserialWindows.html
- extract zip contents
- double-click on jevois-inventor.exe

Building from source
--------------------

- Install the latest Qt (we used 5.11 for development)
- qmake -config release
- make


License
-------

JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern California
(USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.

This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.  You should have received a copy of the GNU General Public License along with this program; if
not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.  Tel: +1 213 740 3527 -
itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org

