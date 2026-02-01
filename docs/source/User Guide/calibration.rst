.. include:: ../global.rst

Calibration, Temperature Correction, and Gravity Smoothing
==========================================================

Although the values as sent by the Tilt Hydrometer are reasonably accurate out-of-the-box, you may find that you want to
calibrate your Tilt Hydrometer's gravity/temperature readings to match known values - or that the Tilt is subject to
momentary fluctuations in readings due to bubbling, splashing, or other factors. TiltBridge supports both calibration
and smoothing of gravity readings to help you get the most accurate readings possible. Smoothing is enabled by default,
but can be configured or disabled in the TiltBridge settings portal if desired. Calibration allows you to determine a
calibration equation that can be applied to the gravity readings sent by the Tilt Hydrometer by entering a list of 
known gravity values and their corresponding Tilt Hydrometer readings.

Once a calibration equation has been entered, it is applied to all readings for a given Tilt displayed or sent by 
the TiltBridge.


.. todo:: Add a section on temperature correction


Calibration
-----------

Calibration allows you to adjust the gravity readings sent by your Tilt Hydrometer via an equation. The calibration
equation can be determined by performing a regresssion on a set of known gravity values and their corresponding
Tilt Hydrometer readings, or can be directly entered if known (i.e. if you have already performed the regression).

To access the calibration panel, simply click the icon next to the Tilt you want to calibrate from the list of Tilts
in the TiltBridge web interface. 


Calibrating with Data Points
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TiltBridge can calculate the calibration equation for you by performing a regression on a set of gravity readings. 
In the calibration panel, you can see the existing calibration equation (if any) and any previously entered data 
points. To add a new data point, click the "Add Calibration Point" button. A dialog will pop up where you can enter 
the known gravity value and the corresponding Tilt Hydrometer reading. Once you have entered the data point, click 
the "Save" button to save it. Repeat for as many data points as you have available.

Once data points have been entered, you can select the degree of polynomial regression you want to perform. The choices
are constant offset, linear (1st degree), and quadratic (2nd degree). A graph will appear showing the entered data points
and a line corresponding to the equation derived from the selected regression. If you are satisfied with the calibration
equation, click "Save Equation".


Entering a Calibration Equation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you happen to know the exact equation that you want to use to calibrate your Tilt Hydrometer, you can enter it
directly into the TiltBridge settings portal. To enter the equation directly, click the icon on the Tilt List to
go to the calibration panel, and then click on the text in the "New Calibration Equation" field. A box will pop up
where you can enter the equation coefficients.

Once you have entered the appropriate coefficients, click the "Save" button to save the calibration equation to
the TiltBridge.



Gravity Smoothing
-----------------

Gravity smoothing is a feature that averages the gravity readings sent by the Tilt Hydrometer over a period of time 
using an exponential smoothing algorithm to reduce the impact of momentary fluctuations in readings due to bubbling, 
splashing, or other factors. This feature is enabled by default, but can be configured or disabled in the TiltBridge 
settings portal.

To disable the feature, click "Configure" from the left bar in the TiltBridge web portal and change the "Specific 
Gravity Averaging" setting to 0. 


