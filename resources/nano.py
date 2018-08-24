import libjevois as jevois
import cv2
import numpy as np

## @SYNOPSIS@
#
# Add some description of your module here.
#
# @author @AUTHOR@
# 
# @videomapping @VIDEOMAPPING@
# @email @EMAIL@
# @address 123 first street, Los Angeles CA 90012, USA
# @copyright Copyright (C) 2018 by @AUTHOR@
# @mainurl @WEBSITE@
# @supporturl @WEBSITE@
# @otherurl @WEBSITE@
# @license @LICENSE@
# @distribution Unrestricted
# @restrictions None
# @ingroup modules
class @MODULE@:
    
    def process(self, inframe, outframe):
        img = inframe.getCvBGR()
        outframe.sendCv(img)
