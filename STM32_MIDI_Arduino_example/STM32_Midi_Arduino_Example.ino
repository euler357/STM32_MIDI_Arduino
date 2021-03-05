/***********************************/
/* STM32 MIDI Arduino Example      */
/***********************************/
/* Copyright uBld Electronics, LLC */
/***********************************/

/* Tested on STM32 MIDI Arduino by uBld Electronics */
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

/* File handle for microSD */
File myFile;

/* Need this line for using I2C on STM32 PB9, PB8 */
TwoWire Wire2(PB9, PB8);

/* Adafruit OLED library */
Adafruit_SSD1306 display(128, 32, &Wire2, -1);

/* Keypad Pins */
#define KEYPAD_R1 PA0
#define KEYPAD_R2 PA1
#define KEYPAD_R3 PA2
#define KEYPAD_R4 PA3
#define KEYPAD_C1 PA4
#define KEYPAD_C2 PA5
#define KEYPAD_C3 PA6
#define KEYPAD_C4 PA7

/* Keypad State */
byte keypad_state=0xff;

/* Rotary Encoder State */
byte encoder_phaseA = 0;
byte encoder_phaseB = 0;
byte encoder_switch = 0;
byte encoder_cw_count=0;
byte encoder_ccw_count=0;

/* Brightness of LEDs */
byte ws2812brightness = 10;

/* Data pin to WS2812Bs */
#define WS2812_DATA_PIN   PC14

/* Bit Banging and Delays for 72MHz STM32F103 */
#define DELAY10     __asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop"); \
                    __asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop")
#define DELAY400nS  DELAY10;DELAY10;__asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop");
#define DELAY800nS  DELAY10;DELAY10;DELAY10;DELAY10;DELAY10;__asm__("nop");__asm__("nop");__asm__("nop"); 
#define SEND_WS2812_ONE   GPIOC->BSRR=0x00004000;DELAY800nS;GPIOC->BSRR=0x40000000;DELAY400nS
#define SEND_WS2812_ZERO  GPIOC->BSRR=0x00004000;DELAY400nS;GPIOC->BSRR=0x40000000;DELAY800nS

/* Number of WS2812B LEDS in the string */
#define NUMBER_WS2812B_LEDS 8

/* Buffer for WS2812B LEDs */
/* Each is 24-bit color */
/* Each LED gets 3 bytes of Green/Red/Blue in that order */
/* 0xGG 0xRR 0xBB */
byte ws2812_grb_buffer[NUMBER_WS2812B_LEDS * 3];

/* Lenght of notes */
#define BASE      25      /* Milliseconds */
#define WHOLE     BASE*16
#define HALF      BASE*8
#define QUARTER   BASE*4
#define EIGHTH    BASE*2
#define SIXTEENTH BASE

/* MIDI Note Definition */
#define mC3    48
#define mC3s   49
#define mD3    50
#define mD3s   51
#define mE3    52
#define mF3    53
#define mF3s   54
#define mG3    55
#define mG3s   56
#define mA3    57
#define mA3s   58
#define mB3    59
#define mC4    60
#define mC4s   61
#define mD4    62
#define mD4s   63
#define mE4    64
#define mF4    65
#define mF4s   66
#define mG4    67
#define mG4s   68
#define mA4    69
#define mA4s   70
#define mB4    71

/* Variables for MIDI */
byte MIDIcommand, MIDIdata1, MIDIdata2;
byte MIDIdataBytes=1;
byte MIDIbyteCounter=0;
byte MIDIpacketSize=3;

/* Global Variables for Main Loop */
int loopcounter=0;    /* slow loop counter */
int loopcounter2=0;   /* fast loop counter */
int ledtoggle=0;      /* LED toggle */
int loops=0;          /* Total loops since start */

/* Send Buffer to WS2812B String */
/* 3 bytes per LED */
/* 0xGG 0xBB 0xRR */
/* Length should be in Bytes (i.e. multiples of 3) */
void send_WS2812b(byte *grb_buffer, int length)
{
  byte *buff_ptr=grb_buffer;

  /* Disable Interrupts to keep from getting unpredictable delays in this stream */
  noInterrupts();

  /* Send one byte at a time to the WS2812Bs */
  for (int n=0;n<length;n++)
  {
    if(*buff_ptr   & 0x80) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x40) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x20) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x10) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x08) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x04) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr   & 0x02) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
    if(*buff_ptr++ & 0x01) { SEND_WS2812_ONE; } else { SEND_WS2812_ZERO; }
  }

  /* Re-enable Interrupts */
  interrupts();
}

/* Reads keypad and returns key pressed */
/* Value = 0xRC */
/* Example: Row 1, Col 3 gives 0x13 */
/* Returns 0xff if no key pressed */
byte read_keypad(void)
{
  /* Set pin modes */
  pinMode(KEYPAD_C1, INPUT_PULLUP);
  pinMode(KEYPAD_C2, INPUT_PULLUP);
  pinMode(KEYPAD_C3, INPUT_PULLUP);
  pinMode(KEYPAD_C4, INPUT_PULLUP);

  /* Scan Row 1 */
  pinMode(KEYPAD_R1, OUTPUT);
  pinMode(KEYPAD_R2, INPUT_PULLUP);
  pinMode(KEYPAD_R3, INPUT_PULLUP);
  pinMode(KEYPAD_R4, INPUT_PULLUP);
  digitalWrite(KEYPAD_R1, LOW);
  if(!digitalRead(KEYPAD_C1))
    return 0x11;
  if(!digitalRead(KEYPAD_C2))
    return 0x12;
  if(!digitalRead(KEYPAD_C3))
    return 0x13;
  if(!digitalRead(KEYPAD_C4))
    return 0x14;

  /* Scan Row 2 */
  pinMode(KEYPAD_R1, INPUT_PULLUP);
  pinMode(KEYPAD_R2, OUTPUT);
  pinMode(KEYPAD_R3, INPUT_PULLUP);
  pinMode(KEYPAD_R4, INPUT_PULLUP);
  digitalWrite(KEYPAD_R2, LOW);
  if(!digitalRead(KEYPAD_C1))
    return 0x21;
  if(!digitalRead(KEYPAD_C2))
    return 0x22;
  if(!digitalRead(KEYPAD_C3))
    return 0x23;
  if(!digitalRead(KEYPAD_C4))
    return 0x24;

  /* Scan Row 3 */
  pinMode(KEYPAD_R1, INPUT_PULLUP);
  pinMode(KEYPAD_R2, INPUT_PULLUP);
  pinMode(KEYPAD_R3, OUTPUT);
  pinMode(KEYPAD_R4, INPUT_PULLUP);
  digitalWrite(KEYPAD_R3, LOW);
  if(!digitalRead(KEYPAD_C1))
    return 0x31;
  if(!digitalRead(KEYPAD_C2))
    return 0x32;
  if(!digitalRead(KEYPAD_C3))
    return 0x33;
  if(!digitalRead(KEYPAD_C4))
    return 0x34;

  /* Scan Row 4 */
  pinMode(KEYPAD_R1, INPUT_PULLUP);
  pinMode(KEYPAD_R2, INPUT_PULLUP);
  pinMode(KEYPAD_R3, INPUT_PULLUP);
  pinMode(KEYPAD_R4, OUTPUT);
  digitalWrite(KEYPAD_R4, LOW);
  if(!digitalRead(KEYPAD_C1))
    return 0x41;
  if(!digitalRead(KEYPAD_C2))
    return 0x42;
  if(!digitalRead(KEYPAD_C3))
    return 0x43;
  if(!digitalRead(KEYPAD_C4))
    return 0x44;

  /* Return 0xff if we have no buttons pressed */
  return 0xff;
}

/* Checks rotary encoder */
/* Returns 0 for no change */
/* Returns 1 for CW */
/* Returns 2 for CCW */
/* Returns 3 for SW press */
byte get_rotary_encoder(void)
{
  byte tempA, tempB, tempSW;
  byte retval = 0;
  
  /* Get rotary encoder state */
  tempSW = digitalRead(PA8);
  tempA  = digitalRead(PB0);
  tempB  = digitalRead(PB1);

  /* If we have a switch change */
  if(tempSW != encoder_switch)
  {
    if(!tempSW)
      retval=3;
  }

  /* If we have a phase change */
  if((tempA != encoder_phaseA) || (tempB != encoder_phaseB))
  {
    /* If we're in state 0 */
    if(!encoder_phaseA & !encoder_phaseB)
    {
        if(tempB) /* State 3 */
        {
          encoder_ccw_count++;
          encoder_cw_count=0;
        }
        else /* State 1 */
        {
          encoder_cw_count++;
          encoder_ccw_count=0;
        }
    }
    
    /* If we're in state 1 */
    if(encoder_phaseA & !encoder_phaseB)
    {
        if(tempB) /* State 2 */
        {
          encoder_cw_count++;
          encoder_ccw_count=0;
        }
        else /* State 0 */
        {
          encoder_ccw_count++;
          encoder_cw_count=0;
        }
    }
    
    /* If we're in state 2 */
    if(encoder_phaseA & encoder_phaseB)
    {
        if(tempB) /* State 3 */
        {
          encoder_cw_count++;
          encoder_ccw_count=0;
        }
        else /* State 1 */
        {
          encoder_ccw_count++;
          encoder_cw_count=0;
        }
    }
    
    /* If we're in state 3 */
    if(!encoder_phaseA & encoder_phaseB)
    {
        if(tempB) /* State 2 */
        {
          encoder_ccw_count++;
          encoder_cw_count=0;
        }
        else /* State 1 */
        {
          encoder_cw_count++;
          encoder_ccw_count=0;
        }
    }
  }

  /* The encoder we tested has 4 states per click/detent */
  if(encoder_cw_count>=4)
  {
    encoder_cw_count=0;
    encoder_ccw_count=0;
    retval = 1;
  }
  
  /* The encoder we tested has 4 states per click/detent */
  if(encoder_ccw_count>=4)
  {
    encoder_cw_count=0;
    encoder_ccw_count=0;
    retval = 2;
  }
  
  /* Save current state */
  encoder_phaseA = tempA;
  encoder_phaseB = tempB;
  encoder_switch = tempSW;

  return retval;
}

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
  pinMode(WS2812_DATA_PIN, GPIO_MODE_OUTPUT_PP); /* WS2812 LED Data */
  pinMode(PA8, INPUT_PULLUP); /* Rotary Encoder Switch */
  pinMode(PB0, INPUT_PULLUP); /* Rotary Encoder Phase A */
  pinMode(PB1, INPUT_PULLUP); /* Rotary Encoder Phase B */
  pinMode(PB8, OUTPUT); /* OLED SCL */
  pinMode(PB9, OUTPUT); /* OLED SDA */
  pinMode(PB6, INPUT); /* Chip Detect */ 

  /* Make sure that GPIO Port C is at 50MHz Clock rate */
  /* Need this speed to bit bang the WS2812Bs */
  GPIOC->CRH = (GPIOC->CRH | 0x33333333);
  
  /* Set up serial port */
  /* On the Leonardo, Serial1 is on pins 0 & 1 */
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

  /* Open the SD card and write a test file to it */
  if (!digitalRead(PB6))
  {
    Serial.println("Card Inserted - Initializing microSD Card");
      
    /* Remap SPI to connected PINS */
    SPI.setMOSI(PB5);
    SPI.setMISO(PB4);
    SPI.setSCLK(PB3);
  
    if (!SD.begin(PB7)) 
    {
      Serial.println("initialization failed!");
    }
    else
      Serial.println("initialization success");
  
    myFile = SD.open("test.txt", FILE_WRITE);
    if (myFile) {
      Serial.print("Writing to test.txt...");
      myFile.println("This is a test file :)");
      for (int i = 0; i < 65536; i++) {
        myFile.println(i);
        if(!(i&0x3ff))
          Serial.printf("%d Values Written\n",i);
        
      }
      // close the file:
      myFile.close();
      Serial.println("done.");
    } 
    else 
    {
      Serial.println("error opening test.txt");
    }  
  }
  else
    Serial.println("No microSD Card Inserted");
    
  
  /* Get rotary encoder state */
  encoder_switch = digitalRead(PA8);
  encoder_phaseA = digitalRead(PB0);
  encoder_phaseB = digitalRead(PB1);
  
  /* Initialize SSD1306 Display */
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println("Can't Initialize SSD1306\n");
  else
    Serial.println("SSD1306 Initialized");
    
  /* Clear Display Buffer */
  display.clearDisplay();

  /* Set Size of Font */
  display.setTextSize(1);

  /* Color = lit */
  display.setTextColor(1);

  /* Top Left */
  display.setCursor(2, 0);

  /* Print uBld Name */
  display.print("uBld Electronics, LLC");
  
  /* Top Left */
  display.setCursor(20, 14);

  /* Set Size of Font to bigger */
  display.setTextSize(2);

  /* Print uBld Name */
  display.print("ubld.it");

  /* Write display buffer to OLED */
  display.display();
  
  /* Outputs notes to MIDI out at startup */
  for (int notes = 50; notes < 72; notes ++) 
    playNote(notes,0x7f,SIXTEENTH);

  delay(500);

  /* Shave and a Haircut */
  playNote(mC4,0x7f,QUARTER);
  playNote(mG3,0x7f,EIGHTH);
  playNote(mG3,0x7f,EIGHTH);
  playNote(mG3s,0x7f,QUARTER);
  playNote(mG3,0x7f,QUARTER);
  playNote(00,00,QUARTER);
  playNote(mB3,0x7f,QUARTER);
  playNote(mC4,0x7f,QUARTER);
  
  /* Clears out any junk that may be in the receive buffer so we start clean */
  Serial1.flush();
}

/* Plays a MIDI note over the MIDI output */
void playNote(byte note, byte velocity, byte notetime)
{
    MIDIcommand=0x90; /* Note On */
    MIDIdata1=note;  /* Pitch */
    MIDIdata2=velocity;     /* Velocity */
    sendMIDI(3);      /* Send MIDI Bytes */
    delay(notetime);
    MIDIdata2=0;     /* Velocity */
    sendMIDI(3);      /* Send MIDI Bytes */
    delay(notetime);
}

/* Send MIDI Message */
/* From byte variables */
void sendMIDI(byte sendBytes) 
{
  if(sendBytes>=1)
    Serial1.write(MIDIcommand);
  if(sendBytes>=2)
    Serial1.write(MIDIdata1);
  if(sendBytes>=3)
    Serial1.write(MIDIdata2);
}

/* Send MIDI Message */
/* From byte variables */
void printMIDI(byte sendBytes) 
{
  Serial.print("MIDI: 0x");
  if(sendBytes>=1)
  {
    Serial.print(MIDIcommand,HEX);
  }
  if(sendBytes>=2)
  {
    Serial.print(" : 0x");
    Serial.print(MIDIdata1,HEX);
  }
  if(sendBytes>=3)
  {
    Serial.print(" : 0x");
    Serial.print(MIDIdata2,HEX);
  }
  Serial.print("\n");
}

/* Receive MIDI Message */
/* Returns packet size received (1 to 3) */
int recvMIDI(void) 
{
  byte tempByte;

  /* While we have a byte in the serial receive buffer */
  if(Serial1.available())
  { 
    /* Get a byte from the serial port buffer */
    tempByte=Serial1.read();

    /* If this is a command byte */
    if(tempByte & 0x80)
    {
      /* Store command */
      MIDIcommand=tempByte;
      MIDIbyteCounter=1;

      switch(MIDIcommand)
      {
        /* Note Off */
        case 0x80: 
          MIDIdataBytes=2;
          break;

        /* Note On */
        case 0x90: 
          MIDIdataBytes=2; 
          break;

        /* Polymorphic key pressure / Aftertouch */
        case 0xA0: 
          MIDIdataBytes=2; 
          break;

        /* Control change */
        case 0xB0: 
          MIDIdataBytes=2; 
          break;

        /* Program Change */
        case 0xC0: 
          MIDIdataBytes=1;
          break;

        /* Channel pressure / Aftertouch */
        case 0xD0: 
          MIDIdataBytes=1;
          break;

        /* Pitch bend change */
        case 0xE0: 
          MIDIdataBytes=2; 
          break;

        /* System Messages */
        case 0xF0: 

          /* System Real Time */
          if(tempByte&0x08)
          {
            MIDIdataBytes=0; 
            return 1;
          }
          else
          {
            /* System Exclusive */
            /* This needs to be fixed if your system uses this command */
            MIDIdataBytes=2; 
            /* return 1; */
            /* System Common */
          }
          break;
      }
    }
    /* This is a data byte */
    else
    {
      if(MIDIbyteCounter <= MIDIdataBytes)
      {
        /* If one byte has been received (command byte) */
        if(MIDIbyteCounter==1)
        {
          MIDIdata1=tempByte;

          if(MIDIbyteCounter==MIDIdataBytes)
            return 2;
        }
    
        /* If two bytes have been received (command and one data) */
        if(MIDIbyteCounter==2)
        {
          MIDIdata2=tempByte;

          if(MIDIbyteCounter==MIDIdataBytes)
            return 3;
        }
      }
      MIDIbyteCounter++;
    }
  }
  
  /* We don't have a message ready */
  return false;
}

/* Main Loop */
void loop() {
  int temp,temp2,temp3;
  byte tempval;
  temp = recvMIDI();

  loops++;
  
  /* Every 100,000 loops */
  if(loopcounter++>100000)
  {
    loopcounter=0;
    toggle_led();
    tempval=read_keypad();
    if(tempval != keypad_state)
    {
      if(tempval != 0xff)
      {
        Serial.printf("Keypad: %02X pressed\n",tempval);
        display.invertDisplay(1);
      }
      else
      {
        Serial.printf("Keypad: %02X released\n", keypad_state);
        display.invertDisplay(0);
      }
      keypad_state=tempval;
    }

    temp3=0;
    /* Flash the LEDs randombly */
    for(int n=0;n<NUMBER_WS2812B_LEDS;n++)
    { 
      ws2812_grb_buffer[temp3++]=random(ws2812brightness);  /* Green */
      ws2812_grb_buffer[temp3++]=random(ws2812brightness);  /* Red */
      ws2812_grb_buffer[temp3++]=random(ws2812brightness);  /* Blue */
    }
    send_WS2812b(ws2812_grb_buffer, NUMBER_WS2812B_LEDS*3);
  }

  /* Every 1000 loops */
  if(loopcounter2++>1000)
  {
    tempval=get_rotary_encoder();
    if(tempval==1)
    {
      Serial.printf("Encoder CW %X\n", loops);
      if(ws2812brightness >= 250)
        ws2812brightness=255;
      else
        ws2812brightness+=5;
    }
    if(tempval==2)
    {
      Serial.printf("Encoder CCW %X\n", loops);
      if(ws2812brightness <= 7)
        ws2812brightness=2;
      else
        ws2812brightness-=5;
    }
    if(tempval==3)
    {
      Serial.printf("Encoder Switch Pressed %X\n", loops);


#if 1 /* Dixie */
      playNote(mB4,0x7f,EIGHTH);
      playNote(mG3s,0x7f,EIGHTH);
      playNote(mE3,0x7f,QUARTER);
      playNote(mE3,0x7f,QUARTER);
      playNote(mE3,0x7f,EIGHTH);
      playNote(mF3s,0x7f,EIGHTH);
      playNote(mG3s,0x7f,EIGHTH);
      playNote(mA3,0x7f,EIGHTH);
      playNote(mB3,0x7f,QUARTER);
      playNote(mB3,0x7f,QUARTER);
      playNote(mB3,0x7f,QUARTER);
      playNote(mG3s,0x7f,QUARTER);
#else /* Yankee Doodle */
      playNote(mC4,0x7f,EIGHTH);
      playNote(mC4,0x7f,EIGHTH);
      playNote(mD4,0x7f,EIGHTH);
      playNote(mE4,0x7f,EIGHTH);
      playNote(mC4,0x7f,EIGHTH);
      playNote(mE4,0x7f,EIGHTH);
      playNote(mD4,0x7f,QUARTER);
      
      playNote(mC4,0x7f,EIGHTH);
      playNote(mC4,0x7f,EIGHTH);
      playNote(mD4,0x7f,EIGHTH);
      playNote(mE4,0x7f,EIGHTH);
      playNote(mC4,0x7f,QUARTER);
      playNote(mB3,0x7f,QUARTER);
      
      playNote(mC4,0x7f,EIGHTH);
      playNote(mC4,0x7f,EIGHTH);
      playNote(mD4,0x7f,EIGHTH);
      playNote(mE4,0x7f,EIGHTH);
      playNote(mF4,0x7f,EIGHTH);
      playNote(mE4,0x7f,EIGHTH);
      playNote(mD4,0x7f,EIGHTH);
      playNote(mC4,0x7f,EIGHTH);
      
      playNote(mB3,0x7f,EIGHTH);
      playNote(mG3,0x7f,EIGHTH);
      playNote(mA3,0x7f,EIGHTH);
      playNote(mB3,0x7f,EIGHTH);
      playNote(mC4,0x7f,QUARTER);
      playNote(mC4,0x7f,QUARTER);
#endif
    }
    loopcounter2=0;
  }
  
  /* If we recevied a MIDI packet */
  if(temp>0)
  {
    temp2=MIDIcommand & 0xf0;

    /* Change Middle-C to D */
    /* Example of modifying the stream */
    if ((temp2==0x80)||(temp2==0x90))
      if(MIDIdata1==0x3C)
        MIDIdata1=0x30;

    /* Send the MIDI and print it on the USB monitor port */
    sendMIDI(temp);
    printMIDI(temp);
  }  
}
