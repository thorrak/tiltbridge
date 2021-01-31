.. include:: ../global.rst

Other TiltBridge Features/Settings
==================================

TiltBridge also has features to simplify both keeping your TiltBridge up to date as well as resetting the WiFi network
settings.

Updating your TiltBridge Firmware
---------------------------------

Updates to TiltBridge can be easily performed by flashing the latest available firmware from `GitHub`_ using either
`BrewFlasher`_, `Fermentrack`_, or your favorite ESP32 flashing software.


Resetting your WiFi Configuration Settings
------------------------------------------

There are two options for resetting your TiltBridge's WiFi configuration settings.

If you are able to log into your TiltBridge using a web browser, do so, and then click the "Settings" option at the top
of the page. Once the settings page loads, choose the "Reset" tab and then select "Reset WiFi". Click the "Reset WiFi"
button to confirm that you want to delete the TiltBridge's network configuration & allow
it to be configured using the TiltBridge configuration AP.

If you are not able to log into your TiltBridge using a web browser, you can also reset the WiFi settings using the "boot"
button (if available) on your OLED/eSPI TiltBridge device. To use this, simply push the "boot" button on the TiltBridge while the
device is scanning for Tilt Hydrometers. After you've pushed the button once, press it a second time within several
seconds to confirm that you want to reset your WiFi settings. For TFT-based TiltBridges without a "boot" button, simply
tap the screen twice.

