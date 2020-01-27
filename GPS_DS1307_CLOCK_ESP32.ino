#include <Wire.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "Adafruit_LEDBackpack.h"
#include <TinyGPS++.h>
#include <TimeLib.h>
#include <Timezone.h>

//make true to turn on debug mode to serial
bool debug = true;


// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

//static const int RXPin = 16, TXPin = 17;
#define RXPin 16
#define TXPin 17
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();
RTC_DS1307 rtc = RTC_DS1307();

// Keep track of the hours, minutes, seconds displayed by the clock.
// Start off at 0:00:00 as a signal that the time should be read from
// the DS1307 to initialize it.
int hours = 0;
int minutes = 0;
int seconds = 0;

//get if time and date is valid from tinygpsplus
bool gpsTimeValid = false;
bool gpsDateValid = false;

//used with millis() to loop for 1 second and gather gps data in same loop
unsigned long time_now = 0;

// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = true;

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

//setup convert to epoch
time_t gpsTimeToEpoch;
time_t epochToTimezone;


void setup() {
  // Setup function runs once at startup to initialize the display
  // and DS1307 clock.

  // Setup Serial port to print debug output.
  Serial.begin(115200);
  if(debug){
    Serial.println("Clock starting!");
  }

  //Setup the GPS
  Serial2.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  //start wire on SDA = 23, SCL = 22
  Wire.begin(23, 22);

  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);

  // Setup the DS1307 real-time clock.
  rtc.begin();

  // Set the DS1307 clock if it hasn't been set before.
  bool setClockTime = !rtc.isrunning();
  // Alternatively you can force the clock to be set again by
  // uncommenting this line:
  //setClockTime = true;
  if (setClockTime) {
    if(debug){
      Serial.println("Setting DS1307 time!");
    }
    // This line sets the DS1307 time to the exact date and time the
    // sketch was compiled:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Alternatively you can set the RTC with an explicit date & time, 
    // for example to set January 21, 2014 at 3am you would uncomment:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // Get the time from the DS1307.
  DateTime now = rtc.now();
  //on startup set time on the arduino program clock
  hours = now.hour();
  minutes = now.minute();
  seconds = now.second();

  //testing setting display brightness
  clockDisplay.setBrightness(5);
}

void loop() {
  // Loop function runs over and over again to implement the clock logic.
  
  // Check if it's the top of the hour and get a new time reading
  // from the DS1307.  This helps keep the clock accurate by fixing
  // any drift.
  //check the time every 11 minutes, keeps away from hour changes ex(11, 22, 33, 44, 55)
  //also, check only for 10 seconds during the 11 minute window.
  if ((minutes % 11 == 0) && (seconds >= 27 && seconds <= 32)) {
    // Get the time from the DS1307.
    DateTime now = rtc.now();

    if(debug){
      // Print out the time for debug purposes:
      Serial.print("Read date & time from DS1307: ");
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print('/');
      Serial.print(now.year(), DEC);
      Serial.print(' ');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();
    }

    //now state whether the data is valid and updated
    if(debug){
      if(gps.date.isUpdated() && gps.date.age() < 2000){
        Serial.println("Date is updated from GPS");
      } else {
        Serial.println("Date is NOT Updated from GPS");
      }
      if(gps.time.isUpdated() && gps.time.age() < 2000){
        Serial.println("Time is updated from GPS");
      } else {
        Serial.println("Time is NOT updated from GPS");
      }
    

      //print time and date age
      char check00[128];
      sprintf(check00, "GPS date age is: %d", gps.date.age());
      Serial.println(check00);
  
      char check000[128];
      sprintf(check000, "GPS time age is: %d", gps.time.age());
      Serial.println(check000);
  
      //Debug gps time and date to serial
      char sz[128];
      sprintf(sz, "Read date & time from GPS: %02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
      Serial.println(sz);

    }//end debug
    
    // Now set the hours and minutes.
    hours = now.hour();
    minutes = now.minute();

    //check if date and time from gps are good
    if (!gps.date.isValid())
    {
      if(debug){
        //gps date not valid, dont set anything.
        Serial.println("Date not valid.");
      }
    }
    else
    {
      if(debug){
        //gps date valid, compare it to RTC and set RTC if needed.
        char check1[128];
        sprintf(check1, "GPS date inside valid check is: %02d/%02d/%02d", gps.date.month(), gps.date.day(), gps.date.year());
        Serial.println(check1);
      }
      gpsDateValid = true;
    }
    
    if (!gps.time.isValid())
    {
      //gps time not valid, dont set anything
      if(debug){
        Serial.println("Time not valid.");
      }
    }
    else
    {
      if(debug){
        //gps time valid, compare to RTC and set RTC if needed
        //Serial.println("Time valid.");
        char check2[128];
        sprintf(check2, "GPS time inside valid check is: %02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        Serial.println(check2);
      }
      gpsTimeValid = true;
    }

    //if time and date are valid compare RTC and GPS data
    if(gpsDateValid && gpsTimeValid){
      if(debug){
        Serial.println("Date and time valid, convert gps time to proper time zone with DST as needed.");
      }

      //set gps time to local timezone from timezone library
      //convert gps time and date to epoch
      gpsTimeToEpoch = tmConvert_t(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
      
      //convert utc epoch time to selected timezone
      epochToTimezone = usPT.toLocal(gpsTimeToEpoch);

      if(debug){
        //debug
        char buf[40];
        Serial.print("Gps date and time after correction: ");
        sprintf(buf, "%02d/%02d/%02d %02d:%02d:%02d", month(epochToTimezone), day(epochToTimezone), year(epochToTimezone), hour(epochToTimezone), minute(epochToTimezone), second(epochToTimezone));
        Serial.println(buf);
      }
      
      //if the jitter is above 1 minute or 5 seconds reset the rtc to gps time.
      //if(abs((minute(epochToTimezone) - now.minute() > 1)) || (abs(second(epochToTimezone) - now.second() > 15))){
      //test for new if statement with epoch time calculation.
      if(abs(tmConvert_t(year(epochToTimezone), month(epochToTimezone), day(epochToTimezone), hour(epochToTimezone), minute(epochToTimezone), second(epochToTimezone)) - tmConvert_t(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second())) >= 15){

        if(debug){
          //if out of sync, reset RTC from gps data
          Serial.println("GPS and rtc out of sync, resetting rtc");
        }
        rtc.adjust(DateTime(year(epochToTimezone), month(epochToTimezone), day(epochToTimezone), hour(epochToTimezone), minute(epochToTimezone), second(epochToTimezone)));

        //reset now variable
        now = rtc.now();
        
        // Now set the hours and minutes and seconds, from RTC
        hours = now.hour();
        minutes = now.minute();
        seconds = now.second();
        
      }else{
        if(debug){
          Serial.println("gps and rtc inside margin of error");
        }
      }
    }
    if(debug){
      //spacing for data
      Serial.println("--------------------------------------------");
    }

    
   }

  // Show the time on the display by turning it into a numeric
  // value, like 3:30 turns into 330, by multiplying the hour by
  // 100 and then adding the minutes.
  int displayValue = hours*100 + minutes;

  // Do 24 hour to 12 hour format conversion when required.
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (hours > 12) {
      displayValue -= 1200;
      
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayValue += 1200;
    }
  }

  // Now print the time value to the display.
  clockDisplay.print(displayValue, DEC);

  // Add zero padding when in 24 hour mode and it's midnight.
  // In this case the print function above won't have leading 0's
  // which can look confusing.  Go in and explicitly add these zeros.
  if (TIME_24_HOUR && hours == 0) {
    // Pad hour 0.
    clockDisplay.writeDigitNum(1, 0);
    // Also pad when the 10's minute is 0 and should be padded.
    if (minutes < 10) {
      clockDisplay.writeDigitNum(2, 0);
    }
  }

  // Blink the colon by flipping its value every loop iteration
  // (which happens every second).
  blinkColon = !blinkColon;
  clockDisplay.drawColon(blinkColon);

  //if hours is >=12 then illuminate the PM light and alternate the blinking colon
  if (hours >= 12) {
    //clockDisplay.writeDigitRaw(2,4 + 2);

    //if it is PM, then new logic is needed for the colon blinking bit_mask
    if (blinkColon == true){
      clockDisplay.writeDigitRaw(2,4 + 2);
    }else{
      clockDisplay.writeDigitRaw(2,4);
    }
  }

  

  // Now push out to the display the new values that were set above.
  clockDisplay.writeDisplay();

  if(debug){
    //TESTING DEBUG
    char serialPrint[40];
    Serial.print("Time in arduino program now: ");
    sprintf(serialPrint, "%02d:%02d:%02d", hours, minutes, seconds);
    Serial.println(serialPrint);
  }

  // Pause for a second for time to elapse.  This value is in milliseconds
  // so 1000 milliseconds = 1 second.
  //dont use delay
  //delay(1000);
  //use millis() instead
  time_now = millis();
  while(millis() < time_now + 1000){
      //wait approx. [period] ms
      //testing
      char c = Serial2.read();
      gps.encode(c);

      
      //Serial.print(c);

      
  }

  // Now increase the seconds by one.
  seconds += 1;
  // If the seconds go above 59 then the minutes should increase and
  // the seconds should wrap back to 0.
  if (seconds > 59) {
    seconds = 0;
    minutes += 1;
    // Again if the minutes go above 59 then the hour should increase and
    // the minutes should wrap back to 0.
    if (minutes > 59) {
      minutes = 0;
      hours += 1;
      // Note that when the minutes are 0 (i.e. it's the top of a new hour)
      // then the start of the loop will read the actual time from the DS1307
      // again.  Just to be safe though we'll also increment the hour and wrap
      // back to 0 if it goes above 23 (i.e. past midnight).
      if (hours > 23) {
        hours = 0;
      }
    }
  }

  // Loop code is finished, it will jump back to the start of the loop
  // function again!
}

//time to epoch object
time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  tmElements_t tmSet;
  tmSet.Year = YYYY - 1970;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet);
}
