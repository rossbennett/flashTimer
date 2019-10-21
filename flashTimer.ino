/*
 * FlashTimer by Ross Bennett
 *
 * This project creates a self-starting countdown timer when a 
 * proximity sensor (IR emitter/receiver pair) is triggered.
 * When the specified durion has elapsed, the audible alarm 
 * happens until the proximity sensor reports no proximity.
 * When there is no proximity, the timer is reset to the 
 * indicated duration.
 * 
 * This was originally written to limit the time a 
 * screen printing press arm was left under a 1600 
 * watt flash dryer.
 */

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"
#include "TimerOne.h"         // NOTE: this breaks analogWrite() on pins 9 & 10
#include <EEPROM.h>

#define IR_DISTANCE_SENSOR A7
#define THRESHOLD_PROXIMITY_SENSE 400
#define ENCODER_PIN_A 3       // reading this from rotary encoder
#define ENCODER_PIN_B 4       // reading this from rotary encoder
#define BUZZER_PIN 9          // output pin to fixed-frequency piezo buzzer
#define DEFAULT_DURATION 25   // seconds.
#define MAXIMUM_DURATION 300  // seconds. Same duration as five minutes.
#define MINIMUM_DURATION 0    // seconds.

Adafruit_LiquidCrystal lcd(0);

volatile int duration;
volatile int timeRemaining;
volatile bool proximity;
volatile bool pinA;
volatile bool pinB;
volatile int lastDuration;
volatile int lastTimeRemaining;
char lcdBuffer[16];

void setup() {
  int eeprom_reading;
  int flag;
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  
  Timer1.initialize();
  Timer1.attachInterrupt(tick);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), doEncoder, RISING);
  
  duration = DEFAULT_DURATION;
  eeprom_reading = duration;
  
  EEPROM.get(256, flag);
  if (flag != 123) {
    flag = 123;
    EEPROM.put(256, flag);
    EEPROM.put(0, eeprom_reading);
  } else {
    EEPROM.get(0, eeprom_reading);
  }

  duration = constrain(eeprom_reading, MINIMUM_DURATION, MAXIMUM_DURATION);
  timeRemaining = duration;
}

void tick() {
  if (timeRemaining && proximity) timeRemaining--;
}

void doEncoder() {
   
  delay(10);  // Maximum bounce time from encoder's spec sheet.
   
  pinA = digitalRead(ENCODER_PIN_A);
  pinB = digitalRead(ENCODER_PIN_B);

  /* If pinA and pinB are the same logic, turning is CW. Else CCW.
   * Note this is only true because we're triggering on pin A rising.
   */
 
  if (pinA == pinB) {
    if (++duration > MAXIMUM_DURATION) duration = MAXIMUM_DURATION;
    if (++timeRemaining > MAXIMUM_DURATION) timeRemaining = MAXIMUM_DURATION;
  } else {
    if (--duration < MINIMUM_DURATION) duration = MINIMUM_DURATION;
    if (--timeRemaining < MINIMUM_DURATION) timeRemaining = MINIMUM_DURATION;
  }
}

void updateDisplayDuration() {
  sprintf (lcdBuffer, " Duration:  %3d", duration);
  lcd.setCursor(0, 0);
  lcd.print(lcdBuffer);
}

void updateDisplayRemaining() {
  sprintf (lcdBuffer, " Remaining: %3d", timeRemaining);
  lcd.setCursor(0, 1);
  lcd.print(lcdBuffer);  
}

void chirpAndBlink() {
  // digitalWrite(BUZZER_PIN, HIGH);  Smedley doesn't like this, so we're clobbering it
  lcd.setBacklight(LOW);
  delay(200);
  // digitalWrite(BUZZER_PIN, LOW);   Smedley doesn't like this, so we're clobbering 
  lcd.setBacklight(HIGH);
  delay(100);
}

void loop() {
  int eeprom_write;
  
  proximity = (analogRead(IR_DISTANCE_SENSOR) > THRESHOLD_PROXIMITY_SENSE);
  digitalWrite(LED_BUILTIN, proximity);

  if (!proximity) {
    timeRemaining = duration;
  }

  if (duration != lastDuration) {            // If the duration has changed since the last loop
    lastDuration = duration;                 // set the new duration mark, update the LCD display,
    updateDisplayDuration();                 // and write the setting to non-volatile storage
    eeprom_write = duration;                 // we use this intermediate local int because EEPROM::
    EEPROM.put(0, eeprom_write);             // doesn't trust globally-scoped volatiles (like duration)
  }

  if (timeRemaining != lastTimeRemaining) {  // if the time remaining has changed since the last loop
    lastTimeRemaining = timeRemaining;       // set the new timeRemaining mark
    updateDisplayRemaining();                // and update the display
  }
     
  if (!timeRemaining) chirpAndBlink();       // if the time has run out, indicate so
}
