#include <CAN.h>

//______________BUTTONS AND SWITCHES
int button4 = 8;
int button3 = 7;
int button2 = 6;
int button1 = 9;
int pedal = A4;
boolean pedalstate = false;
int buttonstate4;
int lastbuttonstate4;
int buttonstate3;
int lastbuttonstate3;
int buttonstate2;
int lastbuttonstate2;
int buttonstate1;
int lastbuttonstate1;

//______________VALUES SEND ON CAN
boolean OP_ON = false;
uint8_t set_speed = 0x0;
double average = 0; 
boolean blinker_left = true;
boolean blinker_right = true;
float LEAD_LONG_DIST = 0;
float LEAD_REL_SPEED = 0;
float LEAD_LONG_DIST_RAW = 0;
float LEAD_REL_SPEED_RAW = 0;
boolean BRAKE_PRESSED = true;


//______________FOR SMOOTHING SPD
const int numReadings = 160;
float readings[numReadings];
int readIndex = 0; 
double total = 0;

//______________FOR READING VSS SENSOR
const byte interruptPin = 3;
int inc = 0;
int half_revolutions = 0;
int spd;
unsigned long lastmillis;
unsigned long duration;
uint8_t encoder = 0;


void setup() {
  
Serial.begin(9600);
CAN.begin(500E3);

pinMode(interruptPin, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(interruptPin), rpm, FALLING);
pinMode(button1, INPUT);
pinMode(button2, INPUT);
pinMode(button3, INPUT);
pinMode(button4, INPUT);

//______________initialize smoothing inputs
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop() {
  
//______________READ SPD SENSOR
attachInterrupt(1,rpm, FALLING);

if (half_revolutions >= 1) {
    detachInterrupt(1);
    duration = (micros() - lastmillis);
    spd = half_revolutions * (0.000135 / (duration * 0.000001)) * 3600;
    lastmillis = micros(); 
    half_revolutions = 0;
    attachInterrupt(1, rpm, FALLING);
  }

//______________SMOOTH SPD TO AVERAGE
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = spd;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits

//______________READING BUTTONS AND SWITCHES
pedalstate = digitalRead(pedal);
buttonstate4 = digitalRead(button4);
buttonstate3 = digitalRead(button3);
buttonstate2 = digitalRead(button2);
buttonstate1 = digitalRead(button1);


if (buttonstate4 != lastbuttonstate4)
    {
       if (buttonstate4 == LOW)
       {
          if (OP_ON == true)
          {
          OP_ON = false;
          }
          else if(OP_ON == false)
          {
          OP_ON = true;
          set_speed = (average += 3);
          }
        }
     }

if (buttonstate3 == LOW)
   {
   blinker_right = false;
   }
  else
   {
   blinker_right = true;
   }

if (buttonstate2 == LOW)
   {
   blinker_left = false;
   }
  else
   {
   blinker_left = true;
   }

if (buttonstate1 != lastbuttonstate1)
    {
       if (buttonstate1 == LOW)
       {
       set_speed += 5;
       }
    }
    
if (pedalstate == LOW)
   {
    Serial.println("Pedal is pressed");
   }

lastbuttonstate1 = buttonstate1;
lastbuttonstate2 = buttonstate2;
lastbuttonstate3 = buttonstate3;
lastbuttonstate4 = buttonstate4;

//______________SENDING_CAN_MESSAGES

  //0x1d2 msg PCM_CRUISE
  uint8_t dat[8];
  dat[0] = (OP_ON << 5) & 0x20;
  dat[1] = 0x0;
  dat[2] = 0x0;
  dat[3] = 0x0;
  dat[4] = 0x0;
  dat[5] = 0x0;
  dat[6] = (OP_ON << 7) & 0x80;
  dat[7] = can_cksum(dat, 7, 0x1d2);
  CAN.beginPacket(0x1d2);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat[ii]);
  }
  CAN.endPacket();

  //0x1d3 msg PCM_CRUISE_2
  uint8_t dat2[8];
  dat2[0] = 0x0;
  dat2[1] = (OP_ON << 7) & 0x80 | 0x28;
  dat2[2] = set_speed;
  dat2[3] = 0x0;
  dat2[4] = 0x0;
  dat2[5] = 0x0;
  dat2[6] = 0x0;
  dat2[7] = can_cksum(dat2, 7, 0x1d3);
  CAN.beginPacket(0x1d3);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat2[ii]);
  }
  CAN.endPacket();

  //0xaa msg defaults 1a 6f WHEEL_SPEEDS
  uint8_t dat3[8];
  uint16_t wheelspeed = 0x1a6f + (average * 100);
  dat3[0] = (wheelspeed >> 8) & 0xFF;
  dat3[1] = (wheelspeed >> 0) & 0xFF;
  dat3[2] = (wheelspeed >> 8) & 0xFF;
  dat3[3] = (wheelspeed >> 0) & 0xFF;
  dat3[4] = (wheelspeed >> 8) & 0xFF;
  dat3[5] = (wheelspeed >> 0) & 0xFF;
  dat3[6] = (wheelspeed >> 8) & 0xFF;
  dat3[7] = (wheelspeed >> 0) & 0xFF;
  CAN.beginPacket(0xaa);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat3[ii]);
  }
  CAN.endPacket();

  //0x3b7 msg ESP_CONTROL
  uint8_t dat5[8];
  dat5[0] = 0x0;
  dat5[1] = 0x0;
  dat5[2] = 0x0;
  dat5[3] = 0x0;
  dat5[4] = 0x0;
  dat5[5] = 0x0;
  dat5[6] = 0x0;
  dat5[7] = 0x08;
  CAN.beginPacket(0x3b7);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat5[ii]);
  }
  CAN.endPacket();

  //0x620 msg STEATS_DOORS
  uint8_t dat6[8];
  dat6[0] = 0x10;
  dat6[1] = 0x0;
  dat6[2] = 0x0;
  dat6[3] = 0x1d;
  dat6[4] = 0xb0;
  dat6[5] = 0x40;
  dat6[6] = 0x0;
  dat6[7] = 0x0;
  CAN.beginPacket(0x620);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat6[ii]);
  }
  CAN.endPacket();

  // 0x3bc msg GEAR_PACKET
  uint8_t dat7[8];
  dat7[0] = 0x0;
  dat7[1] = 0x0;
  dat7[2] = 0x0;
  dat7[3] = 0x0;
  dat7[4] = 0x0;
  dat7[5] = 0x80;
  dat7[6] = 0x0;
  dat7[7] = 0x0;
  CAN.beginPacket(0x3bc);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat7[ii]);
  }
  CAN.endPacket();

  //0x614 msg steering_levers
  uint8_t dat614[8];
  dat614[0] = 0x29;
  dat614[1] = 0x0;
  dat614[2] = 0x01;
  dat614[3] = (blinker_left << 5) & 0x20 |(blinker_right << 4) & 0x10;
  dat614[4] = 0x0;
  dat614[5] = 0x0;
  dat614[6] = 0x76;
  dat614[7] = can_cksum(dat614, 7, 0x614);
  CAN.beginPacket(0x614);
  for (int ii = 0; ii < 8; ii++) {
    CAN.write(dat614[ii]);
  }
  CAN.endPacket();

//______________READING CAN
  CAN.parsePacket();

  //128x2e6 msg LEAD_INFO
  if (CAN.packetId() == 0x2e6)
      {
      uint8_t dat_LEAD_INFO[8];
      for (int ii = 0; ii <= 7; ii++) {
        dat_LEAD_INFO[ii]  = (char) CAN.read();
        }
        LEAD_LONG_DIST_RAW = (dat_LEAD_INFO[0] << 8 | dat_LEAD_INFO[1] << 3); 
        LEAD_REL_SPEED_RAW = (dat_LEAD_INFO[2] << 8 | dat_LEAD_INFO[3] << 4);
        }
  
  //0x224 msg BRAKE_MODULE
    if (CAN.packetId() == 0x224)
      {
      uint8_t dat_224[8];
      for (int ii = 0; ii <= 7; ii++) {
        dat_224[ii]  = (char) CAN.read();
        }
        BRAKE_PRESSED = (dat_224[0] << 5);
        }
Serial.println(BRAKE_PRESSED);
  
}

void rpm() {
  half_revolutions++;
  if (encoder > 255)
  {
    encoder = 0;
  }
  encoder++;
}

//TOYOTA CAN CHECKSUM
int can_cksum (uint8_t *dat, uint8_t len, uint16_t addr) {
  uint8_t checksum = 0;
  checksum = ((addr & 0xFF00) >> 8) + (addr & 0x00FF) + len + 1;
  //uint16_t temp_msg = msg;
  for (int ii = 0; ii < len; ii++) {
    checksum += (dat[ii]);
  }
  return checksum;
}
