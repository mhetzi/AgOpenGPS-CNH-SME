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

const char* yesNo(bool b){
  return b == true ? "Ja" : "Nein";
}