/*
  Software serial multple serial test
 
 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.
 
 The circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 
 Note:
 Not all pins on the Mega and Mega 2560 support change interrupts,
 so only the following can be used for RX:
 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69
 
 Not all pins on the Leonardo support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
 
 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example
 
 This example code is in the public domain.
 
 */

int led = 13;
int ledState = 0;
int justGot = 0;

void setup()  
{
  // initialize the LED digital pin as an output.
  pinMode(led, OUTPUT);     
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

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

