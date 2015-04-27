/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:

const int masterPin1 = 7;
const int masterPin2 = 8;

const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int pins[3] = {redPin, greenPin, bluePin};
unsigned master = 1;
unsigned long eventStart;
unsigned long eventEnd;
unsigned long nextEventStart;
unsigned long nextEventEnd;

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
  
//  Serial.print("tied together: ");
//  Serial.println(tied);
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
  setMaster();
}

void greenRamp(unsigned long tstart, unsigned long tend)
{
  unsigned long now = millis();
  while(now<tend)
  {
    analogWrite(greenPin, map(now, tstart, tend, 0, 255));
    now = millis();
  }
}

void strobeBlue(unsigned long tstart, unsigned long tend)
{
  char buf[64];

  unsigned long now = millis();
  unsigned sections = random()%20+5;
  unsigned long length = (tend-tstart)/sections;
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
    now = millis();
  }
}

void duration(unsigned long *tstart, unsigned long *tend, unsigned long bump)
{
  *tstart = *tend;
  *tend = *tend + bump;
}

// the loop routine runs over and over again forever:
void loop() {  
  unsigned r = random();
  unsigned val = random();
  unsigned long now = millis();
  
  unsigned long ramp_start = 0;
  unsigned long ramp_end = 4000;
  
  while(1)
  {
  greenRamp(eventStart, eventEnd);
  analogWrite(greenPin,0);
  delay(1);
  duration(&eventStart, &eventEnd, 100+random()%7777);

    
  strobeBlue(eventStart, eventEnd);
  analogWrite(bluePin,0);
  delay(1);
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
