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

void setup()
{
    pinMode(ledPin, OUTPUT);
    MIDI.begin(1);                      // Launch MIDI and listen to channel 1

    //optional line: double-ensures that we're talking at 115200.
    //If you do this though, make sure you don't send any other text
    //to the serial interface. You may confuse the midi software.
    //Also note that if this is to be done at all, it needs to be after
    //the MIDI.begin(1) line.
    Serial.begin(115200);
    
}

// Play notes from F3 (53) to A6 (93):
void loop() {
  for (int note = 53; note <= 93; note ++) {
    MIDI.sendNoteOn(note, velocity, channel);
    digitalWrite(ledPin, HIGH);
    delay(250);
    MIDI.sendNoteOff(note, velocity, channel); // Turn the note off.
    digitalWrite(ledPin, LOW);
    delay(250);
  }
}
