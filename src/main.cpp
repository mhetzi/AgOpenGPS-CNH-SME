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

void TRIGGERED() {
    chron.stop();
    if (digitalRead(CHANGE_SW)){
      debounce.restart();
      return;
    }

    if (!debounce.hasPassed(500)){
      serial.println(str_not_passed);
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
  serial.print(str_motor_endstop_type);
  serial.println(flaggen.USE_TIMEBASED ? str_time_based : str_current_based);
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
      serial.printf(str_fmt_time, chron.elapsed(), TIMEOUT, progress);
      return;
    }

    if (!flaggen.USE_TIMEBASED){
      auto current = ina.getCurrent_mA();
      current = abs(current);

      char buf[65] = {0};

      //snprintf(buf, 64, fmt_mc, current, ina.getBusVoltage_V());
      serial.printf(str_fmt_mc, current, ina.getBusVoltage_V());

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
        serial.println(str_set_error_timeout);

        currState = STATE_REG::ERROR;
        return;
      }
    }
}

void loop() {
  if (currState == STATE_REG::IDLE && flaggen.MOT_TEST){
    Serial.println(str_init_sync_motor);
    currState = STATE_REG::EXTEND;
  }

  switch (currState) {
    case STATE_REG::EXTEND:
      Serial.println(str_extending);
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      motor_drv(currState, STATE_REG::EXTENED);
      break;
    case STATE_REG::RETRACT_IDLE:
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      delay(500);
      currState = STATE_REG::RETRACT;
    case STATE_REG::RETRACT:
      Serial.println(str_retracting);
      digitalWrite(STEER_OK, STEER_NOK_SIG);
      motor_drv(currState, STATE_REG::RETRACTED);
      break;
    case STATE_REG::EXTENED:
      if (flaggen.MOT_TEST){
        currState = STATE_REG::RETRACT;
        flaggen.MOT_TEST = false;
        Serial.println(str_init_sync_motor);
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
      serial.printf(
        str_print_errors,
        yesNo(flaggen.USE_TIMEBASED),
        yesNo(flaggen.TIMEDOUT),
        flaggen.TRYS,
        yesNo(flaggen.MOT_TEST)
      );
      delay(1000);
  default:
    break;
  }
}