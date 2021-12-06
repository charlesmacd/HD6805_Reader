/*
  HD6805V1 reader
  (C) 2021 Charles MacDonald
  Based on research by Sean Riddle:
  https://seanriddle.com/mc6805p2.html
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "comms.hpp"
#include "target.hpp"
#include "board.hpp"
#include "cmds.hpp"

void setup() 
{
  pinMode(pin_res_n, OUTPUT);
  pinMode(pin_extal, OUTPUT);
  pinMode(pin_tps_en_n, OUTPUT);
  pinMode(pin_tps_flt_n, INPUT);

  digitalWrite(pin_res_n, HIGH);
  digitalWrite(pin_extal, LOW);
  digitalWrite(pin_tps_en_n, HIGH);

  pinMode(pin_c0, INPUT);
  pinMode(pin_c1, INPUT);
  pinMode(pin_c2, INPUT);
  pinMode(pin_c3, INPUT);
  pinMode(pin_c4, INPUT);
  pinMode(pin_num, INPUT);

  digitalWrite(pin_c0, LOW);
  digitalWrite(pin_c1, LOW);
  digitalWrite(pin_c2, LOW);
  digitalWrite(pin_c3, LOW);
  digitalWrite(pin_c4, LOW);
  digitalWrite(pin_num, LOW);

  pinMode(pin_b0, INPUT);
  pinMode(pin_b1, INPUT);
  pinMode(pin_b2, INPUT);
  pinMode(pin_b3, INPUT);
  pinMode(pin_b4, INPUT);
  pinMode(pin_b5, INPUT);
  pinMode(pin_b6, INPUT);
  pinMode(pin_b7, INPUT);

  digitalWrite(pin_b0, LOW);
  digitalWrite(pin_b1, LOW);
  digitalWrite(pin_b2, LOW);
  digitalWrite(pin_b3, LOW);
  digitalWrite(pin_b4, LOW);
  digitalWrite(pin_b5, LOW);
  digitalWrite(pin_b6, LOW);
  digitalWrite(pin_b7, LOW);

  debug_init(kHostBaudRate);
  Serial.begin(kHostBaudRate);
}

void loop() {
  comms_dispatch();
  delay(100);
}
