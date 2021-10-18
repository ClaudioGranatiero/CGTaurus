# CGTaurus
A midi instrument using an Arduino Micro Pro (or an Arduino Leonardo) and a Fatar PD/3 pedalboard

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
