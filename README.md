Arduino Light-Tracking System

An embedded system built with an Arduino Uno that detects the direction of maximum light intensity and automatically rotates a servo motor to align with it. The system uses a photoresistor mounted on a rotating arm to scan light levels and dynamically adjust positioning.

Overview

The servo performs a 180° sweep to collect real-time light intensity data. The system determines the angle with the highest light level and repositions the arm accordingly. Users can modify scan delays and sensitivity thresholds at runtime via serial input, and reset the system using a push button.

Features

180° servo sweep for light detection

Automatic alignment to strongest light source

Runtime configuration via serial communication

User-defined sensitivity and scan delay

Push-button reset support

Real-time serial logging for debugging

Hardware Used

Arduino Uno

Servo motor

Photoresistor (LDR)

Push button

Resistors

Breadboard and jumper wires

Software

Arduino IDE

C/C++ (Arduino framework)

Applications

Solar tracking systems

Light-following robotics

Embedded sensing and control projects

How It Works

Servo sweeps from 0° to 180° collecting light readings

Maximum light intensity and corresponding angle are calculated

Servo rotates to the optimal angle

System adapts to user-defined sensitivity and environmental changes
