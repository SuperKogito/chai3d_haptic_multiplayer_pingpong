# chai3d_haptic_multiplayer_pingpong
Implementation of a multiplayer (TCP / UDP) pingpong game played with haptic device (Novint Falcon).

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Prerequisites

In order to excute this code you will need the following packages:

CHAID library
http://www.chai3d.org/download/releases

## Tests

This code has been only tested on windows 7 using visual studio 2011

## Deployment

*To run this code you need to start the server file first
*Next change the ip in the client'script to the server ip
*Run the clients, move the device's handles

## Built With

* [VS11](https://www.microsoft.com/de-de/download/details.aspx?id=26830)

## Contributing

If you have any improvement's suggestions, please contact me.

## Remarks

This code is aimed essentially as a testing for sockets with chai3d. It can be used to show the the superior performance delivered by udp sockets comparing to tcp when it comes to haptic features. The performance provided by these 2 examples is unfornately buggy and relatively slow. A better alternative is the use multi-threading.

## License

Please refer to CHAI3D license:
http://www.chai3d.org/download/license

## Acknowledgments

* LMT chair at the TUM
* various online ressources
