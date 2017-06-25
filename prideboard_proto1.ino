#include <Adafruit_LEDBackpack.h>

#include <EEPROM.h>


#include <DCF77.h>

#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Utils.h>
#include "DisplayWiring.h"

#include "Adafruit_LEDBackpack.h"

#define DCF_PIN 2
#define RESET_PIN 3
#define INCREMENT_PIN 4
#define LED_COLON_PIN 5

DCF77 dcfclock = DCF77(DCF_PIN, digitalPinToInterrupt(DCF_PIN));
Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();

struct BoardData {
  byte daysWithoutIncident;
  time_t lastTimeUpdated;
};

BoardData data;

unsigned long byteArrayToLong(byte bytearray4[]){
  unsigned long highWord = word(bytearray4[0], bytearray4[1]);
  unsigned long lowWord = word(bytearray4[2], bytearray4[3]);
  unsigned long out = highWord << 16 | lowWord;
  return out;
}


void longToByteArray(long toConvert, byte bytearray4[]){
     bytearray4[0] = toConvert;
     bytearray4[1] = toConvert >> 8;
     bytearray4[2] = toConvert >> 16;
     bytearray4[3] = toConvert >> 24;
}

void flushDisplay(){
  int _hour = hour();
  int _minute = minute();
  int _year = year()-2000;
  int _month = month();
  int _day = day();
  matrix.displaybuffer[0] = numbertable[_hour/10]|numbertable[_hour%10]<<8;
  matrix.displaybuffer[1] = numbertable[_minute/10]|numbertable[_minute%10]<<8;
  matrix.displaybuffer[2] = numbertable[_day/10]|numbertable[_day%10]<<8|numbertable[17]<<8;
  matrix.displaybuffer[3] = numbertable[_month/10]|numbertable[_month%10]<<8|numbertable[17]<<8;
  matrix.displaybuffer[4] = numbertable[2]|numbertable[0]<<8;
  matrix.displaybuffer[5] = numbertable[_year/10]|numbertable[_year%10]<<8;
  matrix.displaybuffer[6] = data.daysWithoutIncident >9?numbertable[data.daysWithoutIncident/10]:0|numbertable[data.daysWithoutIncident%10]<<8;
  
 matrix.writeDisplay();
}


void readFromEprom(){
  EEPROM.get(0,data);
  
}

void storeToEprom(){
  EEPROM.put(0,data);
}

bool dataSanityCheck(){
  /*this code runs after the time has updated, 
   * so lastTimeUpdated must be smaller than the 
   * current time, or something has gone really wrong */
  if(!data.lastTimeUpdated < now()){
    return false;
  }
  int daysToAdd = elapsedDays(now() - data.lastTimeUpdated);
  addDays(daysToAdd);
  return true;
}

void addDays(int daysToAdd){
  data.daysWithoutIncident += daysToAdd;
  data.lastTimeUpdated = now();
  storeToEprom();
}

void incrementDays(){
  addDays(1);
}

void resetDays(){
  data.daysWithoutIncident = 0;
  data.lastTimeUpdated = now();
  storeToEprom();
  flushDisplay();
}

void displayWait(){
  matrix.clear();
  for (uint8_t i=0; i<8; i++) {
    matrix.displaybuffer[i] = numbertable[16] |numbertable[16]<<8;
  }
  matrix.writeDisplay();
  matrix.blinkRate(HT16K33_BLINK_1HZ);
}

void displayError(){
  matrix.clear();
  matrix.displaybuffer[1] = numbertable[14]|numbertable[18]<<8;
  matrix.displaybuffer[2] = numbertable[18]|numbertable[19]<<8;
  matrix.displaybuffer[3] = numbertable[18];
  matrix.blinkRate(HT16K33_BLINK_2HZ);
}

void clearData(){
  data.daysWithoutIncident = 0;
  data.lastTimeUpdated = 0;
  storeToEprom();
}

void enableInterruptsAndTimers(){
  //TODO
}

void setup() {
  pinMode(INCREMENT_PIN,INPUT);
  if(digitalRead(INCREMENT_PIN)){
    clearData();
  }
  dcfclock.Start();
  readFromEprom();
  while(timeStatus() <2);
  
  if(dataSanityCheck()){
    flushDisplay();
    enableInterruptsAndTimers();
  } else {
    displayError();
  }
}


void loop() {
  // put your main code here, to run repeatedly:

}




