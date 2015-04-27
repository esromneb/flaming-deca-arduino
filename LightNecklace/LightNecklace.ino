/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:

const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int pins[3] = {redPin, greenPin, bluePin};

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.

  Serial.begin(9600);
  randomSeed(42);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); 
}

// the loop routine runs over and over again forever:
void loop() {  
  unsigned r = random();
  unsigned val = random();
  unsigned long now = millis();
  
  unsigned long ramp_start = 0;
  unsigned long ramp_end = 5000;
  
  analogWrite(greenPin, map(now - ramp_start, ramp_start, ramp_end, 0, 255));
  
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
