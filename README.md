# CS2220 Smart Access Control System
Project: cs2220projectSmartAccessSystem  
IDE: Arduino IDE 2.3.6  
Platform: Arduino UNO R3 (Elegoo Kit)  

## Overview

This project was created for the CS2220 course to demonstrate a functional smart access control system using digital logic and embedded system principles. The system simulates a secure access point controlled by a randomly generated 4-digit PIN. All input is taken through a 4×4 matrix keypad, and feedback is provided using a servo motor, buzzer, and LCD1602 display module.

The project focuses on modular hardware-software interaction. Each startup generates a new PIN, briefly displays it on the LCD, and waits for user entry. If the correct code is entered, the servo rotates to unlock; otherwise, the buzzer alerts the user and tracks the number of failed attempts.

## Hardware Components

- Arduino UNO R3 (Elegoo)
- 4×4 Matrix Keypad
- LCD1602 Display with Pin Header
- 10kΩ Potentiometer (LCD contrast)
- Servo Motor (9g micro)
- Buzzer
- Breadboard and Jumper Wires
- USB Cable for power and upload

## Functional Description

- On boot, the system generates a 4-digit PIN.
- The PIN is displayed briefly on the LCD and then cleared.
- The user inputs the PIN using the 4×4 keypad.
- Each digit typed is optionally shown or hidden on the LCD.
- If the entered PIN matches the generated one:
  - The servo rotates to simulate unlocking.
  - A short buzzer tone confirms access.
- If the PIN is incorrect:
  - The buzzer sounds a distinct error tone.
  - The retry count is incremented.
- After three failed attempts, the system locks the user out and displays a denial message.

## Code Structure

- Written in C++ using Arduino libraries.
- Main file: `cs2220projectSmartAccessSystem.ino`
- Uses libraries:
  - `Keypad.h` (for 4x4 keypad input)
  - `LiquidCrystal_I2C.h` or `LiquidCrystal.h` (depending on LCD version)
  - `Servo.h` (for door simulation)
- Modular functions include:
  - `generatePin()`
  - `getKeyInput()`
  - `verifyPin()`
  - `lockoutHandler()`
  - `displayFeedback()`

## Design Considerations

- LCD display is initialized using I2C or parallel depending on header type.
- Buzzer provides auditory signals distinct for success and failure.
- System is fully reset upon Arduino reboot, ensuring fresh security state.
- No EEPROM is used; PIN is volatile.

## Usage Notes

- Ensure the potentiometer is adjusted to display LCD text clearly.
- When uploading, use Arduino IDE 2.3.6 to ensure compatibility.
- All components are mounted on a breadboard and powered via USB.

## Folder Contents

- `cs2220projectSmartAccessSystem.ino` — Arduino sketch (source code)
- `README.md` — This documentation file


## Author

**Oluwatofunmi Samuel Badejo**  
CS2220: Computer Hardware  
Plymouth State University
