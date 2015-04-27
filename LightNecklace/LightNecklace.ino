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
#define RADIO_INT_PIN 4

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
  uint32_t eventEnd;
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
    radio.setPALevel(RF24_PA_LOW); // master tx low power for now
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
    radio.startListening();
  }
  Serial.print("Boot: ");
  Serial.println(master?"Master":"Slave");
}

void greenRamp(uint32_t tstart, uint32_t tend)
{
  uint32_t now = _millis();
  random();
  while(now<tend)
  {
    analogWrite(greenPin, map(now, tstart, tend, 0, 255));
    slaveServiceQuick();
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
    slaveServiceQuick();
    now = _millis();
  }
}

void duration(uint32_t *tstart, uint32_t *tend, uint32_t bump)
{
  *tstart = *tend;
  *tend = *tend + bump;
}

// check interrupt pin and optionally call slaveService
boolean slaveServiceQuick()
{
  // check int pin
  if( !digitalRead(RADIO_INT_PIN) )
  {
    bool txok,txfail,rxready;
    radio.whatHappened(txok,txfail,rxready);
    if( rxready )
    {
      slaveService(true);
      radio.stopListening();
      radio.startListening();
    }
    else
    {
      Serial.println("int for some other reason");
    }
//    char buf[32];
//    sprintf(buf,"hap %d %d %d", txok, txfail, rxready);
//    Serial.println(buf);
  }
}

// returns true if slave gets a packet
boolean slaveService(boolean force)
{
  boolean ret = false;
  if( !master )
  {
    Packet p;  
    uint32_t now = millis();
    if(force || lastSync == 0 || (now-lastSync) > MAX_UNSYNC )
    {
      if ( radio.available() )
      {
        radio.read( &p, sizeof(Packet) );
        
        // apply information from packet
        int32_t delta = p.time - now;
        randomSeed(p.seed);
        eventEnd = p.eventEnd;
        int32_t deltaDelta = delta-millisDelta;
        millisDelta = delta;
        
        
        lastSync = now; // in millis() time
        
        
        char buf[64];
        sprintf(buf, "now %lu seed %lu end %lu delta %ld\r\n", p.time, p.seed, p.eventEnd, deltaDelta);
        Serial.print(buf);
        ret = true;
      }
      else
      {
        Serial.print(".");
      }
    }
  }
  return ret;
}

void service(uint32_t seedIn)
{
  uint32_t now = millis();
  
  Packet p;  
  
  if( master )
  {
    p.time = now;
    p.seed = seedIn;
    p.eventEnd = eventEnd;
    
    radio.write( &p, sizeof(Packet) );
    
    char buf[64];
    sprintf(buf, "now %lu seed %lu end %lu\r\n", p.time, p.seed, p.eventEnd);
    Serial.print(buf);
    
  }
}

// the loop routine runs over and over again forever:
void loop() {  
  unsigned r = random();
  unsigned val = random();
//  uint32_t now = _millis();
  
  uint32_t ramp_start = 0;
  uint32_t ramp_end = 4000;
  
  int i;
  
//  for(i = 0; i < 128; i++)
//  {
//    if( slaveService(true, true) )
//      break;
//    delay(1);
//  }
  
  
  while(1)
  {
    // this allows us to capture the seed
    rSeed = random();
    randomSeed(rSeed);
    service(rSeed);
    
    if(rSeed % 2)
    {

            strobeBlue(eventStart, eventEnd);
      analogWrite(bluePin,0);
    }
    else
    {
      greenRamp(eventStart, eventEnd);
      analogWrite(greenPin,0);
    }
    duration(&eventStart, &eventEnd, 4000);  
    //duration(&eventStart, &eventEnd, 100+random()%7777);
    
    randomSeed(rSeed);
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
