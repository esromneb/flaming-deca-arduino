/*
 * Make two rgb's change and flash random color patterns together.
 */
 
// ----- FLAGS -----

//#define DEBUG_TX_RX
#define DEBUG_PRINT_STATE
 
 
//-----( Import needed libraries )-----
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RGBConverter.h>

//----- PINS -----
#define CE_PIN   9
#define CSN_PIN 10
#define RADIO_INT_PIN 4
const int masterPin1 = 7; // short these together for master mode
const int masterPin2 = 8; // short these together for master mode
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int onboardPin = 13; // can only be used before the radio is first turned on :(


unsigned master = 1; // default true, however each board reads at startup
const int pins[3] = {redPin, greenPin, bluePin};


// -----  TIMES / TIMERS -----
uint32_t eventStart;
uint32_t eventEnd;
uint32_t nextEventStart;
uint32_t nextEventEnd;
uint32_t lastSync; // measured in millis() time
uint32_t rSeed;
int32_t millisDelta;


// ---- TYPES ---

typedef struct {
  uint32_t time;
  uint32_t seed;
  uint32_t eventEnd;
} Packet;



RGBConverter Conv();

// ---- RADIO ----
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
const uint64_t pipe = 0xE8E9F1F2A0LL; // Define the transmit pipe


// runs at boot
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
  if( master )
  {
    analogWrite(redPin,255);
  }
  else
  {
    analogWrite(greenPin,255);
  }
}

// returns time corrected millis() value
uint32_t _millis()
{
  // alter if slave
  return millis() + millisDelta;
}

uint32_t swizzle(uint32_t x)
{
  x ^= x >> 6; // a
  x ^= x << 13; // b
  x ^= x >> 12; // c
  return x * 858216577; //UINT64_C(2685821657736338717);
}

void flashBootPattern()
{
   // FLASH BOOT PATTERN
  pinMode(onboardPin, OUTPUT);
  
  for(unsigned i = 0; i < 30; i++)
  {
    digitalWrite(onboardPin,LOW);
    delay(65-(i/30.0)*65.0);
    digitalWrite(onboardPin,HIGH);
    delay(85-(i/30.0)*80.0);
  }
  digitalWrite(onboardPin,HIGH);
  delay(300);
  pinMode(onboardPin,INPUT);
  // END BOOT
}


// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.

  Serial.begin(19200);
  randomSeed(42);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); 
 
  //pin 13 "onboardPin" for the onboard led can only be used if radio is NOT inintiaited
  // sad face
  flashBootPattern();
  
  eventStart = 0;
  eventEnd = 4000;
  lastSync = 0;
  millisDelta = 0;
  setMaster();
  radio.begin();
  if( master )
  {
    //radio.setPALevel(RF24_PA_LOW); // master tx low power for now
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



// ------------ PATTERNS -----------






uint8_t redVal;
uint8_t greenVal;
uint8_t blueVal;

#define RED redVal
#define BLUE blueVal
#define GREEN greenVal


// master was restarted to WAY earlier, lets bail
#define HELPER_BAIL_EARILY  if( _now+(tend-tstart) < eventStart ){Serial.println("Bail master was restarted");break;}

#define SRED(x) analogWrite(redPin,x);redVal = x;
#define SGREEN(x) analogWrite(greenPin,x);greenVal = x;
#define SBLUE(x) analogWrite(bluePin,x);blueVal = x;


void writeColor(byte rgb[])
{
  SRED(rgb[0]);
  SGREEN(rgb[1]);
  SBLUE(rgb[2]);
}
// 0..1
double randomdouble()
{
  double ret;
  ret = random(0,10000)/10000.0;
  return ret;
}

void greenRamp(uint32_t tstart, uint32_t tend)
{
  uint32_t _now = _millis();
  random();
  while(_now<tend)
  {
    analogWrite(greenPin, map(_now, tstart, tend, 0, 255));
    slaveServiceQuick();
    _now = _millis();
//    char buf[64];
//    sprintf(buf,"now %lu end %lu start %lu\r\n", _now, tstart, tend);
//    Serial.print(buf);
    HELPER_BAIL_EARILY;
  }
}

void strobeBlue(uint32_t tstart, uint32_t tend)
{
  char buf[64];

  uint32_t _now = _millis();
  unsigned sections = random()%10+5;
  uint32_t length = (tend-tstart)/sections;

  unsigned thisPin = pins[random()%3];

  // use swizzle for timing stuff FIXME

  while(_now<tend)
  {
    if( _now%(length*2) < length )
    {
      if( randomdouble() < 0.2 )
      {
        thisPin = pins[random()%3];
      }
      analogWrite(thisPin, 0);
    }
    else
    {
      analogWrite(thisPin, 255);
    }
    
    if( randomdouble() < 0.001 )
    {
      length -= length / 5;
      length = max(length, 30);
    }
//    Serial.println(now%length);
    slaveServiceQuick();
    _now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void hueRoll(uint32_t tstart, uint32_t tend)
{
  uint32_t _now = _millis();
  unsigned dir = random()%2;
  while(_now<tend)
  {
   
    double val = map(_now, tstart, tend, 0, 10000);
    val = val / 10000.0;
    
    if( dir )
    {
      val = 1.0 - val;
    }

    byte rgb[3];
    RGBConverter().hsvToRgb(val, 1.0, 1.0, rgb);
    writeColor(rgb);
   
    slaveServiceQuick();
    _now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void hueRollLimited(uint32_t tstart, uint32_t tend)
{
  uint32_t _now = _millis();
  unsigned dir = random()%2;
  int limStart,limEnd;
  limStart = random()%4500;
  limEnd = random()%4500;
  
  while(_now<tend)
  {
   
    double val = map(_now, tstart, tend, 0+limStart, 10000-limEnd);
    val = val / 10000.0;
    
    if( dir )
    {
      val = 1.0 - val;
    }

    byte rgb[3];
    RGBConverter().hsvToRgb(val, 1.0, 1.0, rgb);
    writeColor(rgb);
   
    slaveServiceQuick();
    _now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void hueRollExtended(uint32_t tstart, uint32_t tend)
{
  uint32_t _now = _millis();
  unsigned dir = random()%2;
  int limStart,limEnd;
  limStart = random()%4500;
  limEnd = random()%4500;
  
  while(_now<tend)
  {
   
    double val = map(_now, tstart, tend, 0+limStart, 10000-limEnd);
    val = val / 10000.0;

    

    byte rgb[3];
    RGBConverter().hsvToRgb(val, 1.0, 1.0, rgb);
    writeColor(rgb);
   
    slaveServiceQuick();
    _now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void pickNColorCycle(uint32_t tstart, uint32_t tend)
{
  uint32_t _now = _millis();
  unsigned pick_max = 90;
  unsigned min_time = 500;
  unsigned max_time = 700;
  unsigned walk = 100;
  
  

  unsigned stamps[pick_max];
  
  stamps[0] = tstart;
  unsigned i;
  for(i = 1; i < pick_max; i++)
  {
    stamps[i] = random(min_time, max_time) + stamps[i-1];
  }

  byte rgb[3];
  
//  SRED(random()%255); // set point for red (important)
  double hue = random(1,1000)/1000.0;
//  Serial.print("h rand:");
//  Serial.println(hue);

  double s = 1-randomdouble()*.3;
  double v = 1-randomdouble()*.2;

  RGBConverter().hsvToRgb(randomdouble(), s, v, rgb);
  writeColor(rgb);
//  Serial.println(rgb[0] * 1.000001);
//Serial.println(rgb[1] * 1.000001);
//Serial.println(rgb[2] * 1.000001);
//  writeColor(rgb);
  
  i = 0; //reuse
  while(_now<tend)
  {
    if( _now > stamps[i])
    {
//      SRED(RED+(random()%walk)-walk);
        RGBConverter().hsvToRgb(randomdouble(), s, v, rgb);
        writeColor(rgb);
//      Serial.print("picked red as: ");
//      Serial.println(RED);
      i++;
      i = min(i,pick_max-1);
    }
//    Serial.println("here");
    
    
    
    //analogWrite(redPin, random()%256);
    
    slaveServiceQuick();
    _now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void duration(uint32_t *tstart, uint32_t *tend, uint32_t bump)
{
  *tstart = *tend;
  *tend = *tend + bump;
}

// check interrupt pin and optionally call slaveService
void slaveServiceQuick()
{
  if( master )
  {
    return;
  }
  
  if(serialChannelAvail(sizeof(Packet)))// required to call this a lot
  {
    slaveService();
    return; // SKIP RADIO PACKET
  }
  
  // check int pin
  if( !digitalRead(RADIO_INT_PIN) )
  {
    bool txok,txfail,rxready;
    radio.whatHappened(txok,txfail,rxready);
    if( rxready )
    {
      slaveService();
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
boolean slaveService()
{
  boolean ret = false;
  if( master )
  {
    return false;
  }
  Packet p;  
  uint32_t now = millis();

  boolean gotPacket = false;
  // check serial
  if(serialChannelAvail(sizeof(Packet)))
  {
     serialChannelRead( &p, sizeof(Packet) );
     ret = true;
  }
  // check radio
  if ( radio.available() )
  {
    radio.read( &p, sizeof(Packet) );
    ret = true;
  } 
  
  if( ret )
  {
    // apply information from packet
    int32_t delta = p.time - now;
    //randomSeed(p.seed); // don't seed here  UNUSED
    eventEnd = p.eventEnd;
    int32_t deltaDelta = delta-millisDelta;
    millisDelta = delta;
    
    
    lastSync = now; // in millis() time
      
//#ifdef DEBUG_TX_RX
      char buf[64];
      sprintf(buf, "now %lu seed %lu end %lu delta %ld\r\n", p.time, p.seed, p.eventEnd, deltaDelta);
      Serial.print(buf);
//#endif
    } // serial or radio packet
    
    
    return ret;
} // fn


void printState()
{
  uint32_t _now = _millis();
  uint32_t pick = random();
  
  char buf[64];
  sprintf(buf, "_now = %lu pick %lu %lu %lu start %lu end %lu", _now, random(), random(), random(), eventStart, eventEnd);
  Serial.println(buf);

}

// 0 for nothing, 1 for got header bytes (BUT PACKET BYTES ARE UNREAD). 2 for everything ready
int serialChannelPacket = 0;

char preamble[32] = "abcdefghijklm";

void serialChannelWrite(void* p, unsigned size)
{
  Serial.print(preamble);
  char *c = (char*)p;
  for(unsigned i = 0; i < size; i++)
  {
    Serial.print(c[i]);
  }
}

boolean serialChannelAvail(unsigned size)
{
  unsigned avail = Serial.available();
//  Serial.print("in serialChannelAvail:");
//  Serial.println(serialChannelPacket);

  if( serialChannelPacket == 2 )
  {
    return true;
  }

  // only if we are in middle mode
  if( serialChannelPacket == 1 )
  {
    if( avail >= size )
    {
      Serial.println("full packet bytes");
      serialChannelPacket = 2;
      return true;
    }
    return false;
  }
  
  
  // normal operation (wait till pream + msg bytes are avail)
  if( size + strlen(preamble) >= avail)
  {
    return false;
  }
  
  char c;
  unsigned ptr = 0;
  unsigned target = strlen(preamble);
  while(Serial.available())
  {
    c = Serial.read();
    Serial.print(c);
    if( c == preamble[ptr] )
    {
      ptr++;
    }
    else
    {
      ptr = 0;
    }
    if( ptr == target)
    {
      serialChannelPacket = 1;
      Serial.println("GOT PACKET");
      return serialChannelAvail(size); // recurse one time to check if all the bytes are ready now
    }
  }
  return false;
}

void serialChannelRead(void* p, unsigned size)
{
  char *c = (char*)p;
  Serial.println("\r\n about to packet:");
  for(unsigned i = 0; i < size; i++)
  {
    c[i] == Serial.read();
    Serial.println(c[i]);
  }
  serialChannelPacket = 0; // reset flag
}



void masterService()
{
  uint32_t now = millis();
  
  Packet p;  
  
  if( master )
  {
    p.time = now;
    p.seed = 0; // UNUSED
    p.eventEnd = eventEnd;
    
    radio.write( &p, sizeof(Packet) );
    serialChannelWrite( &p, sizeof(Packet) );
    
    char buf[64];
#ifdef DEBUG_TX_RX
    sprintf(buf, "now %lu seed %lu end %lu\r\n", p.time, p.seed, p.eventEnd);
    Serial.print(buf);
#endif
    
  }
}

// the loop routine runs over and over again forever:
void loop() {
  // run once
  unsigned r = random();
  unsigned val = random();
//  uint32_t now = _millis();

  // run once setup variables  
  uint32_t ramp_start = 0;
  uint32_t ramp_end = 4000;



  while(1)
  {
          
    rSeed = random();
    masterService();
    
    unsigned next = swizzle(eventEnd) % 5;
//    next = 4; // force
    
    switch(next)
    {
      default:
      case 0:
        strobeBlue(eventStart, eventEnd);
        break;
      case 1:  
        greenRamp(eventStart, eventEnd);
        break;
      case 2:
        pickNColorCycle(eventStart, eventEnd);
        break;
      case 3:
        hueRoll(eventStart, eventEnd);
        break;
      case 4:
        hueRollLimited(eventStart, eventEnd);
        break;
    }
    
    // rng WILL not be the same until seed below
    
//    analogWrite(bluePin,0);
//    analogWrite(redPin,0);
//    analogWrite(greenPin,0);
    
    duration(&eventStart, &eventEnd, 4000);
    eventEnd += swizzle(eventEnd)%7555;
    //duration(&eventStart, &eventEnd, 100+random()%7777);
    randomSeed(eventEnd);
    
#ifdef DEBUG_PRINT_STATE
    printState();
    Serial.print("\r\n\r\n");
#endif

  } // while (1)
} // loop
