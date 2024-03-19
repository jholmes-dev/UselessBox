# Useless Box

This is a Useless Box that flips multiple switches instead of just one. It uses a stepper motor & A4988 driver to move the arm, and two servos: one for the arm, and one for the lid.

The box is controlled by an ATMEGA328P.

In Action:
https://i.imgur.com/oVlNI0r.mp4

3D Models:
https://www.thingiverse.com/thing:2996661

Schematic:
https://easyeda.com/coffeman500/useless-box

Parts:

- Arduino Uno
- L7805 & L7806
- Arm Servo
- Door (Lid) Servo
- Stepper Motor Belt
- A4988 Stepper Motor Controller
- Stepper Motor
- 5cm x 7cm Protoboard
- Pulley Wheel
- Female Power Plug
- Power Adapter
- Home Switch
- Switches
- Box Hinges
- Assorted m3 & m4 nuts & bolts

## Description

The program uses switch interrupts to watch the 3 switches. When flipped the interrupt fires (Line 62-117) and adds any switched pins to the linked list (Line 37-44). 

The main loop then watches the linked list, and goes to turn off any nodes added as they appear, moving down the list as necessary (159-176). 

There's also a fallback method to just hit any switches that are HIGH in case the linked list fails to capture a trigger (179-192).

When the program becomes idle (no switches are available to hit) it resets back to a default state by moving the platform all the way left until the lever switch (Home Switch) is hit, and resetting the arm & lid (Door) servo positions (245-274).
