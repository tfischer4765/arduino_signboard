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
#define DCF_TIMEOUT 300

DCF77 dcfclock = DCF77(DCF_PIN, digitalPinToInterrupt(DCF_PIN));
Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();

struct BoardData {
  byte daysWithoutIncident;
  time_t lastTimeUpdated;
};

BoardData data;
bool increment_semaphore, reset_semaphore;
bool timeError = false;
long interrupt_time= 0;

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
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), handleResetInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(INCREMENT_PIN), handleIncrementInterrupt, RISING);
  Alarm.alarmRepeat(0,0,0,incrementDays);
  Alarm.timerRepeat(5,flushDisplay);
}

void handleIncrementInterrupt(){
   if(millis() - interrupt_time >200){
    interrupt_time = millis();
    increment_semaphore=true;
   }
}

void handleResetInterrupt(){
  if(millis() - interrupt_time >200){
    interrupt_time = millis();
    reset_semaphore = true;
  }
}


void setTimeWaitError(){
  timeError = true;
}

void setup() {
  pinMode(INCREMENT_PIN,INPUT);
  if(digitalRead(INCREMENT_PIN)){
    clearData();
  }
  dcfclock.Start();
  readFromEprom();
  displayWait();
  
  Alarm.timerOnce(DCF_TIMEOUT,setTimeWaitError);
  
  while(timeStatus() <2 && !timeError);
  if(!timeError && dataSanityCheck()){
      int daysToAdd = elapsedDays(now() - data.lastTimeUpdated);
      addDays(daysToAdd);
      flushDisplay();
      enableInterruptsAndTimers();
  } else {
    displayError();
  }
}



void loop() {
  if(increment_semaphore){
    incrementDays();
    increment_semaphore = false;
    flushDisplay();
  }
  if(reset_semaphore){
    resetDays();
    reset_semaphore = false;
    flushDisplay();
  }

}




