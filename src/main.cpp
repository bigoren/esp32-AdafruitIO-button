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
#include <ArduinoOTA.h>

byte buttonRead;
int currButtonPresses = 0;
int buttonPresses = 0;
bool resetFlag = false;

unsigned int lastMonitorTime = 0;
unsigned int lastReportTime = 0;
unsigned int firstReportTime = 0;

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
// AdafruitIO_Feed *counter = io.feed("counter");
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
  
  //// Over The Air section ////
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(THING_NAME);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

}

void loop() {
  unsigned int currTime = millis();

  ArduinoOTA.handle();

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  // don't do anything before we get a time read and set the clock
  if (timeStatus() == timeNotSet) {
    if (secTime > 0) {
      setTime(timeSync());
      Serial.print("Time set, time is now <- ");
      digitalClockDisplay();
    }
    else {
      return;
    }
  }
  
  if (currTime - lastMonitorTime >= (MONITOR_SECS * 1000)) {
    // save count to the 'counter' feed on Adafruit IO
    Serial.print("sending rssi value -> ");
    // Serial.println(count);
    // counter->save(count);
    // save the wifi signal strength (RSSI) to the 'rssi' feed on Adafruit IO
    rssi->save(WiFi.RSSI());
    
    Serial.print("Time is: ");
    digitalClockDisplay();

    Serial.print("Button pressed # ");
    Serial.println(buttonPresses);

    // increment the count by 1
    count++;
    lastMonitorTime = currTime;
  }
  
  // Debounce button presses, so we don't count presses more than once and not flood AdafruitIO more than 30 messages per minute
  if((((currTime - lastReportTime) >= (DEBOUNCE_SECS * 1000)) || (currTime < (DEBOUNCE_SECS * 1000))) && (currButtonPresses < 25)) {
    // make debounce for button reads and reports
    buttonRead = digitalRead(BUTTON_IO);
    if (buttonRead) {
      currButtonPresses++;
      buttonPresses++;
      if (buttonPresses > 10) {
        buttonPresses = 0;
      }
      Serial.print("sending button pressed! # is now -> ");
      Serial.println(buttonPresses);
      Serial.print("curren button presses is -> ");
      Serial.println(currButtonPresses);
      digitalClockDisplay();
      reportButton->save(buttonPresses);
      lastReportTime = currTime;
      if (currButtonPresses == 0) {
        firstReportTime = currTime;
      }
    }
  }

  // reset currButtonPresses every minute, to keep reporting messages to AdafruitIO under 30 per minute
  if((currTime - firstReportTime) >= 60000) {
    currButtonPresses = 0;
  }

  // reset the buttonPresses and count at some hour of the day
  if ((hour() == RESET_HOUR) && (minute() == 0) && (second() == 0) && (!resetFlag)) {
    Serial.print("resetting button presses reset at time -> ");
    digitalClockDisplay();
    buttonPresses = 0;
    reportButton->save(buttonPresses);
    count = 0;
    // counter->save(count);
    // remember a reset happened so we don't do it again and bombard Adafruit IO with requests
    resetFlag = true;
  }

  // reallow reset to occur after reset time has passed
  if (second() != 0) {
    resetFlag = false;
  }
}
