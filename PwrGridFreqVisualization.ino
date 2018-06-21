/*
   This file is part of PwrGridFreqVisualization.

    PwrGridFreqVisualization is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PwrGridFreqVisualization is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <avr/sleep.h>
#include <PinChangeInterrupt.h>
#define PIN_NEO 2
#define PIN_BUTTON 5
#define PIN_FREQ 9
#define PIN_CLOCK 12
//#define SERIAL_OUT
//#define STRIP_OUT
#ifdef STRIP_OUT
#include <Display.h>
#endif

/**
  Button interrupt variables
*/
int buttonTic = 0;  // 1 press on button = 1 tic
boolean buttonPressed = false;  // If button is pressed
int tmpButtonTic = 0; // Tmp cariable to store buttonTic

/**
   Freq interrupt variables
*/
volatile long tmp_tics = 0; // Tmp variable to store timer_tic
volatile int period = -10;  // Period counter
unsigned char send_flag = 0;  // True when a new frequency is calculated
volatile long timer_tic = 0;  // Internal Timer tics

/**
   External clock interrupt variables
*/
long ds_tics = -10; // External clock tics
long correction = 0;  // Correction factor for the internal Timer

/**
   Others variables
*/
volatile char program_state = 0; // 0 = waiting for precision correction , 1 = done with precision  , 2 = mesuring 1st 100 swings , 3 =  normal operating state
double freq = 0;  // Frequency value
boolean modeChange = false; // If mode changed i.e. button is pressed

void setup() {

#ifdef SERIAL_OUT
  Serial.begin(9600);
#endif
#ifdef STRIP_OUT
  initStrip();
#endif
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, LOW);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  // Initialize timer1
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0; // Normal mode

  // Timer stop
  TCNT1 = 0;   // preload timer
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt

  //  Setup PC interrupt
  pinMode(PIN_CLOCK, INPUT);
  pinMode(PIN_FREQ, INPUT);
  digitalWrite(PIN_CLOCK, HIGH);
  digitalWrite(PIN_FREQ, LOW);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_CLOCK), clock_int, RISING); // Enable PCInterrrupt on PIN_CLOCK with rising edge
#ifdef STRIP_OUT
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_BUTTON), button_int, FALLING);
#endif
  interrupts();             // enable all interrupts
  set_sleep_mode(SLEEP_MODE_IDLE);
}

/**
   Internal clock timer interrupt
*/
ISR(TIMER1_OVF_vect) {  // interrupt service routine
  timer_tic += 65536;
}

/**
   Button interrupt with debouncing
*/
void button_int() { // Button Interrupt
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 100) {
    if (!buttonPressed) {
      buttonTic++;
      buttonPressed = true;
      tmpButtonTic = buttonTic;
      modeChange = true;
#ifdef STRIP_OUT
      setInitMode();
#endif
    }
    else {
      buttonPressed = false;
    }
  }
  last_interrupt_time = interrupt_time;
}

/**
   Switch mode from the button (50 or 200MHz)
*/
void setMode(int btTic) {
  if (buttonTic % 2 == 1) {
#ifdef SERIAL_OUT
    Serial.println("Mode 2 ~ 200MHz");
#endif
#ifdef STRIP_OUT
    updateMode(2, freq);
#endif
  }
  else {
#ifdef SERIAL_OUT
    Serial.println("Mode 1 ~ 50MHz");
#endif
#ifdef STRIP_OUT
    updateMode(1, freq);
#endif
  }
}

/**
   Frequency interrupt from the FREQ_PIN (electric power)
*/
void freq_int() {
  if ((program_state == 1) && (period == -1)) {
    TCCR1B = (1 << CS10);  // Start internal Timer with no prescale
    program_state = 2;
  }
  period++;
  if (period == 100) {
    TCCR1B = 0; // Stop internal Timer
    timer_tic = timer_tic + (TCNT1L + (TCNT1H << 8)) - correction;
    TCNT1 = 0;
    tmp_tics = timer_tic;
    timer_tic = 0;
    period = 0;
    send_flag = 1;
    program_state = 3;
    TCCR1B |= (1 << CS10);
  }
}

/**
  Extern clock interrupt for the correction
*/
void clock_int() {
  if (program_state == 0) {
    if (ds_tics == 0) TCCR1B |= (1 << CS10);  // Start internal Timer with no prescale
    ds_tics++;
    if (ds_tics == 32768) {
      TCCR1B = 0; // Stop internal Timer
      timer_tic += TCNT1;
      TCNT1 = 0;
      correction = timer_tic - 16000000;
      timer_tic = 0;
      ds_tics = 0;
      program_state = 1;
      attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_FREQ), freq_int, RISING);  // Enable PCInterrrupt on PIN_FREQ with rising edge
      detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_CLOCK));  // Disable PCInterrupt on PIN_CLOCK
    }
  }
}

/**
  Calculation of the frequency and display it with show_freq()
*/
void loop() {
  float decimal = 0;
  while (1) {
    if (program_state == 3) {
#ifdef STRIP_OUT
      if (modeChange) {
        setMode(tmpButtonTic);
        modeChange = false;
      }
#endif
      cli();
      long lc_send_flag = send_flag;
      send_flag = 0;
      sei();
      if (lc_send_flag) {
        cli();
        long lc_tmp_tics = tmp_tics;
        sei();
#ifdef SERIAL_OUT
        Serial.print("Frequency : ");
#endif
        if (lc_tmp_tics <= 16000000) {
          decimal = ((16000000L - lc_tmp_tics) / 320) * 0.001;
          freq = 50.0 + decimal;
#ifdef SERIAL_OUT
          Serial.println(freq, 3);
#endif
        } else {
          decimal = ((1000 - (lc_tmp_tics - 16000000L) / 320) % 1000) * 0.001;
          if (decimal == 0) decimal = 0.999;
          freq = 49.0 + decimal;
#ifdef SERIAL_OUT
          Serial.println(freq, 3);
#endif
        }
        if (freq < 49.8 || freq > 50.2) digitalWrite(13, HIGH);
        else {
#ifdef STRIP_OUT
          show_freq(freq);
#endif
        }
      }
    }
  }
}

