#! /bin/bash

# Install Custom Icon into .mxo bundle
#
# Uses environment variable SRCROOT, INSTALL_DIR, PRODUCT_NAME

#echo SRCROOT = $SRCROOT
#echo INSTALL_DIR = $INSTALL_DIR
#echo PRODUCT_NAME = $PRODUCT_NAME

# The easiest way we have found to handle the filename Icon\r is to drop the
# file named such as the only file inside a subdirectory, as below.
#
# Resources/IconSuites/LitterIconSuite.mxo.Pro only contains a single file, named Icon\r
/Developer/Tools/CpMac Resources/IconSuites/LitterIconSuite.mxo.Pro/Icon? "$TARGET_BUILD_DIR/$PRODUCT_NAME.mxo/"

/Developer/Tools/SetFile -a V "$TARGET_BUILD_DIR/$PRODUCT_NAME.mxo/Icon"?
/Developer/Tools/SetFile -a C "$TARGET_BUILD_DIR/$PRODUCT_NAME.mxo"
touch "$TARGET_BUILD_DIR/$PRODUCT_NAME.mxo"