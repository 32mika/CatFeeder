#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include "uRTCLib.h"
#include <Stepper.h>

int intervall = 5;  // in Minuten
int nextFeed;       // Zeit bis zum nächsten Feed
int lastFeed;       // letzte Fütterung comp time
int menge = 1;      // Menge konfigurierbar durch Knopf
int futterState;
int intervallState;
int mengeState;

unsigned long prevMil = 0;
unsigned long curMil;
unsigned long clockPrevMil = 0;
unsigned long clockCurMil;

const int SPU = 2048;  // Steps vom Motor
const int buttonPowerPin = 5;
const int futterButton = A1;
const int intervallButton = A2;
const int mengeButton = A3;
const int motorPowerPin = 10;

boolean intervallPrint = true;
boolean first = true;
boolean catFeedingMode = true;

const int intervallStufen[] = { 1, 5, 15, 30, 60, 120, 240, 300, 360, 480, 720, 1440};
const int mengeAuswahl[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

String temp;
String time;
String st;
String min;
String sek;

Stepper Motor(SPU, 6, 7, 8, 9);
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;
DateTime dt;
uRTCLib tmp(0x68);

void setup() {
  lcd.init();
  lcd.backlight();
  URTCLIB_WIRE.begin();

  Motor.setSpeed(10);
  Serial.begin(9600);
  
  URTCLIB_WIRE.begin();

#ifndef ESP8266
  while (!Serial)
    ;
#endif

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }


  dt = rtc.now();
  temp = dt.timestamp();
  time = temp.substring(temp.indexOf("T") + 1, temp.length());
  st = time.substring(0, time.indexOf(":"));
  min = time.substring(time.indexOf(":") + 1, time.lastIndexOf(":"));
  sek = time.substring(time.lastIndexOf(":") + 1, time.length());

  lastFeed = st.toInt() * 60; 
  lastFeed = lastFeed + min.toInt();
  nextFeed = intervall;

  pinMode(buttonPowerPin, OUTPUT);
  pinMode(motorPowerPin, OUTPUT);

  digitalWrite(buttonPowerPin, HIGH);
  digitalWrite(motorPowerPin, HIGH);
}

void loop() {
  if(catFeedingMode == false){
    tmp.refresh();
    updateTime();
    clockCurMil = millis();

    delay(250);
    buttonPress(catFeedingMode);
    

    if(clockCurMil - clockPrevMil > 2500){
      printTime(dt);
      printWeather();
      
      clockPrevMil = millis();
    }
    
    return;
  }
  
  buttonPress();

  updateTime();
  updateNextFeed();
  
  printNextFeed(dt);
  printMenge();
  printIntervall();

  if (nextFeed == 0) {
    futter();
    Serial.println("futter kommt!");
  }
}

void buttonPress(boolean cat){ 
  futterState = analogRead(futterButton);
  intervallState = analogRead(intervallButton);
  mengeState = analogRead(mengeButton);
  
  if (futterState > 1000 && intervallState > 1000 && mengeState > 1000 || futterState > 1000 && intervallState > 1000
      || futterState > 1000 && mengeState > 1000) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nur einen");
    lcd.setCursor(0, 1);
    lcd.print("Knopf druecken!");
    delay(5000);
    lcd.clear();
    return;

  } else if(intervallState > 1000 && mengeState > 1000){
    catFeedingMode = true;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Wechsel in den ");
    lcd.setCursor(0, 1);
    lcd.print("Katzen Modus!");
    delay(5000);
    lcd.clear();

    return;
  }
}

void buttonPress(){
  int count = 0;

  futterState = analogRead(futterButton);
  intervallState = analogRead(intervallButton);
  mengeState = analogRead(mengeButton);

  if (futterState > 1000 && intervallState > 1000 && mengeState > 1000 || futterState > 1000 && intervallState > 1000
      || futterState > 1000 && mengeState > 1000) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nur einen");
    lcd.setCursor(0, 1);
    lcd.print("Knopf druecken!");
    delay(5000);
    lcd.clear();

  } else if(intervallState > 1000 && mengeState > 1000){
    catFeedingMode = false;
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Wechsel zum ");
    lcd.setCursor(0, 1);
    lcd.print("Termometer o. Uhr");
    delay(5000);
    lcd.clear();
    return;

  }else if (futterState > 1000) {
    futter();

  } else if (intervallState > 1000) {
    for (int d : intervallStufen) {
      if (intervall == d) {
        if(count +1 == 12){
          intervall = intervallStufen[0];
          printIntervall(true);
          delay(250);
          return;
        }

        intervall = intervallStufen[count + 1];
        printIntervall(true);
        delay(250);
        return;
        
      }
      count++;
    }
  } else if (mengeState > 1000) {
    for (int d : mengeAuswahl) {
      if (menge == d) {
        menge = mengeAuswahl[count + 1];
        printMenge(true);
        delay(250);
        return;
        
      }
      count++;
    }
  }

  count = 0;
}


void printMenge() {
  curMil = millis();

  if(intervallPrint == false){
    return;
  }

  if(!(curMil - prevMil > 7500) && first == false){
    return;
  }

  first = false;
  prevMil = millis();

  lcd.clear();
  printNextFeed(dt);

  lcd.setCursor(0, 1);
  lcd.print("Menge: ");
  lcd.print(menge);
  
  intervallPrint = false;
}

void printMenge(boolean durchlass) {
  lcd.clear();
  printNextFeed(dt);

  lcd.setCursor(0, 1);
  lcd.print("Menge: ");
  lcd.print(menge);
}


void printIntervall(boolean druchlass) {
  lcd.clear();
  printNextFeed(dt);

  lcd.setCursor(0, 1);
  lcd.print("Intervall: ");
  lcd.print(intervall);

}

void printIntervall() {
  curMil = millis();

  if(!(curMil - prevMil > 7500)){
    return;
  }

  lcd.setCursor(0, 1);
  lcd.print("Intervall: ");
  lcd.print(intervall);
  
  intervallPrint = true;
  prevMil = millis();
}

void updateNextFeed() {
  nextFeed = intervall - ((st.toInt() * 60 + min.toInt()) - lastFeed);
}

void printNextFeed(DateTime dt) {
  lcd.setCursor(0, 0);
  lcd.print("Futter:");
  lcd.setCursor(8, 0);
  lcd.print(nextFeed);
  lcd.print("min");

}

void printWeather(){
  lcd.setCursor(0, 1);
  lcd.print("Temperatur: ");
  lcd.print(tmp.temp()  / 100);
  lcd.print("C");

}

void printTime(DateTime dt) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Uhrzeit: ");
  lcd.print(st);
  lcd.print(":");
  lcd.print(min);
  
}

void updateTime() {
  dt = rtc.now();
  temp = dt.timestamp();
  time = temp.substring(temp.indexOf("T") + 1, temp.length());
  st = time.substring(0, time.indexOf(":"));
  min = time.substring(time.indexOf(":") + 1, time.lastIndexOf(":"));
  sek = time.substring(time.lastIndexOf(":") + 1, time.length());
}

void futter() {
  int stepto = menge * 2048;

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Fuetterung...");
  lcd.setCursor(2, 1);
  lcd.print("Fuetterung...");

  Motor.step(stepto);
  lcd.clear();

  lastFeed = st.toInt() * 60 + min.toInt();
  updateNextFeed();
}