TiltBridge Hardware
===================


The first step in a TiltBridge installation is sourcing the necessary hardware. The only component required for a
TiltBridge is an ESP32-based board - however selecting the LOLIN D32 Pro and adding the compatible TFT screen will
result in the best experience with the most features.

.. note:: While this page documents hardware that has ben demonstrated to work with TiltBridge in the past, the author of TiltBridge does not warrant that any of the hardware listed below continues to be operational. Additionally, although links to vendors are provided to show examples of the referenced hardware, no guarantees are made as to the vendors themselves. Buy hardware you trust from a vendor you trust.


LOLIN D32 Pro + TFT
-------------------



Although the full "TTGO" board includes features we do not need like LoRa radios, there are a handful of boards in the
"TTGO" style that only have the OLED screen. These boards are very small, and have the advantage of having a screen that
is secured to the PCB itself. These boards run hot, but as long as the case has some ventilation they should be fine. An
optional heatsink can be added. These boards do not have any mounting holes which requires that they be secured by other
means.

For these boards, pin 16 is required to be set high to power the board, the address is 0x3c, SDA is 4, and SCK is 15.
TiltBridge will automatically detect boards with this configuration and enable the OLED screen accordingly.


Sources
*******

You will need the board, but will likely also want the TFT screen & cable:

- `AliExpress (board) <https://www.aliexpress.com/item/32883116057.html>`_
- `AliExpress (TFT Screen) <https://www.aliexpress.com/item/32919729730.html>`_
- `AliExpress (Cable) <https://www.aliexpress.com/item/32848833474.html>`_

.. note:: For the board, any of the 3 "colors" are fine - 16MB FLASH, 4MB FLASH, or 16MB FLASH(LED). Preference should go to the non-LED models, but all 3 will work.


Enclosures
**********

- `Sample Case on Thingiverse <https://www.thingiverse.com/thing:3515836>`_

.. todo:: Check this link



Other Boards
------------

TiltBridge is designed to operate without an attached screen, and as such most other ESP32 boards should successfully
run the firmware.


Additional Hardware Required
----------------------------

In addition to the ESP32 board, you will also need a USB power source to power the ESP32 (such as a phone charger), a
MicroUSB cable for flashing, and you will likely want some kind of enclosure.


Once you have acquired the necessary hardware, you are ready to begin :doc:`User Guide/initial_setup`




Notes about Previous Versions/Other Hardware
--------------------------------------------

Previous versions of TiltBridge supported other boards, including OLED-based ESP32 boards. Although these are technically
still supported, due to significant issues consistently sourcing working hardware, other boards are no longer recommended.

