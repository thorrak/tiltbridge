.. include:: global.rst

Installing the Firmware on an ESP32
===================================

Installing TiltBridge on an ESP32 can be accomplished using either `BrewFlasher`_, `Fermentrack`_, or the Espressif esptool.py script.


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
of a Raspberry Pi running `Fermentrack`_.

.. note:: The command below will automatically detect & flash any Espressif chips connected to the computer running esptool.py. Please leave only the device you are setting up connected to the computer to avoid the risk of misflashing.


#. Install esptool.py using the `instructions here <https://github.com/espressif/esptool#installation--dependencies>`_.
#. Download all **five** of the appropriate TiltBridge firmware files for your controller from `GitHub`_ and rename them to the following - spiffs.bin, firmware.bin, partitions.bin, boot_app0.bin, and bootloader_dio_40m.bin
#. Connect the ESP32 board to the computer you will be using to flash
#. Open a command prompt and run the appropriate flash command


The appropriate flash command for your build depends on the version firmware you are using. You may need to tweak the command, but sample flash commands are as follows:

- **"TFT" Firmware:** ``esptool.py --chip esp32 --before default_reset --after hard_reset write_flash 0xe000 boot_app0.bin 0x1000 bootloader_dio_40m.bin 0x10000 firmware.bin 0x8000 partitions.bin 0x910000 spiffs.bin``
- **"OLED" Firmware:** ``esptool.py --chip esp32 --before default_reset --after hard_reset write_flash 0xe000 boot_app0.bin 0x1000 bootloader_dio_40m.bin 0x10000 firmware.bin 0x8000 partitions.bin 0x310000 spiffs.bin``

Next Steps
----------

Once your controller is flashed, secure it in its case/enclosure, and proceed to the :doc:`User Guide/initial_setup` process.

