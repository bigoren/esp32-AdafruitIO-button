# esp32-AdafruitIO-button
An IOT button with esp32 board sending its reads to the Adafruit IO cloud service.

Adafruit and Wifi credentials need to be set using a secrets.h file, see secrets_template.h file.

The esp32 also gets the time from Adafruit's time topics and resets its count at the RESET_HOUR set.

Over The Air (OTA) programming capability using mDNS is supported. 
Initial upload can't be OTA and should be switched to serial, make sure to change upload settings at platformio.ini file.
