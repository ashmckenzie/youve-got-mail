#include <RFM69.h>
#include <SPI.h>
#include <LowPower.h>
#include <Utils.h>

#define SERIAL_BAUD    9600

#define NODE_NAME      "motion-detector"

#define NODE_ID        4
#define NETWORK_ID     100
#define FREQUENCY      RF69_433MHZ

#define GATEWAY_ID     1
#define ENCRYPT_KEY    "a7bd91kaxlchdk36"
#define ACK_TIME       30

#define LED_PIN        9
#define MOTION_PIN     1 // hardware interrupt 1 (D3)

RFM69 radio;
int TRANSMIT_PERIOD =  300;

volatile boolean ready = false;
volatile boolean motion_detected = false;

void send_data(char *message) {
  byte buffLen;
  char buff[60];
  char vcc[60];

  generate_battery_report(vcc);

  sprintf(buff, "%s:%s,battery:%s", NODE_NAME, message, vcc);
  buffLen = strlen(buff);
  print_and_blink_light(LED_PIN, buff);

  radio.send(GATEWAY_ID, buff, buffLen);
  delay(TRANSMIT_PERIOD);
}

void setup_serial() {
  Serial.begin(SERIAL_BAUD);
}

void setup_radio() {
  radio.initialize(FREQUENCY, NODE_ID, NETWORK_ID);
  radio.encrypt(ENCRYPT_KEY);
}

void attach_interrupt() {
  pinMode(MOTION_PIN, INPUT);
  attachInterrupt(MOTION_PIN, interrupt_triggered, RISING);
}

void detach_interrupt() {
  detachInterrupt(MOTION_PIN);
}

void interrupt_triggered() {
  if (ready) {
    Serial.println("Motion detected!");
    motion_detected = true;
  }
}

void motion_detected_action() {
  char buff[60];

  digitalWrite(MOTION_PIN, LOW);

  sprintf(buff, "movement:yes");
  send_data(buff);
}

void wait_for_pir() {
  for (int i=0 ; i < 15 ; i++) {
    delay(1000);
    blink_light(LED_PIN, 750);
  }

  for (int i=0 ; i < 15 ; i++) {
    delay(1000);
    blink_light(LED_PIN, 250);
  }
}

void starting_up() {
  char buff[60];

  sprintf(buff, "starting:yes");
  send_data(buff);
}

void am_ready() {
  char buff[60];

  sprintf(buff, "ready:yes");
  send_data(buff);
  ready = true;
}

void generate_battery_report(char *buff) {
  float vcc;

  vcc = read_vcc() / 1000.0;
  dtostrf(vcc, 0, 2, buff);
}

void send_ping() {
  char buff[60];

  sprintf(buff, "ping:yes");
  send_data(buff);
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

/*----------------------------------------------------------------------------*/

void setup() {
  setup_serial();
  setup_radio();
  attach_interrupt();
  starting_up();
  wait_for_pir();
  am_ready();
}

void loop() {
  if (motion_detected) {
    motion_detected_action();
  } else {
    send_ping();
  }

  motion_detected = false;

  radio.sleep();
  LowPower.powerDown(SLEEP_60S, ADC_OFF, BOD_OFF);
  // LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
