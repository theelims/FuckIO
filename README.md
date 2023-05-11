# FuckIO
An opensource fucking machine, that is far superiour to anything on the market.
![Rendering](/CAD-Screen.png)

## Specs
* Easy to built just with a 3D-printer, screwdrivers and easy-to-source parts.
* Completely 3D-printable with easy to print parts and almost no supports. Also avoiding tight tolerances to make this accessable as possible. 
* Simple, yet verastile mechanics.
* Based on a linear position drive, not a crank mechanism.
* An 24V PWM output to drive external systems. Could be lube pumps, vibrators, etc.

## Mechanics
The mechanics is a direct derivative of an other open source project called OSSM. You can also find some informations about this build on their Discord.
https://github.com/KinkyMakers/OSSM-hardware

The motor mount is designed for a NEMA23 servo or stepper motor. Any motor fitting to this standart should work. 

### BOM
* iHSV57-30-18-36-... integrated servo motor from JMC
* 24V Power Brick Mean Well GST280A24-C6P 
* 40cm MGN12H linear rail
* 4x 605-2RS Z2 bearings
* Micro Switch
* GT2 20 Tooth Pulley for 8mm Axis and a 10mm belt
* GT2 belt with 10mm width
* Various M3 and M5 screws and nuts
* OSSM Ref Board from Research and Desire: https://shop.researchanddesire.com/collections/all/products/ossm-reference-board
* M5 Stack Remote Control from Ortlof: https://github.com/ortlof/OSSM-M5-Remote

## Control
### Firmware
The currently best firmware is the official OSSM firmware, or Ortlof's firmware for the M5 Remote https://github.com/ortlof/OSSM-M5-Remote

The firmware in the repository can only be used in conjunction with node-red.

The firmware uses the following projects:
* IotWebConf for WiFi provisioning: https://github.com/prampec/IotWebConf. Check the [end-user manual](https://github.com/prampec/IotWebConf/blob/master/doc/UsersManual.md) on how to use it.
* MQTT for communication: https://github.com/256dpi/arduino-mqtt Albeit SSL is not supported.
* My own library for generating the stroking motion: https://github.com/theelims/StrokeEngine/
* Which in turn uses https://github.com/gin66/FastAccelStepper for step generation and interfacing the servo motor.

### node-red
I've setup a node-red instance on my NAS to control FuckIO through MQTT messages. Have a look at the provided [flows.json](node-red/flows.json) to draw some inspiration from.

