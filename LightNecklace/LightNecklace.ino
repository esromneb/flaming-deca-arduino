/*
 * Make two rgb's change and flash random color patterns together.
 */
 
// ----- FLAGS -----

//#define DEBUG_TX_RX
//#define DEBUG_PRINT_STATE
 
 
//-----( Import needed libraries )-----
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//----- PINS -----
#define CE_PIN   9
#define CSN_PIN 10
#define RADIO_INT_PIN 4
const int masterPin1 = 7; // short these together for master mode
const int masterPin2 = 8; // short these together for master mode
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;


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
#define MAX_UNSYNC (50*1000)


// ---- TYPES ---

typedef struct {
  uint32_t time;
  uint32_t seed;
  uint32_t eventEnd;
} Packet;

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
}

// returns time corrected millis() value
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



// ------------ PATTERNS -----------








// master was restarted to WAY earlier, lets bail
#define HELPER_BAIL_EARILY  if( now+(tend-tstart) < eventStart ){Serial.println("Bail master was restarted");break;}


void greenRamp(uint32_t tstart, uint32_t tend)
{
  uint32_t now = _millis();
  random();
  while(now<tend)
  {
    analogWrite(greenPin, map(now, tstart, tend, 0, 255));
    slaveServiceQuick();
    now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void strobeBlue(uint32_t tstart, uint32_t tend)
{
  char buf[64];

  uint32_t now = _millis();
  unsigned sections = random()%20+5;
  uint32_t length = (tend-tstart)/sections;

  unsigned thisPin = pins[random()%3];

  while(now<tend)
  {
    if( now%(length*2) < length )
    {
      analogWrite(thisPin, 0);
    }
    else
    {
      analogWrite(thisPin, 255);
    }
//    Serial.println(now%length);
    slaveServiceQuick();
    now = _millis();
    HELPER_BAIL_EARILY;
  }
}

void pickNColorCycle(uint32_t tstart, uint32_t tend)
{
  uint32_t now = _millis();
  unsigned pick_max = 15;
  unsigned max_time = 784;
  unsigned stamps[pick_max];
  
  stamps[0] = tstart;
  for(unsigned i = 1; i < pick_max; i++)
  {
    stamps[i] = (random() % max_time) + stamps[i-1];
    Serial.print(stamps[i]);
  }
  
  while(now<tend)
  {
    
    slaveServiceQuick();
    now = _millis();
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
#ifdef DEBUG_TX_RX
        sprintf(buf, "now %lu seed %lu end %lu delta %ld\r\n", p.time, p.seed, p.eventEnd, deltaDelta);
        Serial.print(buf);
#endif
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

void printState()
{
  uint32_t _now = _millis();
  uint32_t pick = random();
  
  char buf[64];
  sprintf(buf, "_now = %lu pick %lu start %lu end %lu", _now, pick, eventStart, eventEnd);
  Serial.println(buf);

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
    // this allows us to capture the seed
    rSeed = random();
    randomSeed(rSeed);
    service(rSeed);
    
    switch(rSeed % 3)
    {
      case 0:
        strobeBlue(eventStart, eventEnd);
        break;
      case 1:  
        greenRamp(eventStart, eventEnd);
      break;
      default:
      case 2:
        pickNColorCycle(eventStart, eventEnd);
    }
    
    if(rSeed % 3)
    {
      
    }
    else
    {

    }
    
    analogWrite(bluePin,0);
    analogWrite(redPin,0);
    analogWrite(greenPin,0);
    
    duration(&eventStart, &eventEnd, 4000);  
    //duration(&eventStart, &eventEnd, 100+random()%7777);
    randomSeed(eventEnd);
    
    
#ifdef DEBUG_PRINT_STATE
    printState();
    printState();
    printState();
    Serial.print("\r\n\r\n");
#endif

  } // while (1)
} // loop
