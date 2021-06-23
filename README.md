# FuckIO
An opensource fucking machine, that is far superiour to anything on the market.
![Rendering](/images/CAD-Screen.png)

## Specs
* Easy to built just with a 3D-printer, screwdrivers and easy-to-source parts.
* Completely 3D-printable with easy to print parts and almost no supports. Also avoiding tight tolerances to make this accessable as possible. 
* Simple, yet verastile mechanics.
* Based on a linear position drive, not a crank mechanism.
* Double 24V PWM output to drive external systems. Could be lube pumps, vibrators, etc.

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

## Control
PCB, Firmware and Software are under heavy development. For now I recommend going with the OSSM board and firmware: https://github.com/KinkyMakers/OSSM-hardware

