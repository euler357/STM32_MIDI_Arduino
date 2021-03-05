![uBld Electronics, LLC Logo](/images/ublditlogo_color_blue.png)

# STM32_MIDI_arduino
Files for the ubld.it STM MIDI Arduino Board

## To get our board working with Arduino, it needs a bootloader (if you purchased a board from us, this is already installed)
* Burn generic_boot20_pc13.bin to the board using STM32CubeProgrammer, ST-Link V2 (clone), uBld.it ST-Link Clone to 0.050" 2x10 adapter board, and a tag-connect for STM32.
(https://www.tag-connect.com/product/tc2030-ctx-nl-stdc14-for-use-with-stm32-processors-with-stlink-v3)  You can also use st-flash or other STM32F103-compatible SWD programming hw/sw tools.
The bootloader is from: [Roger Clark Melbourne's STM32duino-booloader Project](https://github.com/rogerclarkmelbourne/STM32duino-bootloader) and a working copy of the needed binary is in this repository.
* Set the Arduino software according to the Arduino setting screenshot (Arduino_STM32_Screenshot.jpg)
* Load an example sketch (blink should work) and hit the Upload button.  You may need to reset the board using the reset button if it is waiting.  After the first time, it should program without doing this if you use the recommended Arduino software settings

Note: If you get some Java exception in the Arduino IDE window, it is likely because you have another Java Runtime Environment installed in addition to the one that comes in the Arduino IDE.  You need to update your maple_upload.bat to the one in this directory.  Search your C drive for this file - there may be multiple copies but only one is actually running.  Likely the one in:
Users\Username\AppData\Local\Arduino15\packages\STM32\tools\STM32Tools\1.4.0\tools\win

## Example STM32_MIDI_Ardunio_example.ino sketch 
* Has the following connected
  * 128x32 I2C OLED on J9
  * 4x4 Matrix Keypad on J1
  * Mechanical Quadrature Encoder with Switch on J7
  * microSD (16GB or less) that supports SPI formatted Fat32 or Fat16 inserted
  * Strip of 8 WS2812Bs (NeoPixel) on J11
  * MIDI Synth on MIDI Output
  * MIDI Source (Keyboard) on MIDI Input
  * USB-C connected to PC running Arduino IDE
* The board will do the following
  * Play a MIDI tune at startup
  * Alternating flashing of the 2 user LEDs
  * Put the ubld.it name on the OLED display
  * Flash the WS2812Bs with random colors
  * Pass MIDI from the Input to the Ouput replacing Middle-C with another note
  * Write a test.txt file to the microSD card
  * Adjust the brightness of the WS2812B by turning the mechanical encoder
  * Play a short MIDI tune when the mechanical enoder switch is pressed
  * Invert the display when a key is pressed on the keypad
  * Print status messages to the Arduino serial console
    * MIDI data
    * Button Presses
    * Encoder CW/CCW moves
    * General status messages
