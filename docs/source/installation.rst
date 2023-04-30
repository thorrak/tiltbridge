.. include:: global.rst

Installing the Firmware on an ESP32
===================================

Installing TiltBridge on an ESP32 can be accomplished using either `BrewFlasher`_, `Fermentrack`_, or the Espressif
esptool.py script.


Installation using BrewFlasher
------------------------------

The easiest way to install TiltBridge is using `BrewFlasher`_. BrewFlasher is a standalone application for Windows and Mac OS
which allows flashing the same set of firmware that is available through Fermentrack without having to delve into the command line.
It is free, easy to use, and is recommended for most users.

To install using BrewFlasher simply connect your ESP32 to your computer, download and open BrewFlasher, and follow the prompts on
screen.

Note about MacOS
~~~~~~~~~~~~~~~~

.. _drivers: https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

Although the microcontroller used on each ESP32 board is standardized (thats what the "ESP32" is) the serial-to-usb chip
differs from manufacturer to manufacturer. Some serial-to-USB chips require special drivers to be installed on MacOS in
order to be recognized by the operating system. Some users who have had difficulty getting MacOS to recognize their
boards in order to be able to flash them have reported success getting their ESP32-based chips to
show up in MacOS by installing these drivers_.



Installation using Fermentrack
------------------------------

Installation via `Fermentrack`_ is incredibly easy as it leverages the existing "firmware flash" workflow. To flash via
this method simply log into your Fermentrack installation, choose "Flash Device" from the devices menu, select the
"ESP32" device family, and follow the prompts. When prompted to choose a firmware to flash simply choose "TiltBridge".


Installation using esptool.py
-----------------------------

TiltBridge can also be installed using esptool.py. Although slightly more involved, this method does not require the use
of a Raspberry Pi running `Fermentrack`_, or the use of BrewFlasher.

As this installation method is intended for advanced users, instructions are not provided here. You will need to ensure that all three images (the firmware, partitions, and spiffs files) are all flashed to the correct location. You can determine the correct location for the SPIFFS partition by looking at platformio.ini, determining the correct partition CSV, and then looking for the appropriate address in that CSV.


Next Steps
----------

Once your controller is flashed, secure it in its case/enclosure, and proceed to the :doc:`User Guide/initial_setup` process.

