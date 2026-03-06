# TiltBridge

[![TiltBridge Logo](http://www.tiltbridge.com/static/img/tiltbridge_logo.png "TiltBridge")](http://www.tiltbridge.com/)

[![Documentation Status](https://readthedocs.org/projects/tiltbridge/badge/?version=master)](http://tiltbridge.readthedocs.io/en/master/?badge=master)
                
#### Bring WiFi to your Tilt Hydrometer

TiltBridge is a single-component solution for bridging the gap between your Tilt Hydrometer's Bluetooth signal and the internet. Eliminate the need for a spare phone/tablet to live next to your fermenter without losing the ability to log your gravity data.

#### Log to the Cloud

TiltBridge automatically logs your data to the following cloud data services:

* [Fermentrack](https://www.fermentrack.com/)
* [BrewPi Remix](https://www.brewpiremix.com)
* [Brewer's Friend](http://www.brewersfriend.com/)
* [Brewfather](https://brewfather.app)
* Google Sheets
* [Brewstatus](https://brewstat.us)
* [Grainfather](https://community.grainfather.com/)
* [Taplist.io](https://taplist.io)
* [MQTT](https://mqtt.org/) Broker
* InfluxDB v2.x

#### Features

* One-glance Gravity Readings
* Gravity Web Dashboard
* Log multiple Tilts at once
* Log to multiple cloud services simultaneously
* Single component build


#### Build a TiltBridge

Building a TiltBridge is simple - the hardest decision in most cases is the enclosure. For more information, read the [documentation](http://docs.tiltbridge.com/).

Once you've acquired your hardware, simply flash the TiltBridge firmware with [BrewFlasher](http://www.brewflasher.com/). Easy!


### Officially Supported Hardware

There are currently three officially supported hardware sets:

* TFT or "Large Screen" Build - D32 Pro + D32 LCD Screen
* "Easy" Build - M5 Stick C Plus
* "Cheap" Build - Headless (any ESP32)


### Unofficially Supported Hardware

There are also a number of _unofficially_ supported hardware sets. These sets are not tested prior to release and are not guaranteed to work with future releases. Some are older hardware that are no longer recomended, some have been contributed by members of the community. 

* LCD1306 "OLED" Boards
* ESP32 /w ESPI Display
* ESP32-S3 T-Display
* "Cheap Yellow Display" (CYD) ESP32


#### Requirements

TiltBridge requires an ESP32-based controller properly flashed with the TiltBridge firmware.


### Support TiltBridge

Interested in supporting TiltBridge? Buy a [sticker](https://www.tindie.com/products/thorrak/tiltbridge-sticker/) (or a [magnet](https://www.tindie.com/products/thorrak/tiltbridge-magnet/))!



