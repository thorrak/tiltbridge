Changelog
#########


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