#include <SPI.h>
#include <LowPower.h>
#include <RFM69.h>
#include "Utils.h"

#define FREQUENCY      RF69_433MHZ

RFM69 radio;

void blink_light(byte pin, int delay_ms) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  delay(delay_ms);
  digitalWrite(pin, LOW);
}

void print_and_blink_light(byte pin, char *message) {
  blink_light(pin, 5);
  debug_print(message);
}

void blank_line() {
  Serial.println();
}

void debug_print(char *message) {
  char buff[1024];

  sprintf(buff, "DEBUG: %s", message);
  Serial.println(buff);
}

void send_data_proto(char *message, int gateway_id, char *node_name, int led_pin) {
  byte buffLen;
  char buff[60];
  char vcc[60];

  generate_battery_report(vcc);

  sprintf(buff, "%s:%s,battery:%s", node_name, message, vcc);
  buffLen = strlen(buff);
  print_and_blink_light(led_pin, buff);

  radio.send(gateway_id, buff, buffLen);
  delay(300);
}

void setup_serial(int speed) {
  Serial.begin(speed);
}

void setup_radio(int node_id, int network_id, char *encrypt_key) {
  radio.initialize(FREQUENCY, node_id, network_id);
  radio.encrypt(encrypt_key);
}

void generate_battery_report(char *buff) {
  float vcc;

  vcc = read_vcc() / 1000.0;
  dtostrf(vcc, 0, 2, buff);
}

void sleep(int times) {
  radio.sleep();

  for (int i=0 ; i < times ; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

long read_vcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
