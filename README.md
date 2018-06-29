# Swissloop NAP

This Arduino sketch approximates our expectation of what SpaceX's code will do on the NAP.

Both tape sensors are treated independently. Whenever one of the tape sensors triggers a message is printed to serial (**9600 baud**) showing the number of detected tapes, distance and velocity for both sensors. If the Ethernet shield is attached, then the message is also sent over the network in a UDP package to **192.168.0.7:1338**.

You can view the received messages with netcat for example:

```bash
nc -u -l 1338
```

If an SD card is inserted into the Ethernet shield, the data is also logged to a file called **log.txt**.