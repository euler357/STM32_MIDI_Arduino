/***********************************/
/* STM32 MIDI Arduino Example      */
/***********************************/
/* Copyright uBld Electronics, LLC */
/***********************************/

int ledtoggle=0;      /* LED toggle */

/* Toggle the 2 user LEDs when called */
void toggle_led(void)
{
  if(ledtoggle)
  {
    digitalWrite(PC13,HIGH);
    digitalWrite(PC15,LOW);
    ledtoggle=0;
  }
  else
  {
    digitalWrite(PC13,LOW);
    digitalWrite(PC15,HIGH);
    ledtoggle=1;
  }
}

void setup() {
  /* Set mode for pins */
  pinMode(PC13, OUTPUT); /* LED */
  pinMode(PC15, OUTPUT); /* LED */
 
  /* Make sure that GPIO Port C is at 50MHz Clock rate */
  /* Need this speed to bit bang the WS2812Bs */
  GPIOC->CRH = (GPIOC->CRH | 0x33333333);
  
  /* Set up serial port */
  /* For other Arduino, you may need to set up the port differently */
  Serial1.begin(31250);

  /* Set up USB serial montior on Leonardo */
  Serial.begin(31250);
    for(int i;i<10;i++)
  {
    if(!Serial)
    {
      delay(250);
    }
  }
  
  Serial.println("********** STARTING UP **********");

    /* Clears out any junk that may be in the receive buffer so we start clean */
  Serial1.flush();
}

/* Main Loop */
void loop() {
  byte tempByte=0;
  char text[16];
  /* While we have a byte in the serial receive buffer */
  if(Serial1.available())
  { 
    /* Get a byte from the serial port buffer */
    tempByte=Serial1.read();
    Serial1.write(tempByte);
    Serial.print(tempByte, HEX); /* Write to serial console */
    Serial.write('\n');
    toggle_led();
  }

}
