TiltBridge Hardware
===================


The first step in a TiltBridge installation is sourcing the necessary hardware. The only component required for a
TiltBridge is an ESP32-based board - preferably with a "boot" switch and an OLED screen. Boards that have been tested
with TiltBridge include the following:

.. note:: While this page documents hardware that has ben demonstrated to work with TiltBridge in the past, the author of TiltBridge does not warrant that any of the hardware listed below continues to be operational. Additionally, although links to vendors are provided to show examples of the referenced hardware, no guarantees are made as to the vendors themselves. Buy hardware you trust from a vendor you trust.


"TTGO" OLED Board
-----------------

Although the full "TTGO" board includes features we do not need like LoRa radios, there are a handful of boards in the
"TTGO" style that only have the OLED screen. These boards are very small, and have the advantage of having a screen that
is secured to the PCB itself. These boards run hot, but as long as the case has some ventilation they should be fine. An
optional heatsink can be added. These boards do not have any mounting holes which requires that they be secured by other
means.

For these boards, pin 16 is required to be set high to power the board, the address is 0x3c, SDA is 4, and SCK is 15.
TiltBridge will automatically detect boards with this configuration and enable the OLED screen accordingly.


Sources
*******

- `AliExpress <https://www.aliexpress.com/item/Lolin-ESP32-OLED-V2-0-Pro-ESP32-OLED-wemos-pour-Arduino-ESP32-OLED-WiFi-Modules-Bluetooth/32822105291.html>`_


Enclosures
**********

- `Sample Case on Thingiverse <https://www.thingiverse.com/thing:3515836>`_

.. todo:: Check this link


ESP32 "OLED" Board
------------------

There are a number of ESP32 boards available in the market which come with an attached OLED screen. TiltBridge works with the
"side-by-side" configuration (where the OLED screen is mounted on the face of the PCB to the right of an ESP32 enclosed
in a metal shield). These boards have mounting holes, but the OLED screens are only glued to the front of the PCB which
means that they can be misaligned along the Y axis.

For these boards the address is 0x3c, SDA is 5, and SCK is 4. TiltBridge will automatically detect boards with this
configuration and enable the OLED screen accordingly.


Sources
*******

- `AliExpress <https://www.aliexpress.com/item/ESP32-OLED-Wemos-WiFi-Module-Bluetooth-Dual-ESP-32-ESP-32S-ESP8266-OLED-For-Arduino/32896625954.html>`_


Enclosures
**********

- `Sample Case on Thingiverse <https://www.thingiverse.com/thing:3515836>`_

.. todo:: Fix this link


Lolin D32
---------

The Lolin D32 is an ESP32 board the author of TiltBridge uses in other projects not requiring OLED screens. It comes in
a relatively compact form factor and as of the time of writing is readily available from Lolin. The board does not have
an attached OLED screen, however, and as such requires either a "headless" configuration or the use of an additional
PCB to add OLED support. If an additional PCB is used, TiltBridge expects the screen to have an address of 0x3c,
with SDA on 21 and SCK on 22.

Sources
*******

- `AliExpress <https://www.aliexpress.com/item/WEMOS-LOLIN32-V1-0-0-wifi-bluetooth-board-based-ESP-32-4MB-FLASH/32808551116.html>`_


Enclosures
**********

Due to the lack of an attached OLED screen no stock enclosure design is available for the Lolin D32 on Thingiverse.
If you've designed one, please reach out to the author or submit a pull request to have your case design linked here.



Other Boards
------------

TiltBridge is designed to operate without an attached OLED screen, and as such most other ESP32 boards should successfully
run the firmware. If you choose to connect an OLED screen to a different board, it is recommended that you use a screen
which has an address of 0x3c and use pin 21 for SDA and pin 22 for SCK.


Additional Hardware Required
----------------------------

In addition to the ESP32 board, you will also need a USB power source to power the ESP32 (such as a phone charger), a
MicroUSB cable for flashing, and you will likely want some kind of enclosure.


Once you have acquired the necessary hardware, you are ready to begin :doc:`User Guide/initial_setup`