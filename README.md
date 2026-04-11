## Garage Door Opener  

## Project Specifications

### Core Functionality

#### Calibration
- Calibration is initiated by pressing SW0 and SW2 simultaneously.
- During calibration:
  - The door moves fully up and down.
  - The total number of steps is measured using:
    - Limit switches
    - Rotary encoder
  - The system detects when the door reaches both end positions.
- It is mechanically safe to run the motor against the limit switch body during calibration, but:
  - A correct implementation should stop the motor as soon as the switch is triggered, before physical impact.
- After calibration:
  - The door must not hit the limit switch body during normal operation.

#### Status Reporting (MQTT)
The system reports its status via MQTT:

- Door State:
  - Open
  - Closed
  - In between

- Error State:
  - Normal
  - Door stuck

- Calibration State:
  - Calibrated
  - Not calibrated

#### Local Operation (SW1)
Button SW1 controls the door as follows:

- If the door is closed → starts opening
- If the door is open → starts closing
- If the door is moving → stops
- If the door was previously stopped manually:
  - Movement resumes in the opposite direction

#### Safety Behavior
- If the door becomes stuck during movement:
  - The motor is stopped immediately
  - Error state is reported
  - System resets to Not calibrated

- In non-calibrated state:
  - Local operation is disabled
  - Remote control is disabled

#### LED Status Indication
- System status is indicated using LEDs
- Error state is indicated by a blinking LED

## Minimum Requirements
- Door can be operated locally
- Calibration behaves as specified
- Program uses object-oriented design in some parts

Meeting minimum requirements gives 50 points.

## Advanced Requirements
- Fully object-oriented implementation
- Calibration and door state are preserved across reboot/power loss
- Clear object structure reflecting hardware/components

### MQTT Integration

- Publish (status):
  garage/door/status

- Subscribe (commands):
  garage/door/command

- Publish (responses):
  garage/door/response

### Remote Control Features
- Execute commands received via MQTT
- Send response messages:
  - Success → command result
  - Failure → error message
- If a command changes door state:
  - Updated state must also be published to garage/door/status
- Remote control must support the same functionality as local operation

## Additional Features
Additional remote features can improve grading, provided all requirements above are met.
