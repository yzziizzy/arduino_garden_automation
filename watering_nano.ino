

//#include <avr/wdt.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x3f);  // set the LCD address to 0x27 for a 16 chars and 2 line display

RTC_DS1307 RTC;

int LightsOnTime = (60 * (9)) + 0;
int LightsOutTime = (60 * (12 + 9)) + 30;

int WaterLen = (60 * 4) + 0;
long WaterTime = 60L * ((60L * (12L + 0L)) + 0L);

unsigned char BacklightLevel = 128 + 64;

const char relayPin1 = 2;
const char relayPin2 = 3;
const char waterPin = 7;


// data is aligned to 4 bytes to make room for later type changes
const int LightsOnTimePtr = 0;
const int LightsOutTimePtr = 4;
const int WaterLenPtr = 8;
const int WaterTimePtr = 12;
const int BacklightLevelPtr = 16;

char manualWater = 1; // active low relay

const int PIN_COUNT = 14;
char debounceEnable[PIN_COUNT];
char pinState[PIN_COUNT];
char pinPressed[PIN_COUNT];
char tempPinState[PIN_COUNT];
unsigned long debounceLastTime[PIN_COUNT];
const unsigned long debounceDelay = 50;

void processDebounce() {
  int i;

  for(i = 0; i < PIN_COUNT; i++) {
    if(!debounceEnable[i]) continue;
    
    int r = digitalRead(i);

    if(r != tempPinState[i]) {
      debounceLastTime[i] = millis();
    }

    if(millis() - debounceLastTime[i] > debounceDelay) {
      if(!r && pinState[i]) {
        pinPressed[i]++; 
      }
      pinState[i] = r;
    }

    tempPinState[i] = r;
  }


  
}


void initDebounce(int pin) {
  debounceEnable[pin] = 1;
  pinMode(pin, INPUT);
  pinMode(pin, INPUT_PULLUP);
}


void saveData() {
  EEPROM.put(LightsOnTimePtr, LightsOnTime);
  EEPROM.put(LightsOutTimePtr, LightsOutTime);
  EEPROM.put(WaterTimePtr, WaterTime);
  EEPROM.put(WaterLenPtr, WaterLen);
  EEPROM.put(BacklightLevelPtr, BacklightLevel);
}


void loadData() {
  EEPROM.get(LightsOnTimePtr, LightsOnTime);
  EEPROM.get(LightsOutTimePtr, LightsOutTime);
  EEPROM.get(WaterTimePtr, WaterTime);
  EEPROM.get(WaterLenPtr, WaterLen);
  EEPROM.get(BacklightLevelPtr, BacklightLevel);
}

void resetData() {
  
  LightsOnTime = (60 * (9)) + 0;
  LightsOutTime = (60 * (12 + 9)) + 30;
  WaterLen = (60 * 4) + 0;
  WaterTime = 60L * ((60L * (12L + 0L)) + 0L);
  BacklightLevel = 128 + 64;

  saveData();
}

unsigned long gettimes() {
    DateTime now = RTC.now(); 
    unsigned long mm = now.minute();
    unsigned long hh = now.hour();

//Serial.printf("h: %d m: %d\n", hh, mm);
    
    unsigned long s = (unsigned long)now.second() + (mm * 60) + (hh * 3600);

    return s;
}

unsigned long gettime() {
      DateTime now = RTC.now(); 
    unsigned long mm = now.minute();
    unsigned long hh = now.hour();

//Serial.printf("h: %d m: %d\n", hh, mm);
    
    unsigned long s = mm + (hh * 60);

    return s;
}

// the setup function runs once when you press reset or power the board
void setup() {

  initDebounce(10);
  initDebounce(11);
  initDebounce(12);
  
  //saveData();

  loadData();
  
  //wdt_disable();
  //wdt_enable(WDTO_8S);
  Serial.begin(115200);

  Wire.begin();
  delay(10);
  RTC.begin();
  delay(10);
  if ( ! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

 
  DateTime now = RTC.now(); 
  unsigned long mm = now.minute();
  unsigned long hh = now.hour();
  
  Serial.print(hh);
  Serial.print(":");
  Serial.println(mm);

  lcd.begin(16, 2); // initialize the lcd
  lcd.setBacklight(BacklightLevel);
  lcd.display();
  lcd.home(); lcd.clear();
  lcd.print("starting up...");
 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(waterPin, OUTPUT);
  digitalWrite(waterPin, 1);
  
}



void checkWater() {
  unsigned long ss = gettimes();
  char st = manualWater;

  //DateTime now = RTC.now(); 
  //if(now.day() % 2) return;

  if(ss > WaterTime && ss < (WaterTime + WaterLen)) {
    st = 0;
    //Serial.print("watering ");
    //Serial.print(WaterTime + WaterLen - ss);
    //Serial.println(" seconds left");
  }
  
  
  //Serial.print("Lights :");
  //Serial.print(mins);
  //Serial.print("\n");
  digitalWrite(waterPin, st);
}

void checkLights() {
  unsigned long mins = gettime();
  char st = 0;

  if(mins > LightsOnTime && mins < LightsOutTime) {
    st = 1;
  
  }
  
  
  //Serial.print("Lights :");
  //Serial.print(mins);
  //Serial.print("\n");
  digitalWrite(relayPin1, st);
  digitalWrite(relayPin2, !st);
  //digitalWrite(LED_BUILTIN, st);
}


void printTime(unsigned int mins) {
  unsigned int m = (mins % 60);
  lcd.print(mins / 60);
  lcd.print(":");
  if(m < 10) lcd.print("0");
  lcd.print(m);
}

void loop() {

  processDebounce();
  //wdt_reset();

  
  unsigned long mil = millis();
  //digitalWrite(LED_BUILTIN, mil % 4000 < 100);

  checkLights();
  checkWater();
  
  delay(50);


  static char up = 0;
  static char didSave = 0;
  static char dispState = 0;
  if(pinPressed[10]) {
    dispState = (dispState + 1) % 9;
    pinPressed[10] = 0;
    up = 1;
    didSave = 0;
  }
  if(pinPressed[11] || pinPressed[12]) {
    int inc = 1;
    if(pinPressed[12]) inc = -1;

    switch(dispState) {
      case 1:
          LightsOnTime = (LightsOnTime + (inc*15)) % (24 * 60);
          break;
      case 2:
          LightsOutTime = (LightsOutTime + (inc*15))  % (24 * 60);
          break;
      case 3:
          WaterTime = (WaterTime + ((long)inc*15L*60L))  % (24L * 3600L);
          break;
      case 4:
          WaterLen = (WaterLen + (inc*5)) % (6 * 60);
          break;
      case 5:
          // adjust backlight
          BacklightLevel = (BacklightLevel + (inc * 8));
          lcd.setBacklight(BacklightLevel);
          break;
      case 6:
          // manual watering
          manualWater = !manualWater;
          break;
      case 7:
          // set clock
          break;
      case 8:
        if(pinPressed[11] && !pinPressed[12]) {
          // save
          saveData();
          didSave = 1;
        }
        else {
          // reset and save default values
          resetData();
          didSave = 2;
        }

      default:
        break;
    }

    
    pinPressed[11] = 0;
    pinPressed[12] = 0;
    up = 1;
  }
    
  if(dispState != 6) manualWater = 1;

  

  if(dispState == 0) {
    static unsigned long lastTimeUpdate = millis();
    if(millis() - lastTimeUpdate > 1000) {
      lastTimeUpdate = millis();
      DateTime now = RTC.now(); 
      lcd.clear();
      lcd.setCursor(0, 0);
      //lcd.print("                ");
      //lcd.setCursor(0, 0);
      if(now.hour() < 10) lcd.print(" ");
      lcd.print(now.hour() % 12);
      lcd.print(":");
      if(now.minute() < 10) lcd.print("0");
      lcd.print(now.minute());
      lcd.print(":");
      if(now.second() < 10) lcd.print("0");
      lcd.print(now.second());
      if(now.hour() > 12) lcd.print(" PM");
      else lcd.print(" AM");
      lcd.setCursor(0, 1);

      
      /*if(now.day() % 2) {
        lcd.print("water tomorrow");
      }
      else {
        lcd.print("watering today");
      }*/
    }
  }
  else if(dispState == 1) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lights On Time");
      lcd.setCursor(0, 1);
      printTime(LightsOnTime);
      up = 0;
    }
  }
  else if(dispState == 2) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lights Off Time");
      lcd.setCursor(0, 1);
      printTime(LightsOutTime);
      up = 0;
    }
  }
  else if(dispState == 3) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water Time");
      lcd.setCursor(0, 1);
      printTime(WaterTime / 60);
      up = 0;
    }
  }
  else if(dispState == 4) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water Length");
      lcd.setCursor(0, 1);
      printTime(WaterLen);
      up = 0;
    }
  }
  else if(dispState == 5) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("LCD Backlight");
      lcd.setCursor(0, 1);
      lcd.print(BacklightLevel);
      up = 0;
    }
  }
  else if(dispState == 6) {
    if(up) {
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Manual Water");
       lcd.setCursor(0,1);
       if(manualWater) {
         lcd.print(" turn on");
       }
       else {
         lcd.print(" turn off");
       }
       up = 0;
    }
  }
  else if(dispState == 7) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Set Clock"); 
      lcd.setCursor(0,1);
      lcd.print("-not implemented-");
      up = 0;
    }
  }
  else if(dispState == 8) {
    if(up) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Save Settings"); 
      lcd.setCursor(0,1);
      if(didSave == 1) {
        lcd.print("--saved--");
      }
      else if(didSave == 2) {
        lcd.print("--reset done--");
      }
      else {
        lcd.print("<save     reset>");
      }
      up = 0;
    }
  }
  
}
