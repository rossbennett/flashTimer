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
#include "TimerOne.h"  // NOTE: this breaks analogWrite() on pins 9 & 10

#define IR_PROXIMITY_SENSOR 2
#define IR_DISTANCE_SENSOR A7
#define ENCODER_PIN_A 3
#define ENCODER_PIN_B 4
#define BUZZER_PIN 9
#define DEFAULT_DURATION 20

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
  // TODO: Store duration in flash memory and reinitialize from there.
  // See Chapter 8 in "Programming Arduino" on how to do that.
  duration = DEFAULT_DURATION;
  timeRemaining = duration;
}

void tick() {
  if (timeRemaining && proximity) timeRemaining--;
}

void doEncoder() {
  /*
   * If pinA and pinB are the same logic, turning is CW. Else CCW.
   * Note this is only true because we're triggering on pin A rising.
   */
   
  delay(10);  // Maximum bounce time from encoder's spec sheet.

  /* TODO: Implement the digital table reading decoder debounce
   * technique outlined at this webpage:
   * https://www.best-microcontroller-projects.com/rotary-encoder.html
   */
  pinA = digitalRead(ENCODER_PIN_A);
  pinB = digitalRead(ENCODER_PIN_B);

  if (pinA == pinB) {
    duration++;
    timeRemaining++;
  } else {
    duration--;
    timeRemaining--;
  }
  constrain(duration, 0, 300);
  constrain(timeRemaining, 0, 300);
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
  proximity = (analogRead(IR_DISTANCE_SENSOR) > 400);
  digitalWrite(LED_BUILTIN, proximity);

  if (!proximity) {
    timeRemaining = duration;
  }

  if (duration != lastDuration) {
    lastDuration = duration;
    updateDisplayDuration();
  }

  if (timeRemaining != lastTimeRemaining) {
    lastTimeRemaining = timeRemaining;
    updateDisplayRemaining();
  }
     
  if (!timeRemaining) chirpAndBlink();
}
