.. include:: ../global.rst

Cloud Service Setup
===================

Once your tilt has completed the :doc:`initial_setup` process it is ready to begin scanning for `Tilt Hydrometers`_ and
relay their gravity readings to the cloud. At the moment, TiltBridge supports six cloud service targets for this data:

* `Fermentrack`_
* `BrewPi Remix`_
* `Brewers Friend`_
* Google Sheets
* `Brewfather`_
* `BrewStatus`_


Setting up TiltBridge for Fermentrack
-------------------------------------

The TiltBridge was designed by the primary author of `Fermentrack`_ and is designed with Fermentrack support in mind.
In most cases, your TiltBridge can be linked to a single Fermentrack installation from directly within the Fermentrack
app as part of the "Add Gravity Sensor" workflow. The configuration can also be re-sent to TiltBridge by clicking the
"Update TiltBridge Automatically" button from the "Manage Sensor" page for the Tilt Hydrometer linked to the TiltBridge.

Adding the TiltBridge to Fermentrack
************************************

#. Connect to your Fermentrack installation and log in (if necessary)
#. From the Device menu choose ``Add Gravity Sensor``
#. On the resulting page, choose ``Tilt Hydrometer`` and then select the connection type ``TiltBridge``
#. Click the ``Add New TiltBridge`` link. Fermentrack will automatically scan for TiltBridge devices on your network.
#. If your TiltBridge device appears, click the "Add" button. Fermentrack will attempt to complete the remaining setup steps automatically. Proceed to Adding Tiltbridge-connected Tilts to Fermentrack
#. If your TiltBridge device **is not** in the list you will need to add it manually. In the form at the bottom of the page, enter the mDNS ID you selected during :doc:`initial_setup` and enter a name by which Fermentrack will identify your TiltBridge.
#. Click the `Create TiltBridge` button. Fermentrack now knows to expect your TiltBridge, but you will need to finish the setup on the TiltBridge manually.


If you need to manually configure the TiltBridge
################################################

If Fermentrack was not able to automatically configure your TiltBridge (either because the automatic configuration failed
or because you created the TiltBridge manually) you will need to manually set the Fermentrack URL on the TiltBridge
settings page.

#. Identify the IP address of your Fermentrack installation
#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. In the ``Local Target Settings`` field enter http://<fermentrack_ip_address>/tiltbridge/ and click ``Update``

Fermentrack and TiltBridge are now configured to properly communicate. You can now set up Tilts connected using
TiltBridge in Fermentrack.


Adding TiltBridge-connected Tilts to Fermentrack
************************************************

Once the TiltBridge and Fermentrack are configured to communicate, you can add the Tilt hydrometers to Fermentrack
that will be pushed via the TiltBridge.

#. Connect to your Fermentrack installation and log in (if necessary)
#. From the Device menu choose ``Add Gravity Sensor``
#. On the resulting page, choose ``Tilt Hydrometer`` and then select the connection type ``TiltBridge``
#. Select the TiltBridge you just set up
#. Enter the name, temp format, and color, and click ``Create Sensor``

Congratulations - your Tilt will now send gravity readings to Fermentrack.



Setting up TiltBridge for BrewPi Remix
---------------------------------------

Tiltbridge will operate natively with `BrewPi Remix`_. Tiltbridge is configured to push to BrewPi Remix in the
same manner it may be manually configured to point to Fermentrack.

Adding the TiltBridge to BrewPi Remix
**************************************

#. Identify the IP address of your BrewPi Remix installation
#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. In the ``Local Target Settings`` field enter http://<brewpi_remix__ip_address>//brewpi-api.php and click ``Update``

BrewPi Remix and TiltBridge are now configured to properly communicate. You can log Tilts through TiltBridge in BrewPi Remix.

Note: If you are using multi-chamber mode you must update the URL accordingly, and you may (currently) only report one Tilt via Tiltbridge. If this impacts you negatively, you should definitely
log an `issue to the BrewPi Remix project <https://github.com/brewpi-remix/brewpi-tools-rmx/issues>`_.


Adding TiltBridge-connected Tilts to BrewPi Remix
**************************************************

Once the TiltBridge and BrewPi Remix are configured to communicate, you can add the Tilt hydrometers to BrewPi Remix
that will be pushed via the TiltBridge.

#. Connect to your Pi with ssh and login
#. Edit your config file (``sudo nano /home/brewpi/settings/config.cfg``)
#. Add a line at the bottom of this file for the color you would like to report (``tiltColor = Yellow``)
#. Save and exit (``Ctrl-X``, ``y``)
#. Restart the BrewPi Remix Daemon for the changes to take effect (``sudo systemctl restart brewpi``)

Congratulations - your Tilt will now send gravity readings to BrewPi Remix.


Setting up TiltBridge for Brewers Friend
----------------------------------------

With a premium `Brewers Friend`_ account, you can store temperature and gravity readings from your Tilt Hydrometer as
part of your brew logs. After you set up your TiltBridge to push to Brewers Friend, it will create stream devices for
each of the detected Tilts which can then be subsequently attached to your brew logs. To comply with rate limits
placed by Brewer's Friend, data is first sent about 1 minute after startup, and subsequently no more than once every
15 minutes.

.. note:: Due to ESP32 hardware limitations, all connections to Brewers Friend are done over HTTP


Configuring the TiltBridge for Brewers Friend
*********************************************

To configure TiltBridge to use Brewer's Friend, you will need to obtain your Brewer's Friend API key and provide it to
TiltBridge.

#. Go to `Brewers Friend`_ and log in to your account
#. Click the ``Profile`` button in the upper right, and click ``Account``
#. Copy the API key (not one of the URLs) to your clipboard
#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. In the ``Brewers Friend API Key`` field paste the API key you copied earlier and click ``Update``


Testing Brewers Friend
**********************

Once the Brewer's Friend API key is provided, the TiltBridge will begin transmitting gravity data once every 15 minutes.
Following the first data transmission, you can easily check in Brewer's Friend to see if the data was received.

#. Go to `Brewers Friend`_ and log in to your account
#. Click the ``Profile`` button in the upper right, and click ``Account``
#. Click the ``Device Settings`` button on the right
#. In the ``Device Settings`` pane click ``Show All`` (to the right of ``All Brew Sessions``)
#. All Tilt devices should appear in the list as ``Stream`` devices and will be identified by their color

.. note:: Per Brewer's Friend guidelines, data is only pushed once every 15 minutes.


Setting up TiltBridge for Google Sheets
---------------------------------------

Similar to TiltPi or the Tilt Hydrometer app, TiltBridge supports logging to Google Sheets. Setting up Google Sheets is
more involved than either `Fermentrack`_ or `Brewers Friend`_, but provides a free, easily accessible cloud data service.



Preparing Google Sheets to Receive Data
***************************************

The first step in Google Sheets integration is preparing Google Sheets to receive the data. Google Sheets requires the
use of a Gmail or Google Apps for Domains account. This preparation step only needs to be done once per Google Account
and once complete can be used with multiple Tilts, TiltBridges, and beer logs.

#. Open the `TiltBridge Master Sheet <https://docs.google.com/spreadsheets/d/1yyqzjzJ7M0LxvQuV-1U7GbdZ_w_vOXV1AR5nivVNVYI/>`_ on Google Sheets. If prompted to login, log into your Gmail or Google Apps for Domains account.
#. As prompted, go to the ``File`` Menu, and select ``Make a Copy``
#. Enter a name for this sheet (it will be the master sheet to coordinate all your beer logs) and click ``OK``
#. The copy you just made will open in a new window. Go to the ``Tools`` menu and select ``Script Editor``

This will then open the Google Script editor. Before you can begin logging to Google Sheets, you need to "publish" the
script that will receive the incoming data. To do so, you can follow these instructions (which are the same as those at
the top of the window that appears):

#. Go to the ``Publish`` menu and select ``Deploy as web app...``
#. In the dialog box that appears, set ``Who has access to the app:`` to ``Anyone, even anonymous`` and click ``Deploy``
#. A dialog box will appear. Select ``Review Permissions``. Another dialog box will appear. Select your Google Account.
#. A dialog box with ``This app isn't verified`` will appear. Select ``Advanced`` then select ``Go to Tilt Cloud Template for TiltBridge (unsafe)``
#. A dialog box with permission requests will appear. Select "Allow".
#. A dialog box confirming the app has been published will appear. Click ``OK``, then Close Script Editor and return to the Google Sheet.
#. Use the new "Tilt" menu in the Google Sheet to view the cloud URL or email it to yourself.


Configuring the TiltBridge for Google Sheets
********************************************

Once you have prepared Google Sheets to receive data pushed by the TiltBridge, you will need to update a handful of
settings on the TiltBridge itself so that it knows where to send the data.

#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. In the ``Google Script URL`` field paste the Google Script URL you made note of during the preparation step above and click ``Update``
#. After the URL is updated, enter your Gmail (or Google Apps) email address in the ``Google Script Email`` field and click ``Update``

Your TiltBridge should now be configured to send data to Google Sheets. To begin logging a Tilt you will need to enter a
sheet name for the data to be logged to.


Logging a Beer with TiltBridge and Google Sheets
************************************************

After configuring the TiltBridge each Tilt your TiltBridge detects can be individually logged to its own sheet on Google
Sheets. To enable logging, you will need to specify a sheet name in TiltBridge. If this sheet does not exist on Google
Sheets it will be created. If the sheet does exist, new data points will be appended to it.

#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. Type a name for the Google Sheet to which you want to log data points in the appropriate ``Tilt Sheet Name`` field and click ``Update``

Once this is complete, your Tilt will begin logging data points to the sheet name you specified. If the sheet does not
exist, it will automatically be created.

.. note:: Points are only pushed to Google Sheets once every 10 minutes. As a result, it may take up to 10 minutes for the sheet to be created or for the first points to appear.



Setting up TiltBridge for Brewfather
------------------------------------

With a premium `Brewfather`_ account, you can store temperature and gravity readings from your Tilt Hydrometer as
part of your brew logs.

Configuring your TiltBridge for Brewfather
******************************************

TiltBridge will need your Brewfather stream ID in order to post to your Brewfather account.

#. Go to `Brewfather`_ and log in to your account
#. Click the ``Settings`` option in the menu on the left
#. Under "Power-ups" in the lower left corner, click the "switch" next to "Custom Stream" if it is not already toggled
#. Copy just the string of letters/numbers that appears after the start of the URL (http://log.brewfather.net/stream?id=) to your clipboard
#. On a device connected to the same network as the TiltBridge, navigate to http://tiltbridge.local/ (replace tiltbridge in this URL with the mDNS name you set during initial setup)
#. Click the ``Settings`` link at the top of the dashboard
#. In the ``Brewfather Stream key`` field paste the string you copied earlier and click ``Update``


Testing Brewfather
**********************

Once the Brewfather Stream key is provided, the TiltBridge will begin transmitting gravity data once every 15 minutes.
Following the first data transmission, you can easily check in Brewfather to see if the data was received.

#. Go to `Brewfather`_ and log in to your account
#. Click the ``Devices`` option in the menu on the left
#. All Tilt devices should appear in the list as ``Custom Stream`` devices and will be identified by their color

.. note:: Per Brewfather's guidelines, data is only pushed once every 15 minutes.



Setting up TiltBridge for Brewstatus
------------------------------------

TiltBridge is designed to allow for data to be pushed to `Brewstatus`_. This functionality was helpfully added by
contributors to the project on `GitHub`_ and has not yet been documented.

.. todo:: Document Brewstatus configuration


