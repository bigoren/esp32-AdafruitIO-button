// Adafruit IO Publish Example
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/
#include <TimeLib.h>

byte buttonRead;
int buttonPresses = 0;
bool resetFlag = false;

unsigned int lastReportTime = 0;
unsigned int lastMonitorTime = 0;

// set up the 'time/seconds' topic
AdafruitIO_Time *seconds = io.time(AIO_TIME_SECONDS);
time_t secTime = 0;

// set up the 'time/milliseconds' topic
//AdafruitIO_Time *msecs = io.time(AIO_TIME_MILLIS);

// set up the 'time/ISO-8601' topic
// AdafruitIO_Time *iso = io.time(AIO_TIME_ISO);
// char *isoTime;

// this int will hold the current count for our sketch
int count = 0;

// set up the 'counter' feed
AdafruitIO_Feed *counter = io.feed("counter");
AdafruitIO_Feed *rssi = io.feed("rssi");
AdafruitIO_Feed *reportButton = io.feed("button");


// message handler for the seconds feed
void handleSecs(char *data, uint16_t len) {
  // Serial.print("Seconds Feed: ");
  // Serial.println(data);
  secTime = atoi (data);
}

// message handler for the milliseconds feed
// void handleMillis(char *data, uint16_t len) {
//   Serial.print("Millis Feed: ");
//   Serial.println(data);
// }

// message handler for the ISO-8601 feed
// void handleISO(char *data, uint16_t len) {
//   Serial.print("ISO Feed: ");
//   Serial.println(data);
//   isoTime = data;
// }

void setButton(AdafruitIO_Data *data) {
  Serial.print("received new button value <- ");
  Serial.println(data->value());
  buttonPresses = atoi(data->value());
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

// time sync function
time_t timeSync()
{
  if (secTime == 0) {
    return 0;
  }
  return (secTime + TZ_HOUR_SHIFT * 3600);
}

void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  // select IO for push button
  pinMode(BUTTON_IO, INPUT);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  // attach message handler for the seconds feed
  seconds->onMessage(handleSecs);

  // attach a message handler for the msecs feed
  //msecs->onMessage(handleMillis);

  // attach a message handler for the ISO feed
  // iso->onMessage(handleISO);

  // attach message handler for the button feed
  reportButton->onMessage(setButton);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  // Because Adafruit IO doesn't support the MQTT retain flag, we can use the
  // get() function to ask IO to resend the last value for this feed to just
  // this MQTT client after the io client is connected.
  reportButton->get();

  setSyncProvider(timeSync);
  setSyncInterval(60); // sync interval in seconds, consider increasing
  
}

void loop() {
  unsigned int currTime = millis();

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  // don't do anything before we get a time read and set the clock
  if (timeStatus() == timeNotSet) {
    if (secTime > 0) {
      setTime(timeSync());
      Serial.print("Time set, time is now -> ");
      digitalClockDisplay();
    }
    else {
      return;
    }
  }
  
  if (currTime - lastMonitorTime >= (MONITOR_SECS * 1000)) {
    // save count to the 'counter' feed on Adafruit IO
    Serial.print("sending count value -> ");
    Serial.println(count);
    counter->save(count);
    // save the wifi signal strength (RSSI) to the 'rssi' feed on Adafruit IO
    rssi->save(WiFi.RSSI());
    
    Serial.print("Time is -> ");
    digitalClockDisplay();

    Serial.print("Button pressed # -> ");
    Serial.println(buttonPresses);

    // increment the count by 1
    count++;
    lastMonitorTime = currTime;
  }
  
  if(currTime - lastReportTime >= (DEBOUNCE_SECS * 1000)) {
    // make debounce for button reads and reports
    buttonRead = digitalRead(BUTTON_IO);
    if (buttonRead) {
      buttonPresses++;
      Serial.print("button pressed! # is now -> ");
      Serial.println(buttonPresses);
      digitalClockDisplay();
      reportButton->save(buttonPresses);
      lastReportTime = currTime;
    }
  }

  // reset the buttonPresses and count at some hour of the day
  if ((hour() == RESET_HOUR) && (minute() == 0) && (second() == 0) && (!resetFlag)) {
    Serial.print("count and presses reset at time -> ");
    digitalClockDisplay();
    buttonPresses = 0;
    reportButton->save(buttonPresses);
    count = 0;
    counter->save(count);
    // remember we just reset so we don't do it again and bombard Adafruit IO with requests
    resetFlag = true;
  }

  // reallow reset to occur after reset time has passed
  if (second() == 1) {
    resetFlag = false;
  }
}
