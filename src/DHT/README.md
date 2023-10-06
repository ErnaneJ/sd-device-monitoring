# DHTXX Digital Temperature and Humidity Sensor

- DHTXX is a digital temperature and humidity sensor.
- It provides a much more precise temperature reading compared to an analog sensor. The output of the DHTXX is a digital signal that can be read on the digital I/O pins of an Arduino.
- However, the sensor's digital output does not conform to common serial data protocols like UART, SPI, or I2C. Instead, the DHTXX operates through a one-wire protocol to digitally communicate sensor data.

## About

The DHTXX sensor is a basic digital temperature and humidity sensor. The sensor comes pre-calibrated and does not require an external circuit to measure temperature or humidity.

For humidity detection, the DHTXX has a resistive component with two electrodes and a substrate that retains moisture between them. When the substrate absorbs water vapor, it releases ions that increase conductivity between the electrodes.

When there is higher relative humidity, the resistance between the electrodes decreases, and when there is lower relative humidity, the resistance between the electrodes increases. This change in resistance is proportional to relative humidity.

For temperature detection, the DHTXX has an NTC thermistor, which is a negative temperature coefficient resistor. The thermistor in the DHTXX has a negative temperature coefficient, so its resistance decreases with increasing temperature.

## DHT11 vs DHT22

The DHT22 is a sensor from the same family. It maintains the same pin configuration, voltage supply, and current consumption as the DHT11 sensor. However, the DHT22 measures humidity in the range of 0 to 100% RH with an accuracy of 2 to 5%. It also measures temperatures ranging from -40°C to 125°C with an accuracy of +/- 0.5°C.

The maximum sampling rate for the DHT22 is 0.5 Hz (i.e., once every two seconds). DHTXX and DHT22 sensors are used for applications such as environmental monitoring, local weather stations, and automatic climate control.

## DHTXX Interface with Arduino

The DHTXX sensor can interface directly with Arduino. It can be supplied with 5V DC and grounded from the Arduino. The sensor's data pin can also be connected directly to any of Arduino's digital I/O pins. However, the digital I/O pin to which it is connected must be configured to use an internal pull-up. Otherwise, an external 10K pull-up resistor must be connected between VCC and the DATA pin of the DHTXX. Alternatively, a DHTXX sensor module can be used.

### The digital communication between the DHTXX and the host controller (such as Arduino) can be divided into four steps:

1. Start Signal: To initiate communication with the DHTXX sensor, the host (Arduino) must send a start signal to the DHTXX DATA pin. The DATA pin is pulled HIGH by default. The start signal is a logic LOW level for 18 milliseconds, followed by a LOW to HIGH transition (rising edge).

2. Response: After receiving a start signal from the host, the DHTXX sends a response signal to indicate that it is ready to transmit sensor data. The response pulse is a logic LOW level for 54 microseconds followed by a logic HIGH level for 80 microseconds.

3. Data: After sending a response pulse, the DHTXX begins transmitting sensor data containing humidity, temperature values, and a checksum byte. The data packet consists of 40 bits, divided into five segments or bytes of 8 bits.

The first two bytes contain the relative humidity value, with the first byte being the integral part of the humidity value and the second byte being the decimal part of the humidity value. The next two bytes contain the temperature value, with the third byte as the integral part and the fourth as the decimal part.

The last byte is a checksum byte, which should be equal to the binary sum of the first four bytes. If the checksum byte does not match the binary sum of the humidity and temperature values, there is an error in the values.

Bits are transmitted as timing signals, where the pulse width of the digital signal determines whether it is bit 1 or bit 0. Bit 0 starts with a logic LOW (Ground) signal for 54 microseconds, followed by a logic HIGH (VCC) signal for 24 microseconds.

Bit 1 starts with a logic LOW (Ground) signal for 54 microseconds, followed by a logic HIGH (VCC) signal for 70 microseconds.

4. End of Frame: After transmitting the 40-bit data packet, the sensor sends a logic LOW for 54 microseconds and then pulls the DATA pin HIGH. After that, the sensor enters a low-power standby mode. Sensor data can be sampled at a rate of 1 Hz or once every second.

## Reading DHTXX Sensor Data

To read data from the DHTXX sensor, the Arduino must first send the initialization signal. To do this, the pin with which the DHTXX DATA pin interfaces must be configured as a digital output. A digital pulse of 18 milliseconds must be passed to the DATA pin, followed by a rising edge. Immediately afterward, the Arduino pin should be configured as a digital input with internal pull-up.

Now, read the response signal from the DHTXX on the Arduino pin. If a falling edge is detected within 90 microseconds, it means that the DHTXX has successfully sent a response pulse.

The data received from the DHTXX sensor can be sampled by monitoring the logic level of the digital pulse while tracking the pulse width. It is possible to track the pulse width by measuring the elapsed time from a certain point in time while monitoring a logic HIGH or LOW.

## Code Examples

- [DTH22Sensor](DHT22Sensor.ino)
- [DTH11Sensor](DHT11SensorRAW.ino)

## Simulation

[Wokwi DHT22](https://wokwi.com/projects/377600286695312385).