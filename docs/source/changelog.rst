Changelog
#########


v1.1.1 - Feb 18, 2022 - TiltBridge Cloud Support
------------------------------------------------

- Increased MQTT client size to support 1.1.0 additions
- Fix Home Assistant MQTT autodiscovery (Thanks @pletch)
- Add TiltBridge Cloud support (Thanks @djmarlow)
- Change to using 4MB partition scheme for non-TFT firmware


v1.1.0 - Jan 12, 2022 - Grainfather & Taplistio Support
-------------------------------------------------------

- Added Grainfather as a push target (Thanks @teanooki!)
- Added Taplistio as a push target (Thanks @TaplistIO!)
- Fixed typo in Brewfather log messages
- Added placeholder page when waiting for the first GSheets push
- Added 24 hour reboot timer to improve overall stability
- Shorten logo display time by 60%
- Added reboot if reconnecting to WiFi fails after some time
- Send 'uniq_id' as part of MQTT config messages
- Change MQTT keep alive timing to seconds to match MQTT spec
- Replaced WiFi management libraries & WiFi reconnection process
- Refactored configuration storage backend for performance
- Changed "Invert TFT" option to also work for OLED screens


v1.0.2 - Feb 19, 2021 - Calibration Bugfixes
--------------------------------------------

- Fixed bugs in calibration workflow
- Display calibration functions on calibration page


v1.0.1 - Feb 16, 2021 - Bugfixes & Configuration Tweaks
-------------------------------------------------------

- Rewrite code for storing/loading Google Sheets configuration
- Fix issue that can cause Google Sheets posts to periodically fail
- Corrected delay timers for Google Sheets, Brewer's Friend, and BrewFather
- Other behind-the-scenes cleanup


v1.0.0 - Feb 12, 2021 - UI Overhaul, Scanner Overhaul, and loads more
---------------------------------------------------------------------

This is a significant release, and is the result of a large number of changes over the past few weeks. Of note - this
update touched virtually every line of code in some fashion (300+ commits!). I cannot express enough gratitude towards
the primary authors of this update - @lbussy and @pletch.

- Considerable user interface improvements
- Added "Weeks on Battery", Tilt Pro detection, and signal strength to dashboard
- Refactored large chunks of code for performance
- Converted from synchronous to asynchronous HTTP server
- Converted to use ticker-based timers for data sending
- Removed nlohmann JSON library in favor of ArduinoJson
- Updated to latest BLE library (thanks @h2zero!)
- Switched TFT libraries, and combined eSPI and full-size TFT code
- Updated TiltBridge logo on OLED screens
- Partially automated update of version information
- Added uptime/last crash statistics to "About" page
- Added "Factory Reset" option which removes WiFi configuration and push target settings
- Changed: Multiple endpoints for settings updates
- Fixed bug where configuration AP could drown out other devices on nearby channels
- Added note to TFT at first boot about screen rotation being configurable



v0.2.3 - Dec 28, 2020 - Performance Tweaks & Home Assistant Support
-------------------------------------------------------------------

- Added specific MQTT subtopics to support Home Assistant (Thanks Pletch/Kidmock!)
- Added Google Sheets timezone support (Thanks Pletch!)
- Fix edge case involving two Tilts of the same color, but of different hardware versions (Thanks Pletch!)
- Fix race condition involving mDNS updates (Thanks Pletch!)
- Clean up favicon links (Thanks Pletch!)


v0.2.2 - Dec 22, 2020 - MQTT and Tilt Pro
-----------------------------------------

- Added support for Tilt Pro devices
- Added MQTT support (Thanks Pletch!)
- Add gravity smoothing support - except when sending to Fermentrack/BrewPiRemix (Thanks Pletch!)
- Add toggleable Celsius support (Thanks ricnewton!)
- Add support for rotated (e.g. Lolin v1.1.0) TFT Screens (Thanks Pletch!)
- Numerous bugfixes/cleanups (Thanks Pletch!)
- Properly capture/filter the "version" info sent on wake by v3 Tilts
- Added version information to about.htm
- Results are now expired after 5 minutes if no signal is received from a Tilt
- Renamed Fermentrack support to indicate it also works for BrewPi Remix (Thanks lbussy!)
- Changing the mDNS ID now resets the controller
- Update to latest version of Bluetooth library



v0.2 - June 19, 2020 - Brewfather, Brewstatus, and Secure GScripts Support
--------------------------------------------------------------------------

- Adds support for communicating with Google Scripts directly over HTTPS (no more proxy!)
- Adds Brewfather support
- Adds Brewstatus support
- Refactored Bluetooth stack
- Removed OTA update support



v0.1 - Mar 31, 2019 - Initial Release
-------------------------------------

- Initial release of TiltBridge!