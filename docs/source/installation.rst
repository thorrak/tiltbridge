.. include:: global.rst

Installing the Firmware on an ESP32
===================================

Installing TiltBridge on an ESP32 can be accomplished using either Fermentrack or the Espressif esptool.py script.

Installation Using Fermentrack
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
#. Download all three of the TiltBridge firmware files from `GitHub`_ - tiltbridge-spiffs.bin, tiltbridge.bin, and tiltbridge-partitions.bin
#. Connect the ESP32 board to the computer you will be using to flash
#. Open a command prompt and run the flash command ``esptool.py --chip esp32 --before default_reset --after hard_reset write_flash 0x10000 tiltbridge.bin 0x8000 tiltbridge-partitions.bin 0x3D1000 tiltbridge-spiffs.bin``


Next Steps
----------

Once your controller is flashed, secure it in its case/enclosure, and proceed to the :doc:`User Guide/initial_setup` process.

