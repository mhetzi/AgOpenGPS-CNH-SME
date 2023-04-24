#include <Arduino.h>
#include <Adafruit_INA219.h>
#include <Chrono.h>
#include <PrintEx.h>

#include "structs.h"

Adafruit_INA219 ina;
Chrono  chron;
Chrono debounce;

STATE_REG currState = STATE_REG::IDLE;
Flags flaggen;

PrintEx serial = Serial;

#define MOTORA 7
#define MOTORB 8

#define STEER_OK 4
#define CHANGE_SW 2

#define TIMEOUT 10000 // 10s
#define MIN_MA 70 
#define STEER_OK_SIG LOW
#define STEER_NOK_SIG HIGH

const char * const fmt_mc PROGMEM = "Motorcurrent: %fmA.\nVoltage: %f\n";

void TRIGGERED() {
    chron.stop();
    if (digitalRead(CHANGE_SW)){
      debounce.restart();
      return;
    }

    if (!debounce.hasPassed(500)){
      serial.println("NotPassed!");
      return;
    }
    flaggen.BUTTON_PRESSED = true;
}


void setup() {
  debounce.stop();

  pinMode(MOTORA, OUTPUT);
  pinMode(MOTORB, OUTPUT);
  pinMode(STEER_OK, OUTPUT);
  pinMode(CHANGE_SW, INPUT_PULLUP);

  digitalWrite(MOTORA, LOW);
  digitalWrite(MOTORB, LOW);
  digitalWrite(STEER_OK, STEER_NOK_SIG);
  
  attachInterrupt(digitalPinToInterrupt(CHANGE_SW), TRIGGERED, CHANGE);

  Serial.begin(9600);
  delay(1000);

  flaggen.USE_TIMEBASED = !ina.begin();
  //ina.setCalibration_16V_400mA();
  ina.setCalibration_32V_2A();
  serial.print("Erkennung des Anschlages?: ");
  serial.println(flaggen.USE_TIMEBASED ? "Zeitablauf" : "Stommessung");
}


void motor_drv(STATE_REG from, STATE_REG to) {
    if (!chron.isRunning())
      chron.restart();
    
    digitalWrite(MOTORB, from == STATE_REG::EXTEND);
    digitalWrite(MOTORA, from == STATE_REG::RETRACT);
    
    if (flaggen.USE_TIMEBASED){
      if (chron.hasPassed(TIMEOUT)){
        currState = to;
        chron.stop();
      }

      auto elapsed = chron.elapsed();
      float progress = elapsed / TIMEOUT * 100;
      serial.println("Timebased: ");
      serial.print("Vergangene Zeit: ");
      serial.print(chron.elapsed());
      serial.print("ms / ");
      serial.print(TIMEOUT);
      serial.println("ms");
      serial.print("Fortschritt: ");
      serial.print( progress );
      serial.println("% \n");
      return;
    }

    if (!flaggen.USE_TIMEBASED){
      auto current = ina.getCurrent_mA();
      current = abs(current);

      char buf[65] = {0};

      //snprintf(buf, 64, fmt_mc, current, ina.getBusVoltage_V());
      serial.printf(fmt_mc, current, ina.getBusVoltage_V());

      if (current < MIN_MA){
        flaggen.TRYS++;
        delay(100);
        if (flaggen.TRYS > 2){
          currState = to;
          flaggen.TRYS = 0;
        }
        return;
      }
      if (chron.hasPassed(TIMEOUT * 1.5)){
        flaggen.TIMEDOUT = true;
        serial.println("SET ERROR TIMEOUT!");

        currState = STATE_REG::ERROR;
        return;
      }
    }
}

void loop() {
  if (currState == STATE_REG::IDLE && flaggen.MOT_TEST){
    Serial.println("Motortest bzw Sync with reality");
    currState = STATE_REG::EXTEND;
  }

  switch (currState) {
    case STATE_REG::EXTEND:
      Serial.println("Extending...");
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      motor_drv(currState, STATE_REG::EXTENED);
      break;
    case STATE_REG::RETRACT_IDLE:
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      delay(500);
      currState = STATE_REG::RETRACT;
    case STATE_REG::RETRACT:
      Serial.println("Retracting...");
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      motor_drv(currState, STATE_REG::RETRACTED);
      break;
    case STATE_REG::EXTENED:
      if (flaggen.MOT_TEST){
        currState = STATE_REG::RETRACT;
        flaggen.MOT_TEST = false;
        Serial.println("Motortest bzw Sync with reality");
        return;
      }
      if (flaggen.BUTTON_PRESSED){
        flaggen.BUTTON_PRESSED = false;
        currState = STATE_REG::RETRACT_IDLE;
      }
      digitalWrite(STEER_OK, STEER_OK_SIG);
      digitalWrite(MOTORB, LOW);
      digitalWrite(MOTORA, LOW);
      break;
    case STATE_REG::RETRACTED:
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      digitalWrite(MOTORB, LOW);
      digitalWrite(MOTORA, LOW);
      if (flaggen.BUTTON_PRESSED){
        flaggen.BUTTON_PRESSED = false;
        currState = STATE_REG::EXTEND;
      }
      break;
    case STATE_REG::ERROR:
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      digitalWrite(MOTORB, LOW);
      digitalWrite(MOTORA, LOW);
      Serial.println("Error: \n  Flags:");
      Serial.print("USE_TIMEBASED: ");
      Serial.println(yesNo(flaggen.USE_TIMEBASED));
      Serial.print("TIMEDOUT: ");
      Serial.println(flaggen.TIMEDOUT);
      Serial.print("TRYS: ");
      Serial.println(flaggen.TRYS);
      Serial.print("MOT_TEST: ");
      Serial.println(yesNo(flaggen.MOT_TEST));
      delay(1000);
  default:
    break;
  }
}