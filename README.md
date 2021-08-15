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
* ESP32 DevKitC V4 board
* custom FuckIO PCB plus parts

## Control
### Electronics
A small PCB provides some interface electronics like level shifting and a 5V supply for the ESP32 DevKitC V4 board. It's all through hole and easy to hand solder. There is only one button to force the ESP32 in AP mode should on forget the password.

### Firmware
The firmware uses the following projects:
* IotWebConf for WiFi provisioning: https://github.com/prampec/IotWebConf. Check the [end-user manual](https://github.com/prampec/IotWebConf/blob/master/doc/UsersManual.md) on how to use it.
* MQTT for communication: https://github.com/256dpi/arduino-mqtt Albeit SSL is not supported.
* My own library for generating the stroking motion: https://github.com/theelims/StrokeEngine/
* Which in turn uses https://github.com/gin66/FastAccelStepper for step generation and interfacing the servo motor.

### node-red
I've setup a node-red instance on my NAS to control FuckIO through MQTT messages. Have a look at the provided [flows.json](node-red/flows.json) to draw some inspiration from.

## ToDo's & Known Issues
There are still a few open items and kinks to iron out:
* The iHSV57 can draw more power then the Meanwell power brick can supply. In high load conditions the power brick goes into failure mode. It needs a couple of minutes to recover. I need to look into tuning the servo.
* Very hard decerelations may trip the over-voltage protection of the power brick. The motor's induction can raise the voltage quite drastically. The power brick will recover after a couple of minutes. Will be adressed in a next PCB revision with a protection circuit. For now you may tone down a little bit on the maximum acceleration. 

