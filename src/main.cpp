#include <dummy.h>

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 4
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

//--------------Wifi Variables--------------//
 const char* ssid = ""; // input network ssid/name
 const char* password = ""; // input network password

//--------------Client Variables--------------//
const int httpsPort = 443;
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const char* GScriptId = ""; // input google script id
String url = String("/macros/s/") + GScriptId + "/exec";

//--------------Other Variables--------------//
boolean payloadFlag = 0;           // determines whether payload contains event time or current time
const byte buttonPin = 5;          // digital pin #5 - button pin
int pushButton;                    // variable to store pushButton state
int diagnostics = 0;               // runs certain LED functions (e.g. wifi connected, batt level)

//--------------LED Variables--------------//
uint32_t red = strip.Color(255,0,0);
uint32_t green = strip.Color(0,255,0);
uint32_t blue = strip.Color(0,0,255);
uint32_t white = strip.Color(255,255,255);
uint32_t yellow = strip.Color(255,255,0);
uint32_t orange = strip.Color(255,128,0);
uint32_t off = strip.Color(0,0,0);


//--------------Function Prototypes--------------//
int payloadConvert(String);
uint32_t Wheel(byte);
void batteryLED(int battPercent);
void cycleLED(uint32_t, int);
void flashLED(uint32_t, int);
void eventLED(uint8_t);
void batteryLevel();
void ledOff();



void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP); // pull-up resistor, button reads HIGH when open, LOW when pressed
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.println("");
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();

  // if button held after start up, diagnostic features enabled
  pushButton = digitalRead(buttonPin);
  if (pushButton == LOW) {
      diagnostics = 1;
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (diagnostics == 1) {
      cycleLED(white, 1);
      pushButton = digitalRead(buttonPin);
      // stop displaying LED if button pressed
      if (pushButton == LOW) {
        ledOff();
        break;
      }
    }
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  HTTPSRedirect client(httpsPort);
  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      Serial.println("Connection Successful");
      i = 5;
      if (diagnostics == 1) {
         cycleLED(green, 1);
       }
    }
    else if (retval == 0) {
      Serial.println("Connection failed. Retrying...");
      if (diagnostics == 1) {
         cycleLED(blue, 1);
      }
    }
  }

  Serial.flush();
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    cycleLED(red, 1);
    return;
  }
  Serial.println("================================");
}


void loop() {
  int multiplier = 30; // how many minutes to sleep for
  unsigned long napTime = 60000000; // default is 60 seconds

  HTTPSRedirect client(httpsPort);
  if (!client.connected()) {
      client.connect(host, httpsPort);
  }

  String payload = client.getData(url, host, googleRedirHost);
  int eventUnix = payloadConvert(payload);
  int currentUnix = payloadConvert(payload);
  // how many minutes before next event
  int timeDiff = (eventUnix - currentUnix)/60;

  // if event in less than 10min run eventLED
  if (timeDiff < 10) {
      eventLED(50);
  }

  // if event in less than 30min, sleep for timeDiff-9min (e.g. 13-9 = 4min)
  if (timeDiff < 30) {
      multiplier = timeDiff-9;

      // if event in less than 10min, wake up in time for 1min alert
      if (multiplier <= 0) {
          multiplier = timeDiff-1;

          // if event in less than 1min, sleep for 30min (prevents negative values)
          if (multiplier <= 0) {
              multiplier = 30;
          }
      }
  }
  Serial.println(eventUnix);
  Serial.println(currentUnix);
  Serial.println(timeDiff);
  Serial.println(multiplier);

  // check battery level
  batteryLevel();
  // sleep for 30min unless otherwise specified
  ESP.deepSleep(napTime*multiplier, WAKE_RF_DEFAULT);
}


int payloadConvert(String payload){
  // payload example "a60|1482186600;1482131233|0d"
  int unixTime;

  // get index of first | symbol and then remove everything before it
  int prefix = payload.indexOf('|');
  payload.remove(0, prefix+1);
  // get index of last | symbol and then remove everything after it
  int suffix = payload.indexOf('|');
  payload.remove(suffix, 3);

  // payload example "1482186600;1482131233"

  String eventUnix = payload;
  String currentUnix = payload;

  int SemiColon = eventUnix.indexOf(';'); // gets index of semicolon separator
  eventUnix.remove(SemiColon, 15); // removes currentUnix from string
  currentUnix.remove(0, SemiColon+1); // removes eventUnix from string

 if (payloadFlag == 0) {
    unixTime = eventUnix.toInt();
 }
 if (payloadFlag == 1) {
    unixTime = currentUnix.toInt();
 }
 // same function run for both times, controlled by flag
 payloadFlag = !payloadFlag;

 return(unixTime);
}


void eventLED(uint8_t wait) {
  int ledFlag = 0;

  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
     if (ledFlag == 1) {
        ledOff();
        break;
     }
     for (int q=0; q < 3; q++) {   // repeat three times
        if (ledFlag == 1) {
          ledOff();
          break;
        }
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
          pushButton = digitalRead(buttonPin);
          if (pushButton == LOW) {
            ledFlag = 1;
            ledOff();
            break;
          }
        }
        strip.show();
        delay(wait);
        for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
          pushButton = digitalRead(buttonPin);
          if (pushButton == LOW) {
            ledFlag = 1;
            ledOff();
            break;
          }
        }
     }
  }
}


void cycleLED(uint32_t colour, int x) {
  for (int q = 0; q < x; q++) { // how many cycles
    for (int y = 0; y < strip.numPixels(); y++) { // turn on LED, turn off LED, move to the next
      strip.setPixelColor(y, colour);
      strip.show();
      delay(100);
      strip.setPixelColor(y, off);
      strip.show();
    }
  }
}


void flashLED(uint32_t colour, int x) {
  for (int q = 0; q < x; q++) { // how many flashes
    for (int y = 0; y < strip.numPixels(); y++) { // turn on all LEDs then turn off
      strip.setPixelColor(y, colour);
    }
    strip.show();
    delay(500);
    ledOff();
    delay(500);
  }
}


void batteryLED(int battPercent) {
  // displays current battery level by lighting
  // the number of LEDS according to (battPercent/(100/13))
  // LED ring has 13 positions (incl off)
  battPercent = map(battPercent, 0, 100, 0, 12);
  for (int y = 0; y < battPercent; y++) {
    strip.setPixelColor(y, orange);
    strip.show();
    delay(100);
  }
  delay(5000);
  ledOff();
}


void ledOff() {
  for (int i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, off);
      strip.show();
  }
}


void batteryLevel() {

  // 654 ADC = 3.69V = 177.23 ADC per V
  // 1024 ADC = 5.78V
  // Max 4.2v = 744 ADC
  // Min 3.0v = 531 ADC
  // Max-Min = 1.2, 1.2/100 = 0.012
  // 0.012V per battPercent

  int battLevel = analogRead(A0);
  int battPercent = map(battLevel, 531, 744, 0, 100);
  float battVoltage = (3.00 + (battPercent*0.012));

  Serial.print("Battery perecent: "); Serial.print(battPercent); Serial.println("%");
  Serial.print("Battery voltage: "); Serial.print(battVoltage); Serial.println("V");

  if (battVoltage < 3.30) {
    flashLED(red, 3);
  }
  if (battVoltage >= 4.19) {
    flashLED(green, 3);
  }
  if (diagnostics == 1) {
    batteryLED(battPercent);
  }
}


uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
