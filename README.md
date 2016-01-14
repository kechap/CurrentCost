# CurrentCost

This sketch is for the CurrentCost Bridge device that causes readings to be published over HTTP using JSON.

![alt tag](http://www.pems.ie/Photo's/Current%20Cost%20Bridge.JPG)

Sending interval is set to 5 minutes and the value is an average of all packets arrived to the bridge.

You will need a programmer to upload this sketch to your device. I am using a USBtinyISPv2 but you can use every programmer that is listed in the arduino IDE.

## Customize

You will need to specify the `SERVER_HOST_NAME` and `uuid`.

## Installation
This must be compiled with the board set to: `Arduino Pro or Pro Mini (3.3V, 8MHz) w/ ATmega328`

Plug the ISP into the 6-pin header on the bridge board. If you have the ISP set to power the board make sure you haven't got the bridge's own power supply plugged in.
From the Arduino IDE, burn the bootloader. To do this, first ensure 

`Tools->Board->Arduino Pro or Pro Mini (3.3V, 8 MHz) w/ ATmega328` 

is selected. Make sure you select the correct type of programmer `Tools->Programmer`.

After that just press `CTRL+SHIFT+U`(or `Sketch->Upload Using Programmer`) to upload using the programmer.

## Consuming the data
The bridge will make a PUT request to the specified url with a payload like the following.

`[
  {"id": "0", "value":"420"},
  {"id": "1", "value":"80"},
  {"id": "10", "value":"25.4"}
]`
