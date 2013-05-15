/*
 Receives from the hardware Serial0, blinks LED, sends back to Serial0
 
 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example
 
 This example code is in the public domain.
 
 */
#define BAUD_RATE 9600
int led = 13;
int ledState = 0;
int justGot = 0;

void setup()  
{
  // initialize the LED digital pin as an output.
  pinMode(led, OUTPUT);     
  // Open serial communications and wait for port to open:
  Serial.begin(BAUD_RATE);

  delay(1000);  // wait for a second -- otherwise we send some garbage

  Serial.println("Goodnight moon!");

  Serial.println("Hello, world?");
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);               // wait for a second
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
}

void loop() // run over and over
{
  if (Serial.available()) {
    digitalWrite(led, ((ledState++&1)?HIGH:LOW));   // Toggle the LED
    delay(200);               // wait for a moment
    Serial.write(Serial.read());
    justGot = 1;
  } else if (justGot) {
    // Serial.write('\r');
    // Serial.write('\n');
    Serial.println("");
    justGot = 0;
  }
}

