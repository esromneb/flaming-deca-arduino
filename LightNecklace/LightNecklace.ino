/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
 
/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10

const int masterPin1 = 7;
const int masterPin2 = 8;

const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int pins[3] = {redPin, greenPin, bluePin};
unsigned master = 1;
uint32_t eventStart;
uint32_t eventEnd;
uint32_t nextEventStart;
uint32_t nextEventEnd;
uint32_t lastSync; // measured in millis() time
uint32_t rSeed;
int32_t millisDelta;

#define MAX_UNSYNC (50*1000)

typedef struct {
  uint32_t time;
  uint32_t seed;
} Packet;


/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
//int joystick[2];  // 2 element array holding Joystick readings
const uint64_t pipe = 0xE8E9F1F2A0LL; // Define the transmit pipe


void setMaster() {
  unsigned tied = 1;
  unsigned rd = 0;
  int i;
  
  pinMode(masterPin1, INPUT);
  pinMode(masterPin2, OUTPUT);
  
  for(i = 0; i < 4; i++)
  {
    rd = i%2;
    digitalWrite(masterPin2, rd);
    if( rd != digitalRead(masterPin1) )
    {
      tied = 0;
      break;
    }
  }
  
  master = tied;
  
//  Serial.print("tied together: ");
//  Serial.println(tied);
}

uint32_t _millis()
{
  // alter if slave
  return millis() + millisDelta;
}

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.

  Serial.begin(9600);
  randomSeed(42);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); 
  eventStart = 0;
  eventEnd = 4000;
  lastSync = 0;
  millisDelta = 0;
  setMaster();
  radio.begin();
  if( master )
  {
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
    radio.startListening();
  }
}

void greenRamp(uint32_t tstart, uint32_t tend)
{
  uint32_t now = _millis();
  while(now<tend)
  {
    analogWrite(greenPin, map(now, tstart, tend, 0, 255));
    now = _millis();
  }
}

void strobeBlue(uint32_t tstart, uint32_t tend)
{
  char buf[64];

  uint32_t now = _millis();
  unsigned sections = random()%20+5;
  uint32_t length = (tend-tstart)/sections;
//  Serial.print("length");
//  Serial.println(length);
//  sprintf(buf, "len %ld tstart %ld tend %ld\r\n", length, tstart, tend);
//  Serial.print(buf);
  while(now<tend)
  {
    if( now%(length*2) < length )
    {
      analogWrite(bluePin, 0);
    }
    else
    {
      analogWrite(bluePin, 255);
    }
//    Serial.println(now%length);
    now = _millis();
  }
}

void duration(uint32_t *tstart, uint32_t *tend, uint32_t bump)
{
  *tstart = *tend;
  *tend = *tend + bump;
}

void slaveService(boolean force)
{
  if( !master )
  {
    uint32_t now = millis();
    if(force || lastSync == 0 || (now-lastSync) > MAX_UNSYNC )
    {
      
    }
  }
}

void service(uint32_t seedIn)
{
  uint32_t now = millis();
  
  Packet p;  
  
  if( master )
  {
    p.time = now;
    p.seed = seedIn;
    
    radio.write( &p, sizeof(Packet) );
  }
  else
  {
    if ( radio.available() )
    {
      radio.read( &p, sizeof(Packet) );
      
      // apply information from packet
      randomSeed(p.seed);
      millisDelta = p.time - now;

      lastSync = now; // in millis() time
      
      // master is at 1500
      // slave is at 0
      
      
      
      
      char buf[64];
      sprintf(buf, "now %lu seed %lu\r\n", p.time, p.seed);
      Serial.print(buf);
//      Serial.print("ota: ");
//      Serial.println(nowOTA);
    }
    else
    {
      Serial.println("noota");
    }
  }
}

// the loop routine runs over and over again forever:
void loop() {  
  unsigned r = random();
  unsigned val = random();
//  uint32_t now = _millis();
  
  uint32_t ramp_start = 0;
  uint32_t ramp_end = 4000;
  
 
  
  while(1)
  {
    // this allows us to capture the seed
    rSeed = random();
    randomSeed(rSeed);
    service(rSeed);
    
    greenRamp(eventStart, eventEnd);
    analogWrite(greenPin,0);
    duration(&eventStart, &eventEnd, 100+random()%7777);
  
      
    strobeBlue(eventStart, eventEnd);
    analogWrite(bluePin,0);
    duration(&eventStart, &eventEnd, 100+random()%7777);
  }
  

  
  analogWrite(greenPin,0);
    while(1){}
  

  
//  int thisPin = pins[r%3];
//  analogWrite(thisPin, max(val%256, 200));
//  
//  delay(2000);
  
//  analogWrite(greenPin,255);
//    analogWrite(bluePin,255);
//        analogWrite(redPin,150);

  
  
//  Serial.print(F("%d pick"));
//  Serial.println(r);
}
