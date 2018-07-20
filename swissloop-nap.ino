/*
 * Swissloop NAP
 *
 *  Created on: 09 Jun 2018
 *      Author: Carl Friess
 */

#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SD.h>

#define TAPE_INTERVAL_DISTANCE   30480  // mm

#define PIN_LEFT  2
#define PIN_RIGHT 3

byte mac[] = {
  0x00, 0x08, 0xDC, 0x53, 0x5C, 0xDE
};
IPAddress ip(192, 168, 0, 8);

const IPAddress remote_ip(192, 168, 0, 7);
const unsigned int remote_port = 1340;

EthernetUDP Udp;
bool using_network = false;

File file;
bool using_sd_card = true;

volatile unsigned long time_left = 0;
volatile unsigned long time_right = 0;
volatile int count_left = 0;
volatile int count_right = 0;

void left_isr() {
  time_left = millis();
  count_left++;
}
void right_isr() {
  time_right = millis();
  count_right++;
}

/*
 * Progression in m based on the number of tapes passed
 */
int nav_calc_progression_tape(int tape_count)
{
    uint64_t s = tape_count;
    s *= TAPE_INTERVAL_DISTANCE;
    s /= 1000;
    return s;
}

/*
 * Velocity in km/h based on the time between two tapes
 */
int nav_calc_velocity_tape(unsigned long time_diff)
{
  if (!time_diff) {
    return 0;
  }
  uint64_t v = TAPE_INTERVAL_DISTANCE;
  v *= 36;
  v /= time_diff;
  v /= 10;
  return v;
}

void setup() {

  // Configure serial
  Serial.begin(9600);
  
  // Configure ethernet
  Ethernet.begin(mac, ip);
  Udp.begin(1339);

  // Configure SD card
  if (!SD.begin(4)) {
    Serial.println("SD card missing!");
    using_sd_card = false;
  }

  // Configure GPIO
  pinMode(PIN_LEFT, INPUT);
  pinMode(PIN_RIGHT, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LEFT),
                  left_isr,
                  RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_RIGHT),
                  right_isr,
                  RISING);

  // Send ready over network
  Udp.beginPacket(remote_ip, remote_port);
  if (Udp.println("Ready")) {
    Udp.endPacket();
    using_network = true;
  } else {
    Serial.println("Network down!");
  }

  // Print ready to serial
  Serial.println("Ready");
  
}

void loop() {

  static unsigned long last_time_left = 0;
  static unsigned long last_time_right = 0;
  static int last_count_left = 0;
  static int last_count_right = 0;

  char string[100];

  // Check if a new tape was detected
  if (last_count_left < count_left ||
      last_count_right < count_right) {

    // Calculate time differences
    unsigned long time_diff_left = time_left - last_time_left;
    unsigned long time_diff_right = time_right - last_time_right;

    last_count_left = count_left;
    last_count_right = count_right;
    last_time_left = time_left;
    last_time_right = time_right;

    // Build a nicely formatted string
    sprintf(string,
            "LEFT: (%2.d) %4.dm %4.dkm/h\t"
            "RIGHT: (%2.d) %4.dm %4.dkm/h\n",
            count_left,
            nav_calc_progression_tape(count_left),
            nav_calc_velocity_tape(time_diff_left),
            count_right,
            nav_calc_progression_tape(count_right),
            nav_calc_velocity_tape(time_diff_right));

    // Print the string to serial
    Serial.write(string);

    // Send the string over the network
    if (using_network) {
      Udp.beginPacket(remote_ip, remote_port);
      Udp.write(string);
      Udp.endPacket();
    }

    // Store the string to the SD card
    if (using_sd_card &&
          (file = SD.open("log.txt", FILE_WRITE))) {
      file.print(string);
      file.close();
    }

  }

}
