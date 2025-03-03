#include <SolarCalculator.h>
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <hardware/rtc.h>
#include "DHT.h"

#define DHTPIN 27 
#define DHTTYPE DHT22

int line_clear = 0;
float h = 0;
float t = 0;

TinyGPSPlus gps;
int time_dispHour, time_dispMin, time_dispSec;

double latitude = -100.633938;    // Observer's latitude 
double longitude = -27.986206;  // Observer's longitude
int time_zone = 10;          // UTC offset

struct DateTime {
    int year, month, day, hour, minute, second;
};

int recalc = 99; // Recalculation counter
int screen_off = 0;

int gps_year, gps_month, gps_day, gps_hour, gps_min, gps_second;
double transit, sunrise, sunset;

String message;
const int windowSize = 21; // Number of characters to display
const int scrollStep = 1;  // Number of characters to shift each call
int scrollIndex = 0;       // Current scroll position
DateTime parisTime, moscowTime, dubaiTime, chennaiTime, tokyoTime, laTime, nyTime;

char* day_of_week = "Sun";

int suncalc_timezone = 34;

char str[6];

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial1.setRX(17);
  Serial1.setTX(16);
  Serial1.begin(19200); // Start serial communication at 19200 baud

  Serial2.setRX(5);     // Set RX pin to GP5
  Serial2.setTX(4);     // Set TX pin to GP4
  Serial2.begin(9600);  // Start Serial1 at 9600 baud
  //waitForGPSTime();
  Serial1.write(0x19);

  rtc_init();
  datetime_t date_to_set = {
    .year = 2025,
    .month = 3,
    .day = 3,
    .hour = 12,
    .min = 00,
    .sec = 00
  };
  rtc_set_datetime(&date_to_set);
  //dht.begin();

}

void loop() { 

  recalc +=1;
  line_clear +=1;
  while (Serial2.available() > 0) {  // Continuously read available GPS data
    gps.encode(Serial2.read());
  }
  
  if (gps.time.isUpdated() == true){
    datetime_t date_to_set = {
    .year = gps.date.year(),
    .month = gps.date.month(),
    .day = gps.date.day(),
    .hour = gps.time.hour(),
    .min = gps.time.minute(),
    .sec = gps.time.second()
  };
  rtc_set_datetime(&date_to_set);
  DateTime utc = {gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second()}; // Dataset input for the timezone adjustment
  } 
  datetime_t cur_date;
  rtc_get_datetime(&cur_date);
  DateTime utc = {cur_date.year, cur_date.month, cur_date.day, cur_date.hour, cur_date.min, cur_date.sec}; // Dataset input for the timezone adjustment

  DateTime localtime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, time_zone);

  if (recalc == 100){ // Recalculation algorithm. Reduces the strain on the mcu. Can potentially decrease the number to 50
    latitude = gps.location.lat();
    longitude = gps.location.lat();
    calcSunriseSunset(gps.date.year(), gps.date.month(), gps.date.day() , latitude, longitude, transit, sunrise, sunset);
    day_of_week = getDayOfWeek(localtime.day,localtime.month,localtime.year);
    Serial1.write(0x1B);
    Serial1.write(0xFC);
    recalc = 0;
  }

  /*  // To debug this pesky GPS library and other future needs
  Serial1.println(gps_year);
  Serial1.println(gps_month);
  Serial1.println(gps_day);
  Serial1.println(gps_hour);
  Serial1.println(gps_min);
  Serial1.println(gps_second);
  */
  
  // Resetting the cursor
  Serial1.write(0x10);
  Serial1.write((byte) 0x00);
  Serial1.write(0x07);

  // Resetting (typeface) size
  Serial1.write(0x1D);

  // Day of the week/ Date
  Serial1.print(day_of_week);
  Serial1.write(0x1C);
  Serial1.write(0x2C);
  if (line_clear == 300){
    Serial1.write(0x0E);
  }
  Serial1.write(0x1C);
  Serial1.print(intToRoman(localtime.day));
  Serial1.write(0x1D);
  Serial1.write(0x2F);
  Serial1.write(0x1C);
  Serial1.print(intToRoman(localtime.month));
  Serial1.write(0x1D);
  Serial1.write(0x2F);
  Serial1.write(0x1C);
  Serial1.print(intToRoman(localtime.year));
  Serial1.print(" ");

  // The time!!
  Serial1.write(0x10);
  Serial1.write((byte) 0x00);
  Serial1.write(0x18);
  Serial1.write(0x1E);
  
  if (localtime.hour < 10){
    Serial1.print("0");
  }
  Serial1.print(localtime.hour);
  Serial1.write(0x3A);
  if (localtime.minute < 10){
    Serial1.print("0");
  }
  Serial1.print(localtime.minute);
  Serial1.write(0x3A);
  if (localtime.second < 10){
    Serial1.print("0");
  }
  Serial1.print(localtime.second);

  // Sun rise/set
  Serial1.write(0x10);
  Serial1.write(0x58);
  Serial1.write(0x07);
  
  if (localtime.hour > 9) {
    Serial1.write(0x1C);
    Serial1.print(" S S ");
    Serial1.write(0x1D);
    Serial1.print(hoursToString(sunset-2, str));
  }
  else {
    Serial1.write(0x1C);
    Serial1.print(" S R ");
    Serial1.write(0x1D);
    Serial1.print(hoursToString(sunrise-2, str));
  }

  // Temperature
  /*
  Serial1.write(0x10);
  Serial1.write(0x6A);
  Serial1.write(0x1F);
  Serial1.print((int)t);
  Serial1.write(0x95);
  Serial1.write(0x1C);
  Serial1.print("C");
  */

  // Relative Humidity
  /*
  Serial1.write(0x1D);
  Serial1.write(0x10);
  Serial1.write(0x6A);
  Serial1.write(0x17);
  Serial1.print((int)h);
  Serial1.write(0x25);
  Serial1.print(" ");
  */

  // Satellites
  Serial1.write(0x10);
  Serial1.write(0x66);
  Serial1.write(0x0F);
  Serial1.write(0x1C);
  Serial1.print("SAT: ");
  if (line_clear == 300){
    Serial1.write(0x0E);
    line_clear = 0;
  }
  Serial1.write(0x1D);
  if (gps.satellites.value() < 10){
    Serial1.write(0x30);
  }
  Serial1.print(gps.satellites.value());

  // Text Gen
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 250) { // Adjust speed of scrolling
    Serial1.write(0x10);
    Serial1.write((byte) 0x00);
    Serial1.write(0x20);

    DateTime parisTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 1);
    DateTime moscowTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 3);
    DateTime dubaiTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 4);
    DateTime chennaiTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 5.5);
    DateTime brisbaneTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 10);
    DateTime tokyoTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, 9);
    DateTime laTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, -8);
    //DateTime bocaChicaTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, -6);
    DateTime nyTime = adjustTimeZone(utc.year, utc.month, utc.day, utc.hour, utc.minute, utc.second, -5);

    if (gps.satellites.value() < 1){
      message = "GNSS still attempting fix...     ";
    } else {
      message = " London " + String(utc.hour < 10 ? "0" : "") + String(utc.hour) + ":" + String(utc.minute < 10 ? "0" : "") + String(utc.minute) + 
      " Paris " + String(parisTime.hour < 10 ? "0" : "") + String(parisTime.hour) + ":" + String(parisTime.minute < 10 ? "0" : "") + String(parisTime.minute) + 
      " Moscow " + String(moscowTime.hour < 10 ? "0" : "") + String(moscowTime.hour) + ":" + String(moscowTime.minute < 10 ? "0" : "") + String(moscowTime.minute) + 
      " Dubai " + String(dubaiTime.hour < 10 ? "0" : "") + String(dubaiTime.hour) + ":" + String(dubaiTime.minute < 10 ? "0" : "") + String(dubaiTime.minute) + 
      " Chennai " + String(chennaiTime.hour < 10 ? "0" : "") + String(chennaiTime.hour) + ":" + String(chennaiTime.minute < 10 ? "0" : "") + String(chennaiTime.minute) + 
      " Tokyo " + String(tokyoTime.hour < 10 ? "0" : "") + String(tokyoTime.hour) + ":" + String(tokyoTime.minute < 10 ? "0" : "") + String(tokyoTime.minute) + 
      " Brisbane " + String(brisbaneTime.hour < 10 ? "0" : "") + String(brisbaneTime.hour) + ":" + String(brisbaneTime.minute < 10 ? "0" : "") + String(brisbaneTime.minute) + 
      " Los Angeles " + String(laTime.hour < 10 ? "0" : "") + String(laTime.hour) + ":" + String(laTime.minute < 10 ? "0" : "") + String(laTime.minute) + 
      " New York " + String(nyTime.hour < 10 ? "0" : "") + String(nyTime.hour) + ":" + String(nyTime.minute < 10 ? "0" : "") + String(nyTime.minute);
    }
    scrollText();
    lastUpdate = currentTime;
    }
  
  
}

char* getDayOfWeek(int day, int month, int year) { // Day of the week from the given date
    char* days[] = {"Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"};

    if (month < 3) {  // Treat Jan and Feb as months 13 and 14 of previous year
        month += 12;
        year -= 1;
    }

    int K = year % 100;   // Year within the century
    int J = year / 100;   // Zero-based century

    int h = (day + (13 * (month + 1)) / 5 + K + (K / 4) + (J / 4) - (2 * J)) % 7;

    return days[(h + 7) % 7];  // Ensure positive index
} 

char * hoursToString(double h, char *str) // Rounded HH:mm format
{
  int m = int(round(h * 60));
  int hr = (m / 60) % 24;
  int mn = m % 60;

  str[0] = (hr / 10) % 10 + '0';
  str[1] = (hr % 10) + '0';
  str[2] = ':';
  str[3] = (mn / 10) % 10 + '0';
  str[4] = (mn % 10) + '0';
  str[5] = '\0';
  return str;
}

void scrollText() {
    int msgLength = message.length();
    String displayBuffer = "";
    
    for (int i = 0; i < windowSize; i++) {
        displayBuffer += message[(scrollIndex + i) % msgLength];
    }
    
    Serial1.println(displayBuffer); // Output the scrolled text
    
    scrollIndex = (scrollIndex + scrollStep) % msgLength; // Update index with wrap-around
}


void waitForGPSTime() {
  Serial1.write(0x19);

  Serial1.write(0x10);
  Serial1.write((byte) 0x00);
  Serial1.write(0x07);
  Serial1.print("Waiting for");

  Serial1.write(0x10);
  Serial1.write(0x16);
  Serial1.write(0x12);
  Serial1.print("a valid");

  Serial1.write(0x10);
  Serial1.write(0x22);
  Serial1.write(0x1E);
  Serial1.print("GPS signal...");

  while (gps.satellites.value() < 2){
    gps.encode(Serial2.read());

    Serial1.write(0x10);
    Serial1.write(0x77);
    Serial1.write(0x1E);
    Serial1.print(gps.satellites.value());
  }
    Serial1.println("\nGPS time synchronised!");
}

DateTime adjustTimeZone(int year, int month, int day, int hour, int minute, int second, int tz_offset) {
    // Month lengths (non-leap year)
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Apply timezone offset
    hour += tz_offset;

    // Adjust day/month/year if hour overflows
    while (hour >= 24) {
        hour -= 24;
        day++;
        // Check if we need to increment the month
        int maxDays = daysInMonth[month - 1] + (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))); 
        if (day > maxDays) {
            day = 1;
            if (++month > 12) {
                month = 1;
                year++;
            }
        }
    }

    while (hour < 0) {
        hour += 24;
        if (--day < 1) {
            if (--month < 1) {
                month = 12;
                year--;
            }
            day = daysInMonth[month - 1] + (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))); 
        }
    }

    return {year, month, day, hour, minute, second};
}

// Roman Numerals. Should be good.
String intToRoman(int num) {
    struct Roman {
        int value;
        const char *numeral;
    };

    Roman romanMap[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
        {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"},
        {1, "I"}
    };

    String roman = "";
    for (const auto &r : romanMap) {
        while (num >= r.value) {
            roman += r.numeral;
            num -= r.value;
        }
    }
    return roman;
}