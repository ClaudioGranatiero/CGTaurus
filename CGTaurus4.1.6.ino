/*
CGtaurus4

We are using KeyPad library to control the 13 pedals of a Fatar PD/3 pedalboard and 7 arcade buttons (1,2,3,4,5,P,N).
We cannot use the more logical configuration of 3 rows x 8 columns, because the Fatar has diodes, so we must reverse the logic and use 8 rows x 3 columns.
Column "1" is the one controlling the buttons, "2" and "3" controls the pedals.
The device can be controlled and configured with some sequence of buttons or by pressing a pedal or a button during the startup; the configuration is saved on EEPROM and restored on startup.
The device works in 2 modes: "HOLD" and "POLY" (the default). In POLY mode every press of a pedal is a NoteOn and the release of the pedal sends a NoteOff. In "HOLD" mode the NoteOff is sent only pressing the higher pedal (High C), so in this mode the notes available to be played are only 12, not 13. The HOLD mode can be useful if you need to maintain a note running without keeping the pedal pressed (so you can use your foot for other actions). Just remember to press the NoteOff pedal at the end.
The 7 buttons emits a CC or a note (specified in the configuration) on a specific (configurable) midi channel.
During runtime you can enter some configuration pressing simultaneously the "N" botton (the rightmost one) and another button:
- N+1: reset and enter immediately the offline setup (see later)
- N+2: factory default
- N+3: set button actions to Note or CC (default to CC)
- N+4: CC midi channel 14
- N+5: CC midi channel 15
- N+P: CC midi channel 16 (default)

Offline setup:
during startup, pressing a button or a pedal changes some configuration:
- Button "1": toggle between "HOLD" and "POLY" (default) Mode
- Button "2": note offset: 0 (the "C" leftmost note is midi note 0)
- Button "3": note offset 12 (+1 octave)
- Button "4": note offset 24 (+2 octave)
- Button "5": note offset 36 (+3 octave)
- Button "P": note offset 48 (+4 octave) [default]
- Button "N": note offset 60 (+5 octave)
- pedal "C" : Note Midi channel: 1 [default]
- pedal "C#": Note Midi channel: 2
- pedal "D" : Note Midi channel: 3
- pedal "D#": Note Midi channel: 4
- pedal "E" : Note Midi channel: 5
- pedal "F" : Note Midi channel: 6
- pedal "F#": Note Midi channel: 7
- pedal "G" : Note Midi channel: 8
- pedal "G#": Note Midi channel: 9
- pedal "A" : Note Midi channel: 10
- pedal "A#": Note Midi channel: 11
- pedal "B" : Note Midi channel: 12
- pedal "C" (high) : Note Midi channel: 13

The sketch is loaded on an Arduino Micro Pro (chinese equivalent of an Arduino Leonardo), set as "TeeOnArdu" (https://github.com/adafruit/TeeOnArdu). This restrict the usage of Arduino IDE to the old 1.0.6 version, because more recent versions doesn't support TeeOnArdu. The code could maybe ported to MIDIUSB library (official and currently supported).
To load the sketch on the Micro Pro you need to press the reset button as soon as the loading starts, because the usb port is set to "MIDI device".

Pins on Pro Micro:
                   --------------------
         (COL1)   |1                RAW|
         (COL2)   |0                GND|
                  |GND              RST|
                  |GND              VCC|
          ROW1    |2              A 21 |    LEDS
          ROW2    |3  P           A 20 |    LED
          ROW3    |4 A            A 19 |
          ROW4    |5  P           A 18 |
          ROW5    |6 AP             15 |          -SLCK-
          ROW6    |7                14 |   (COL1) -MISO-
          ROW7    |8 A              16 |   (COL2) -MOSI-
          ROW8    |9 AP          PA 10 |    COL3
                   --------------------

History:
CGTaurus 4.1: pulizia codice
CGTaurus 4.1.2: refactoring, aggiunto setup iniziale tramite pressione pedali, reset (P7+P5), implementata modalità "Sustain" (Off con P5)
CGTaurus 4.1.3: uso il Do alto per fermare il Sustain (al posto di P5)
CGTaurus 4.1.4: opzione NoteOn su pulsanti 
CGTaurus 4.1.5: gestione led di status (8 led) [WIP]; se teniamo liberi i pin 15,16,14 possiamo usare la SPI per un display LCD
CGTaurus 4.1.6: commenti e documentazione 
*/
#define MODEL01
#include <Keypad.h>
#include <EEPROMex.h>

#define POLY 1
#define SUST 2

#define MAGIC "CGT1"

# default values
byte MidiChannelNote=1;
byte MidiChannelCC=16;
byte NoteOffset=48;
byte PlayMode = POLY;
boolean ButtonCCMode=true;
int LED = 20;
int CurrentNote = -1;
int ControlPressed = 0;

const byte ROWS = 8; // how many rows
const byte COLS = 3; // how many columns
char keys[ROWS][COLS] = {
{'1','a','A'},
{'2','b','B'},
{'3','c','C'},
{'4','d','D'},
{'5','e','E'},
{'6','f','F'},
{'7','g','G'},
{'8','h','H'}
};
byte rowPins[ROWS] = {9,8,7,6,5,4,3,2}; //connect to the row pinouts of the kpd

#ifdef MODEL01
byte colPins[COLS] = {10,1,0}; //connect to the column pinouts of the kpd
#else
byte colPins[COLS] = {10,16,14}; //connect to the column pinouts of the kpd
#endif
byte ledsPin = 21; // this controls 8 leds connected to rowPins output

byte leds=0;

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void(*reset)(void)=0; // declare reset function at address 0

void SetLed(byte pos,boolean val)
// prepare to set on or off the specific led in array (actual operation in "updateLeds()")
{
    if(val)
        leds |= (2 << pos);
    else
        leds &= ~(2 << pos);
}

void updateLeds()
// set on or off the leds (as specified in the "leds" variable)
{
    digitalWrite(ledsPin,HIGH); //enable the leds
    for(int i=0; i < 8; i++)
    {
       pinMode(rowPins[i],OUTPUT);
       if(leds & (2 << i))
           digitalWrite(rowPins[i],LOW);
       else
           digitalWrite(rowPins[i],HIGH);
        
    }
    digitalWrite(ledsPin,LOW); //disable the leds control
}

void FactoryDefault()
{
  MidiChannelNote=1;
  MidiChannelCC=16;
  NoteOffset=48;
  PlayMode = POLY;
  ButtonCCMode=true;
  SaveConfig();
}

boolean SaveConfig()
{
  char Magic[4];
  strcpy(Magic,MAGIC);
  EEPROM.updateByte(4,MidiChannelNote);
  EEPROM.updateByte(5,MidiChannelCC);
  EEPROM.updateByte(6,NoteOffset);
  EEPROM.updateByte(7,PlayMode);
  EEPROM.updateByte(8,ButtonCCMode);

  EEPROM.updateBlock<char>(0, Magic, 4);
  return true;
}

boolean LoadConfig()
// see EEPROMex documentation
{
  char Magic[4];
  EEPROM.readBlock<char>(0, Magic, 4);
  if (strncmp(Magic,MAGIC,4) == 0)
  {
    MidiChannelNote=EEPROM.readByte(4);
    MidiChannelCC=EEPROM.readByte(5);
    NoteOffset=EEPROM.readByte(6);
    PlayMode = EEPROM.readByte(7);
    ButtonCCMode = EEPROM.readByte(8);

    if(PlayMode == SUST)
      SetLed(1,1);
    else
      SetLed(1,0);
    SetLed(2,!ButtonCCMode);
    return true;
  }
  return false;
}

void MidiReset()
{
  for(int j=0; j<=127;j++)
    usbMIDI.sendNoteOn(j,0,MidiChannelNote);
}

void LedOn()
{
  digitalWrite(LED,LOW);
  digitalWrite(ledsPin,HIGH); 
}

void LedOff()
{
  digitalWrite(LED,HIGH);  
   digitalWrite(ledsPin,HIGH); 
}

void LedBlink(int ms)
{
  LedOff();
  delay(ms);
  LedOn();
  delay(ms);
  LedOff();
}

void setup()
{
  pinMode(LED,OUTPUT);
  pinMode(ledsPin,OUTPUT);
  LoadConfig();
  Initialize();
}

void Initialize() {

  int note = -1;
  LedBlink(250);
  delay(1000);
  LedBlink(100);
  
  // if a button or a pedal is pressed at startup, change configuration
  char k = kpd.getKey();
  if (k)
  {
    LedBlink(100);
    delay(200);
    LedBlink(100);
    delay(200);
    LedBlink(100);
    delay(200);    
  
    note = Value(k);
  }
  if (note != -1)
  {
    LedBlink(500);
    switch(note)
    {
        // Setup Configuration
      case 101:
        if(PlayMode == POLY)
        {
          PlayMode = SUST;
          SetLed(1,1);
        }
        else
        {
          PlayMode = POLY;
          SetLed(1,0);
        }
        break;
      case 102:
        NoteOffset = 0;
        break;
      case 103:       
        NoteOffset = 12;
        break;
      case 104:
        NoteOffset = 24;      
        break;
      case 105:       
        NoteOffset = 36;
        break;
      case 109:
        NoteOffset = 48;
        break;
      case 110:
        NoteOffset = 60;
        break;
      case 0:
        MidiChannelNote = 1;
        break;
      case 1:
        MidiChannelNote = 2;
        break;
      case 2:
        MidiChannelNote = 3;
        break;
      case 3:       
        MidiChannelNote = 4;
        break;
      case 4:       
        MidiChannelNote = 5;
        break;
      case 5:       
        MidiChannelNote = 6;
        break;
      case 6:       
        MidiChannelNote = 7;
        break;
      case 7:       
        MidiChannelNote = 8;
        break;
      case 8:       
        MidiChannelNote = 9;
        break;
      case 9:       
        MidiChannelNote = 10;
        break;
      case 10:       
        MidiChannelNote = 11;
        break;
      case 11:       
        MidiChannelNote = 12;
        break;
      case 12:       
        MidiChannelNote = 13;
        break;
    }
    SaveConfig();
  }
}


void loop() {
  Scan();
  updateLeds();
}


int Value(char pedal)
{
  int note = -1;
  switch(pedal) {
      case '1':
        note = 101; // Pulsante "1"
        break;                    
      case '2':
        note = 102; // Pulsante "2"
        break;
      case '3':
        note = 103; // Pulsante "3"
        break;
      case '4':
        note = 104; // Pulsante "4"
        break;
      case '5':
        note = 105; // Pulsante "5"
        break;
      case '6':
        note = 109; // Pulsante "P"
        break;
      case '7':
        note = 110; // Pulsante "N"
        break;                                        
      case 'A':
        note = 0; // Do
        break;
      case 'C':
        note = 1; // Do#
        break;
      case 'E':
        note = 2; // Re
        break;
      case 'G':
        note = 3; // Re#
        break;
      case 'B':
        note = 4; // Mi
        break;           
      case 'D':          
        note = 5; // Fa
        break;           
      case 'F':          
        note = 6; // Fa#
        break;           
      case 'H':          
        note = 7; // Sol
        break;           
      case 'a':          
        note = 8; // Sol#
        break;           
      case 'c':          
        note = 9; // La
        break;           
      case 'e':          
        note = 10;// La#
        break;           
      case 'g':          
        note = 11; // Si
        break;
      case 'b':
        note = 12; // Do (alto)
        break;
    }  
    return(note);
}

int Scan() {
    // Fills kpd.key[ ] array with up-to 10 active keys.
    // Returns true if there are ANY active keys.
    int note = -1;
    if (kpd.getKeys())
    {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( kpd.key[i].stateChanged)   // Only find keys that have changed state.
            {
                int val = -1;
                note = -1;

                switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case PRESSED:
                    LedOn();
                    val = 255;
                break;
                    case HOLD:
                break;
                    case RELEASED:
                    LedOff();
                    if (PlayMode != SUST)
                      val = 0;
                break;
                    case IDLE:
                break;
                }
                if (val != -1)
                {
                  note=Value(kpd.key[i].kchar); 
                 
                  if(note != -1)
                  {
                    if (note >= 100)
                    {
                      if(ButtonCCMode || (note >= 109))
                        usbMIDI.sendControlChange(note-100, val, MidiChannelCC);
                      else
                          usbMIDI.sendNoteOn(note-100,val,MidiChannelCC);
                      if (note == 110) 
                        ControlPressed=val;
                      if (ControlPressed) // Se è stato premuto "N"...
                      {
                        if ((note == 101) && val)  // Control (P7) + P1: reset and setup
                          {
                            MidiReset();
                            reset();
                          }
                        if ((note == 102) && val) // Control (P7) + P2: Factory defaults
                        {
                          FactoryDefault();
                        }
                        if ((note == 109) && val) // Control (P7) + P6: CC Channel 16
                        {
                          MidiChannelCC = 16;
                          SaveConfig();
                        }
                        if ((note == 105) && val) // Control (P7) + P5: CC Channel 15
                        {
                          MidiChannelCC = 15;
                          SaveConfig();
                        }
                        if ((note == 104) && val) // Control (P7) + P4: CC Channel 14
                        {
                          MidiChannelCC = 14;
                          SaveConfig();
                        }
                        if ((note == 103) && val) // Control (P7) + P3: CC/Note
                        {
                          if (ButtonCCMode)
                          {
                              ButtonCCMode=false;
                          }
                          else 
                          {
                              ButtonCCMode=true;
                          }
                          SetLed(2,!ButtonCCMode);
                          SaveConfig();
                        }
                      }
                    }
                    else
                    {
                      if (PlayMode == SUST)
                        usbMIDI.sendNoteOn(CurrentNote+NoteOffset,0,MidiChannelNote);

                      if((note == 12) && (PlayMode == SUST))
                          for(int j=0; j<=12;j++)
                            usbMIDI.sendNoteOn(j+NoteOffset,0,MidiChannelNote);
                      else
                          usbMIDI.sendNoteOn(note+NoteOffset,val,MidiChannelNote);
                      if(val)
                        CurrentNote = note;
                      else
                        CurrentNote = -1;
                    }
                  }
               }
            }
        }
    }
    usbMIDI.read();

    return(note);
}  // End loop
