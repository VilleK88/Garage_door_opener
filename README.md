Project specifications
Door opener must have the following functionality:
• Calibration: Pressing SW0 and SW2 at the same time starts calibration. During calibration
the door is run up and down and the number of steps between up and down position is
determined using the limit switches and the rotary encoder to detect when the door stops at
both ends. It is safe to run the motor against the limit switch body during calibration, but a
properly implemented program should stop the motor when the switch is closed but the
block does not yet hit the body of the switch. After calibration the door may not hit the body
of either limit switch during normal operation.
• Status reporting: Program reports the following status through MQTT
o Door state: Open, Closed, In between
o Error state: Normal, Door stuck
o Calibration state: Calibrated/Not calibrated
• Local operation: Pressing SW1 has following functionality:
o Door is closed → door starts to open
o Door open → door starts to close
o Door is current opening or closing → door stops
o Door was earlier stopped by pressing the button → door starts movement to the
opposite direction (stopped during opening→close and vice versa)
• If the door gets stuck during opening/closing the motor is stopped, error state is reported,
and door goes to not calibrated state.
• Opening button may not work in the non-calibrated state.
• Remote control may not work int the non-calibrated state.
• Status must be indicated locally using LEDs. Error state must be indicated by blinking an LED.
Minimum requirements
The door can be operated locally, and calibration rules work as described earlier. Object orientation is
used in some parts of the program. Meeting minimum requirements gives you 50 points.
Advanced requirements
Program is implemented in object-oriented style.
Program preserves calibration and door state across power down or reboot.
Objects have clear responsibilities and reflect entities in the application/hardware.
Program sends status messages to an MQTT-server using topic “garage/door/status”.
Program subscribes to topic “garage/door/command” and executes the received commands.
Command responses are sent to topic “garage/door/response”. Response must contain the
command result if command was successfully executed or an error message if command can’t be
completed. Note that status of the door must also be reported to “garage/door/status” if the
command changes door status. User must be able to perform same operations through remote
commands as with the local (button) interface.
Additional remote features count towards higher grade provided that the requirements stated above
are met.
