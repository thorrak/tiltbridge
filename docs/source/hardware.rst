TiltBridge Hardware
===================

The first step in a TiltBridge installation is sourcing the necessary hardware. The only component required for a
TiltBridge is an ESP32-based board - however selecting the LOLIN D32 Pro and adding the compatible TFT screen will
result in the best experience with the most features.

.. note:: While this page documents hardware that has ben demonstrated to work with TiltBridge in the past, the author of TiltBridge does not warrant that any of the hardware listed below continues to be operational. Additionally, although links to vendors are provided to show examples of the referenced hardware, no guarantees are made as to the vendors themselves. Buy hardware you trust from a vendor you trust.


LOLIN D32 Pro + TFT
-------------------

The recommended board for this project is the LOLIN D32 Pro, along with its TFT screen and cable. This set of hardware
enables the full suite of TiltBridge functionality without needing to solder anything. Simply add an enclosure and
you're done!

Sources
*******

Although the only required component is the board, you will likely also want the TFT screen & cable:

- `AliExpress (board) <https://www.aliexpress.com/item/32883116057.html>`_
- `AliExpress (TFT Screen) <https://www.aliexpress.com/item/32919729730.html>`_
- `AliExpress (Cable) <https://www.aliexpress.com/item/32848833474.html>`_

.. note:: For the board, you will want one of the 16MB variants. Preference should go to the non-LED models.


Enclosures
**********

- `Sample Case on Thingiverse <https://www.thingiverse.com/thing:3515836>`_

.. todo:: Check this link



Additional Hardware Required
----------------------------

In addition to the ESP32 board, you will also need a USB power source to power the ESP32 (such as a phone charger), a
MicroUSB cable for flashing, and you will likely want some kind of enclosure.

Once you have acquired the necessary hardware, you are ready to begin :doc:`User Guide/initial_setup`




Notes about Previous Versions/Other Hardware
--------------------------------------------

TiltBridge is designed to run either with or without an attached screen, and therefore can be used with a variety of
other boards. Although this functionality exists, due to the myriad issues experienced with OLED boards, non-LOLIN
boards, etc. the use of boards other than the LOLIN D32 Pro are not currently recommended.

If you use another board, make sure to select the appropriate firmware:

- "OLED" boards and those with less than 16MB of flash should select the "OLED" firmware variant
- Any non-"OLED" board with 16MB of flash or greater should select the "TFT" firmware variant
