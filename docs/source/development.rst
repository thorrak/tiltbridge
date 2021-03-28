.. include:: ./global.rst

Notes for Developers
====================

Interested in extending TiltBridge or integrating it into your project? Awesome!

While I developed TiltBridge with a specific feature set in mind, the project is open source for a reason. There are
plenty of features that I haven't imagined which would make the project even better than it is. That said, before
developing a new feature and submitting a pull request, I strongly recommend reaching out. Although recent optimizations
have increased the available RAM and flash space on the ESP32, they remain at a premium and thus not all features may
be able to be merged in.


Developer Setup
---------------

This section is for developers interested in building and flashing firmware locally.

TiltBridge firmware uses the `PlatformIO`_ framework to manage toolchains, libraries,
and board definitions. If you're interested it building/modifying your own firmeware,
you'll want to download and install PlatformIO from the link above.

This section will give you a quick crash course on using the ``pio`` command-line tool.

Environments
~~~~~~~~~~~~

TiltBridge supports several different board types. In PlatformIO, the board type and
board-specific settings are called an "environment". TiltBridge ships with a few
environments defined in the ``platformio.ini`` file.

Building firmware
~~~~~~~~~~~~~~~~~

To build a firmware, determine what environment (board) you're targeting, then use
the ``pio run`` command to build it::

  $ pio run -e d32_pro_tft

Uploading firmware
~~~~~~~~~~~~~~~~~~

Use the ``--target upload`` flag to upload the firmware to your device. This step will
also build the firmware::

  $ pio run -e d32_pro_tft --target upload

Erasing filesystem
~~~~~~~~~~~~~~~~~~

If you repartitioned your device, it may be necessary to upload/initialize the filesystem
on the device. Use the ``uploadfs`` target to do so::

  $ pio run -e d32_pro_tft --target uploadfs
