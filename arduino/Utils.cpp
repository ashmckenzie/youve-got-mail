#include <SPI.h>
#include "Utils.h"

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
