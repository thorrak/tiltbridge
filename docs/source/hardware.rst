TiltBridge Hardware
===================

The first step in a TiltBridge installation is sourcing the necessary hardware. The only component required for a
TiltBridge is an ESP32-based board - however selecting the LOLIN D32 Pro and adding the compatible TFT screen will
result in the best experience with the most features. For a smaller build, Heltec-style ESP32 OLED boards are also
tested to work with the latest version of TiltBridge.

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

.. note:: For the board, you need one of the 16MB variants. Preference should go to the non-LED models (though the LED models work just as well).


Heltec-Style ESP32 OLED Board
-----------------------------

If you're looking for a smaller build (or the LOLIN D32 Pro is out of stock) then a Heltec-style board can be used in
its place. These boards are compact and work well, but lack the large flash and large screen of the D32 Pro.
Additionally, the location of the cutout for the screen may need to be adjusted depending on where the OLED screen is
affixed to the PCB.

Sources
*******

- `Heltec WiFi Kit 32 <https://heltec.org/project/wifi-kit-32/>`_



Additional Hardware Required
----------------------------

In addition to the ESP32 board, you will also need a USB power source to power the ESP32 (such as a phone charger), a
MicroUSB cable for flashing, and you will likely want some kind of enclosure (see below).

Once you have acquired the necessary hardware, you are ready to begin :doc:`User Guide/initial_setup`


Enclosures
**********

Although not required, an enclosure is recommended. There are a number of designs out there depending on the board you
choose.

D32 Pro Cases
~~~~~~~~~~~~~

- `D32 Pro + TFT Case by gromitdj <https://www.thingiverse.com/thing:4368639>`_


OLED Cases
~~~~~~~~~~

- `TiltBridge Heltec/TTGO ESP32 OLED Enclosure by Thorrak <https://www.thingiverse.com/thing:3515836>`_
- `Generic "OLED" Enclosure by Thorrak <https://www.thingiverse.com/thing:3604590>`_
- `HiLetGo Wifi 32 Tiltbridge Case by calandryll <https://www.thingiverse.com/thing:4444391>`_



Notes about Previous Versions/Other Hardware
--------------------------------------------

TiltBridge is designed to run either with or without an attached screen, and therefore can be used with a variety of
other boards than the ones recommended above. If you use another board, make sure to select the appropriate firmware:

- "OLED" boards, those with less than 16MB of flash, and those lacking a screen entirely should select the "OLED" firmware variant
- Any non-"OLED" board with 16MB of flash or greater using an attached ILI9341-driven TFT should select the "TFT" firmware variant
