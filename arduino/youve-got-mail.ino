#include <SPI.h>
#include <Utils.h>

#define SERIAL_BAUD    9600

#define NODE_NAME      "motion-detector"

#define NODE_ID        4
#define NETWORK_ID     100

#define GATEWAY_ID     1
#define ENCRYPT_KEY    "a7bd91kaxlchdk36"
#define ACK_TIME       30

#define LED_PIN        9
#define MOTION_PIN     1 // hardware interrupt 1 (D3)

volatile boolean ready = false;
volatile boolean motion_detected = false;

void send_data(char *message) {
  send_data_proto(message, GATEWAY_ID, NODE_NAME, LED_PIN);
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

bool is_ready() {
  return ready;
}

void send_ping() {
  char buff[60];

  sprintf(buff, "ping:yes");
  send_data(buff);
}

void attach_interrupt() {
  pinMode(MOTION_PIN, INPUT);
  attachInterrupt(MOTION_PIN, interrupt_triggered, RISING);
}

void detach_interrupt() {
  detachInterrupt(MOTION_PIN);
}

void interrupt_triggered() {
  if (is_ready()) {
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

/*----------------------------------------------------------------------------*/

void setup() {
  setup_serial(SERIAL_BAUD);
  setup_radio(NODE_ID, NETWORK_ID, ENCRYPT_KEY);
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
  sleep(8);
}
