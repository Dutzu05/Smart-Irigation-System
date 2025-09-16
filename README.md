# Smart-Irigation-System
This a rudimentary app made for myself to help watering my plants around the house automatically.
It involves electronics: ESP32 for WIFI compatibilies, DC pump, DC valves for directing the water, IRLZ44N MOSFETs, LM393 sensors for humidity resistors, 2 9V batteries connected together and LEDs. Look for the schematic
The .ino file must be runned in order to find out the IP address for the microcontroller. That will pe copied in the C# script
The sensors are used to send notifications about the soil's state
With the help of virtual buttons, water is well directionated to the requested plants
