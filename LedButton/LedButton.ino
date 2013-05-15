/*
 Receives from the hardware Serial0, blinks LED, sends back to Serial0
 
 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example
 
 This example code is in the public domain.
 
 */
#define BAUD_RATE 9600
const int FALSE = 0;
const int TRUE = 1;
int led = 13;
int ledState = 0;
int justGot = 0;

#define CMD_BUFF_LEN 128
char cmdBuff[CMD_BUFF_LEN];
int cmdIndex = 0;

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

  cmdIndex = 0;
}

// TRUE => a complete command has been collected
bool collectCmd()
{
  char c;
  bool ret = FALSE;
  if (Serial.available()) {
    c = Serial.read();
    if (('\r' == c) || ('\n' == c)) {
      c = 0;
      if (CMD_BUFF_LEN <= cmdIndex) cmdIndex = CMD_BUFF_LEN - 1;
      if (cmdIndex > 0) {
        cmdBuff[cmdIndex++] = 0;
        ret = TRUE;
      }
    } else {
      if (CMD_BUFF_LEN > cmdIndex) cmdBuff[cmdIndex++] = c;
    }
  }
  return ret;
}

// TRUE => an error was detected
bool dispatchCmd()
{
  bool ret = FALSE;
  switch (cmdBuff[0]) {
  case 'o': // Turn LED off
    ledState = 0;
    digitalWrite(led, LOW);
    break;
  case 'O': // Turn LED ON
    ledState = 1;
    digitalWrite(led, HIGH);
    break;
  case 't': // Toggle the LED
    ledState++;
    digitalWrite(led, ((ledState&1)?HIGH:LOW));
    break;
  default:
    ret = TRUE;
  }
  return ret;
}

void loop() // run over and over
{
  if (collectCmd()) {
    Serial.println(cmdBuff);
    delay(200);               // wait for a moment
    if (dispatchCmd()) {
      Serial.println("*** ERROR: Bad cmd\n");
    } else {
      Serial.println("OK\n");
    }
    cmdIndex = 0;
  }
}

