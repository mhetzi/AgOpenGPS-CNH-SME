#pragma once

enum STATE_REG {
    IDLE,
    EXTEND,
    EXTENED,
    RETRACT,
    RETRACT_IDLE,
    RETRACTED,
    ERROR
};


struct Flags {
    bool USE_TIMEBASED;
    bool TIMEDOUT;
    uint8_t TRYS;
    bool MOT_TEST;
    bool BUTTON_PRESSED;

    Flags() {
        this->USE_TIMEBASED = false;
        this->TIMEDOUT = false;
        this->TRYS = 0;
        this->MOT_TEST = true;
        this->BUTTON_PRESSED = false;
    }
};

const char * const str_yes = "Ja";
const char * const str_no = "Nein";

const char* yesNo(bool b){
  return b == true ? str_yes : str_no;
}


const char * const str_fmt_mc PROGMEM = "Motorcurrent: %fmA.\nVoltage: %f\n";
const char * const str_not_passed PROGMEM = "NotPassed!";
const char * const str_motor_endstop_type PROGMEM = "Erkennung des Anschlages?: ";
const char * const str_time_based PROGMEM = "Zeitablauf";
const char * const str_current_based PROGMEM = "Strommessung";

const char * const str_fmt_time PROGMEM = "Vergangene Zeit: %fms von %ms \n Fortschritt %f%%\n";
const char * const str_set_error_timeout = "SET ERROR TIMEOUT!";

const char * const str_init_sync_motor PROGMEM = "Motortest bzw Sync with reality";
const char * const str_extending PROGMEM = "Lenkmotor wird eingehängt...";
const char * const str_retracting PROGMEM = "Lenkmotor wird entfernt...";

const char * const str_print_errors PROGMEM = "ERROR: \n Flags:\n  USE_TIMEBASED: %s\n  TIMEDOUT: %s\n  TRYS: %c\n  MOT_TEST: %s\n\n";