.. include:: ../global.rst

Notes for Developers
====================

Interested in extending TiltBridge or integrating it into your project? Awesome!

While TiltBridge was developed with a certain feature set in mind, TiltBridge is open source for a reason. There are
plenty of features that I couldn't imagine which would make the project even better than it is. That said, prior to
developing a new feature and submitting a pull request, I strongly recommend reaching out. Due to the current state of
the Arduino framework for ESP32, TiltBridge currently uses 99.2% of the *expanded* program partition as well as the
majority of the ESP32's available RAM and as such new features are likely to face considerable space constraints. My
hope is that as the ESP32's Bluetooth drivers are optimized, both the RAM usage and the size of the compiled binaries
will shrink allowing for additional code to be added.


