.. include:: ../global.rst

Initial Setup
=============

Once you have acquired the necessary hardware, you are ready to flash the TiltBridge firmware to your controller and set
it up for use on your network.

Flashing the TiltBridge Firmware
--------------------------------

The TiltBridge firmware can downloaded from `GitHub`_ and flashed to your controller using either `BrewFlasher`_,
`Fermentrack`_, or your favorite ESP32 flashing software. The easiest method to flash the firmware is using `BrewFlasher`_ -
a desktop firmware flashing application maintained by the author of TiltBridge.

.. note:: Be sure to flash the right version of the firmware. For new builds using the LOLIN D32 Pro you will want the "TFT" firmware. For builds without a screen you will want to use the "OLED" firmware.



Setting up TiltBridge
---------------------

Once you have flashed the firmware to your ESP32, secured it in its enclosure, and powered it on, you are ready to connect
it to your WiFi network. After the TiltBridge is powered on it will create a new WiFi access point with the SSID ``TiltBridgeAP``
and password ``tiltbridge``. Connect to this AP using a phone, laptop, or other WiFi enabled device, and attempt to
navigate to any website using a web browser in order to see the configuration panel.

#. Flash & power on the TiltBridge ESP32
#. Using a WiFi-enabled phone, laptop, or other device, connect to the wireless network with the SSID ``TiltBridgeAP``
#. Once connected, the configuration screen may automatically appear. If it does not, open a web browser and attempt to navigate to any webpage.
#. When the configuration screen appears, click ``Configure WiFi``
#. Select your wireless network from the list of available networks and enter the password in the ``Password`` field beneath the list of available networks
#. Choose a name to identify your TiltBridge and enter it in the ``Device (mDNS) Name`` field. The default name - ``tiltbridge`` can be used unless you have multiple TiltBridge devices in which case each must have a unique name.
#. Click ``Save``
#. Reconnect your phone/laptop/etc to your regular wireless network

Once these steps are complete, your TiltBridge should connect to your wireless network and is ready to begin transmitting
your Tilt's data over WiFi.
