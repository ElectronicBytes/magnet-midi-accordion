//YOUTUBE TUTORIAL - ELECTRONIC BYTES
#include <MIDI.h>

// MIDI TUTORIAL MOD - loop through a bunch of notes and send it over serial.

//OVERRIDE default baud rate -> faster! Required for hairless midi to serial
struct MySettings : public midi::DefaultSettings{
  static const long BaudRate = 115200;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
//MIDI_CREATE_DEFAULT_INSTANCE();

const int velocity = 127; //Max Velocity (range is 0-127)
const int channel = 1; //MIDI Channel 1 (out of 16)
static const unsigned ledPin = 13;      // LED pin on Arduino Uno
const int sensePin = 9;

void setup()
{
    pinMode(ledPin, OUTPUT);
    pinMode(sensePin, INPUT_PULLUP);
    MIDI.begin(1);                      // Launch MIDI and listen to channel 1

    //optional line: double-ensures that we're talking at 115200.
    //If you do this though, make sure you don't send any other text
    //to the serial interface. You may confuse the midi software.
    //Also note that if this is to be done at all, it needs to be after
    //the MIDI.begin(1) line.
    Serial.begin(115200);
    
}
bool trebleStatus[100]; // on/off status for the 41 piano keys
// Play notes from F3
void loop() {
  if(!digitalRead(sensePin) && !trebleStatus[53]){
    MIDI.sendNoteOn(53, velocity, channel);
    trebleStatus[53] = true;
    digitalWrite(ledPin, HIGH);
  }
  if(digitalRead(sensePin) && trebleStatus[53]){
    MIDI.sendNoteOff(53, velocity, channel);
    trebleStatus[53] = false;
    digitalWrite(ledPin, LOW);
  }
}
