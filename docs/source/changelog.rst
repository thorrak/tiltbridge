Changelog
#########


v1.0.0 - XXX XX, 2021 - UI Overhaul, Scanner Overhaul, and loads more
---------------------------

This is a significant release, and is the result of a large number of changes over the past few weeks. To note - this
update touched virtually every line of code in some fashion. I cannot express enough gratitude towards the primary
authors of this update - @lbussy and @pletch.

- Refactored large chunks of code for performance
- Considerable web UI improvements
- Converted from synchronous to asynchronous HTTP server
- Converted to use ticker-based callbacks/timers for data sending
- Removed nlohmann JSON library in favor of ArduinoJson
- Updated to latest BLE library (thanks @h2zero!)
- Switched TFT libraries, and combined eSPI and full-size TFT code
- Updated TiltBridge logo on OLED screens



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