# Garage Door Opener

Object-oriented embedded garage door opener in C/C++ with motor calibration, limit switches, rotary encoder feedback, EEPROM persistence, and MQTT-based remote control.

## Overview
This project implements a fully functional embedded system for controlling a garage door. It supports both local operation and remote control via MQTT, while maintaining persistent state across restarts.

The system is designed with modular components to separate hardware control, state management, and communication.

## Key Features
- Motor control with calibration and limit switches
- Rotary encoder for position tracking
- Persistent state storage using EEPROM
- Local control via physical buttons
- Remote monitoring and control via MQTT
- Error detection and safe state handling
- Modular architecture with clear separation of concerns

## Architecture
The system is divided into multiple components:
- `StateMachine` – controls system states and transitions
- `StepMotor` – handles motor movement
- `LimitSwitch` – detects physical boundaries
- `RotaryEncoder` – tracks door position
- `MqttService` – handles remote communication
- `Eeprom` – manages persistent storage
- `ButtonController` – handles user input

## Technologies
- C / C++
- Embedded systems programming
- MQTT protocol
- EEPROM persistence

## Notes
This project focuses on reliability, modular design, and real-world embedded system constraints.
