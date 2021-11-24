// lol sup

#include <AccelStepper.h>
#include <LiquidCrystal.h>
#include <String.h>
#include <math.h>


// STEPPER INIT
AccelStepper quill_stepper(AccelStepper::DRIVER, 2, 3);

const int max_speed = 4000;

const int DIRECTION_HOLD = 0;
const int DIRECTION_DOWN = 1;
const int DIRECTION_UP = -1;

int speed;

int direction = DIRECTION_HOLD;


// SWITCH INIT
int at_bottom_limit = 0;
int at_top_limit = 0;

const int toggle_up_pin = 4;
int _last_toggle_up_state = HIGH;
int& last_toggle_up_state = _last_toggle_up_state;

const int toggle_down_pin = 5;
int _last_toggle_down_state = HIGH;
int& last_toggle_down_state = _last_toggle_down_state;

const int bottom_limit_pin = 6;
int _last_bottom_limit_state = HIGH;
int& last_bottom_limit_state = _last_bottom_limit_state;

const int top_limit_pin = 7;
int _last_top_limit_state = HIGH;
int& last_top_limit_state = _last_top_limit_state;


// debounce times
const unsigned int debounce_delay = 750;    // the debounce time; increase if the output flickers

unsigned long _last_toggle_up_debounce_time = 0;
unsigned long& last_toggle_up_debounce_time = _last_toggle_up_debounce_time;

unsigned long _last_toggle_down_debounce_time = 0;
unsigned long& last_toggle_down_debounce_time = _last_toggle_down_debounce_time;

unsigned long _last_bottom_limit_debounce_time = 0;
unsigned long& last_bottom_limit_debounce_time = _last_bottom_limit_debounce_time;

unsigned long _last_top_limit_debounce_time = 0;
unsigned long& last_top_limit_debounce_time = _last_top_limit_debounce_time;


// POTS INIT
int quill_pot_pin = A0;
int quill_pot_reading;
int quill_pot_min_delta = 50;

// POLLING INPUT TIMES

// switches
const unsigned int switches_poll_delay = 100;
unsigned int switches_last_poll = 0;

// pots
const unsigned int pots_poll_delay = 500;
unsigned int pots_last_poll = 0;


// LCD SETUP
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

const float seconds_per_min = 60.0;
const float steps_per_rev = 400.0;



void setup()
{  
    // debug shit
    // Serial.begin(9600);
    // Serial.println("starting...");

    // QUILL STEPPER SETUP
    quill_stepper.setMinPulseWidth(3);
    quill_stepper.setMaxSpeed(max_speed);
    quill_stepper.setSpeed(speed);
    // quill_stepper.setAcceleration(max_speed);

    // INITIAL SPEED FROM POT
    check_pots();


    // SWITCH SETUP
    pinMode(toggle_up_pin, INPUT_PULLUP);
    pinMode(toggle_down_pin, INPUT_PULLUP);
    pinMode(bottom_limit_pin, INPUT_PULLUP);
    pinMode(top_limit_pin, INPUT_PULLUP);

    // INITIAL LIMIT CONTACT CHECKS
    check_switches();

    // LCD SETUP
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("FEED-O-MATIC");
    // Serial.println("printed lcd start");
}


void write_lcd() {
    // Serial.println("WRITING TO LCD");
    // clear screen
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);

    // calculate quill mm/m
    // gear ratio is 3.33
    float rpm = ((float(speed) / 3.33) / steps_per_rev) * seconds_per_min;
    float mm_per_min = rpm * 2;

    String quill_speed = String(mm_per_min, 2);

    String quill_speed_unitised = String(quill_speed + "mm/min");

    String direction_indicator = String("-- ");

    if (direction == 1) {
        direction_indicator = String("UP ");
    } else if (direction == -1) {
        direction_indicator = String("DN ");
    }

    lcd.print(String(direction_indicator + quill_speed_unitised));
    // Serial.println(quill_speed_unitised);
}


bool check_pots() {
    int quill_pot_new_reading = analogRead(quill_pot_pin);

    int speed_delta = abs(quill_pot_new_reading - quill_pot_reading);

    if (speed_delta < quill_pot_min_delta) {
        return false;
    }

    // Serial.print("absolute reading and delta: ");
    // Serial.print(quill_pot_new_reading);
    // Serial.print(" ");
    // Serial.println(speed_delta);

    // [((x ** 3) / 1000000) + 1 for x in range(1, 1024)]

    float x = (pow(float(quill_pot_new_reading), 3) / 1000000) + 1;
    x = x * 2;
    int adjusted_quill_speed_reading = floor(x);
    // Serial.print("Adjusted speed reading: ");
    // Serial.println(adjusted_quill_speed_reading);

    if (adjusted_quill_speed_reading >= 0 && adjusted_quill_speed_reading <= max_speed) {
        quill_pot_reading = quill_pot_new_reading;

        if (adjusted_quill_speed_reading != speed) {
            speed = adjusted_quill_speed_reading;
            return true;
        }

    }
    return false;
}


bool switch_activated(int reading, int& last_state, unsigned long& last_debounce) {
    if (reading != last_state) {
        unsigned long now = millis();

        if ((now - last_debounce) > debounce_delay) {
            last_state = HIGH;
            if (reading != HIGH) {
                    last_debounce = now;
                    return true;
                }
            }
    }
    return false;
}


bool check_switches() {
    bool new_input = false;

    // read all switch inputs, in order of catastrophe
    
    int bottom_limit_pin_reading = digitalRead(bottom_limit_pin);

    if (bottom_limit_pin_reading == LOW) {
        // Serial.println("BL");
        bool bottom_limit_switched = switch_activated(
            bottom_limit_pin_reading,
            last_bottom_limit_state,
            last_bottom_limit_debounce_time
        );
        if (bottom_limit_switched) {
            if (direction == DIRECTION_UP) {
                direction = DIRECTION_HOLD;
                at_top_limit = 1;
                new_input = true;
            }
        };
    }


    int top_limit_pin_reading = digitalRead(top_limit_pin);

    if (top_limit_pin_reading == LOW) {
        // Serial.println("TL");
        bool top_limit_switched = switch_activated(
            top_limit_pin_reading,
            last_top_limit_state,
            last_top_limit_debounce_time
        );
        if (top_limit_switched) {
            if (direction == DIRECTION_DOWN) {
                direction = DIRECTION_HOLD;
                at_bottom_limit = 1;
                new_input = true;
            }
        };
    }

    int toggle_down_pin_reading = digitalRead(toggle_down_pin);

    if (toggle_down_pin_reading == LOW) {
        
        // Serial.println("DT");
        bool toggle_down_switched = switch_activated(
            toggle_down_pin_reading,
            last_toggle_down_state,
            last_toggle_down_debounce_time
        );
        
        if (toggle_down_switched && direction < DIRECTION_DOWN && !at_bottom_limit) {
            direction++;
            if (at_top_limit) {
                at_top_limit = 0;
            };
            new_input = true;
        };
    }

    int toggle_up_pin_reading = digitalRead(toggle_up_pin);

    if (toggle_up_pin_reading == LOW) {
        // Serial.println("UT");
        bool toggle_up_switched = switch_activated(
            toggle_up_pin_reading,
            last_toggle_up_state,
            last_toggle_up_debounce_time
        );
        
        if (toggle_up_switched && direction > DIRECTION_UP && !at_top_limit) {
            direction--;
            if (at_bottom_limit) {
                at_bottom_limit = 0;
            };
            new_input = true;
        };
    }

    return new_input;
}


void loop()
{
    quill_stepper.runSpeed();

    unsigned long now = millis();

    bool update_quill_stepper = false;
    bool update_lcd = false;

    // poll switches
    if (now - switches_last_poll > switches_poll_delay) {
        if (check_switches()) {
            update_quill_stepper = true;
        }
        switches_last_poll = now;
    }

    // Pol Pot
    if (direction == DIRECTION_HOLD) {
        if (now - pots_last_poll > pots_poll_delay) {
            bool pots_changed = check_pots();
            if (pots_changed) {
                // Serial.println("pot changed!");
                update_quill_stepper = true;
            }
            pots_last_poll = now;
        }
    }

    if (update_quill_stepper) {
        // Serial.println("updating speeeeeeeeeeeeeeeeed");
        // Serial.println(speed);
        write_lcd();
        quill_stepper.setSpeed(direction * speed);
    }

}
