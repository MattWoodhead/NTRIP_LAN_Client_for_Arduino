/* This script and the associated files allow and Arduino Mega 2560 and an Ethernet shield to act as an NTRIP Server.
 * See the documentation at https://github.com/MattWoodhead/NTRIP_LAN_Server_for_Arduino
 * 
 * Copyright MattWoodhead
 */

#include "NTRIPServer.h"
#include <Ethernet.h>

// This is where the caster address, port, mopuntpoint, password etc are stored.
#include "NTRIPConfig.h"                                               

// SimpleRTK2B RX2 is connected to Mega RX1 (19)
HardwareSerial & RtcmSerial = Serial1;                              // this is assigning a name to the serial 1 port

// Randomly generated at https://www.hellion.org.uk/cgi-bin/randmac.pl 
const byte mac_address[] = {0xA8,0x61,0x0a,0xAE,0x7E,0x6C};           // Note: Newer boards come with a fixed MAC address
byte ip_address[] = {192,168,1,184};                                  // Assign an IP address if DHCP is not available

char char_buffer[512];                                                // buffer into which we will read the RtcmSerial port
int char_count = 0;                                                   // counter to keep track of buffer length

unsigned long previous_time = 0;                                      // timers to decide when to send 1008 message
unsigned long latest_time = 0; 
const int rtcm1008_rate = 10000;                                      // Rate at which to send 1008 message in milliseconds

const byte rtcm1008[] = {                                             // This byte sends a 1008 message with ADVNULL as the antenna model
0xd3,0x00,0x14,0x3f,0x00,0x00,0x0e,0x41,0x44,0x56,0x4e,0x55,0x4c,
0x4c,0x41,0x4e,0x54,0x45,0x4e,0x4e,0x41,0x00,0x00,0x79,0x06,0x89
};

NTRIPServer ntrip_s;                                                  // Create an instance of the NTRIPServer class

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println(F("Starting..."));
  RtcmSerial.begin(115200);                                            //57600 causes issues - crashes arduino uno during ethernet connection
  delay(250);
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);                                              // disable SD card
  
  delay(250);

  //Ethernet.begin(mac_address, ip_address);
  Ethernet.begin(mac_address);                                        // Use this one if using DHCP
  delay(100);
  while (Ethernet.linkStatus() == LinkOFF) {                          // Wait for ethernet to connect
    delay(500);
    Serial.print(".");
  }

  Serial.println(F("LAN connected"));
  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());

  delay(100);
  if (!ntrip_s.subStation(host, httpPort, mountPoint, password, sourceString)) {
    delay(15000);
  }
  Serial.println(F("MountPoint Subscribed"));
  previous_time = millis();
}

void loop() {
  if (ntrip_s.connected()) {
//    digitalWrite(13, HIGH);
    while (RtcmSerial.available()) {
      char c = RtcmSerial.read();                                       // read in a byte as a character
      char_buffer[char_count++] = c;                                    // add character to buffer and increment counter
      
      if ((c == '\n') || (char_count == sizeof(char_buffer)-1)) {       // if the buffer gets full or we see the newline character
        ntrip_s.write((uint8_t*)char_buffer, char_count);               // send buffer to the ntrip server
        char_count = 0;                                                 // reset counter
        for (unsigned int i=0;i<sizeof(char_buffer);i++){                        // wipe the buffer for the next loop
          char_buffer[i] = " ";
        //Serial.println(F("packet sent"));                             // for debugging only
        }
      }
      
      latest_time = millis();
      if ((latest_time - previous_time) > rtcm1008_rate){               // if the specified length of time has passed since the last 1008 message was sent
        ntrip_s.write(rtcm1008, sizeof(rtcm1008));                      // send the 1008 message to the ntrip server
        previous_time = millis();                                       // reset the timer
      }
      
    }
  } else {
//    digitalWrite(13, LOW);
    ntrip_s.stop();
    Serial.println(F("reconnect"));
    delay(1000);                                                        // prevents reconnect spam
    if (!ntrip_s.subStation(host, httpPort, mountPoint, password, sourceString)) {
      delay(100);
    }
    else {
      Serial.println(F("MountPoint Subscribed"));
      delay(10);
    }
  }
  delay(5);                                                            //server cycle
}
