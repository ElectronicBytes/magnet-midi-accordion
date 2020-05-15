/*
 *  Electronic Bytes - https://www.youtube.com/channel/UClaih1NqAGCBeOgAWQV12jA
    Based on BLE_MIDI Example by neilbags https://github.com/neilbags/arduino-esp32-BLE-MIDI
    
    MIDI ACCORDION
    Takes input from shift registers that hold the status for each key, and sends note on 
    and note off events over bluetooth.
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

const bool printOutput = false; //accordion might miss notes if you leave print on. Turn it off if you're not debugging.

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

uint8_t midiPacket[] = {
   0x80,  // header
   0x80,  // timestamp, not implemented 
   0x00,  // status
   0x3c,  // 0x3c == 60 == middle c
   0x00   // velocity
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      digitalWrite(21, HIGH);
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      digitalWrite(21, LOW);
      deviceConnected = false;
    }
};

byte prev=0;
byte prev2=0;
byte incoming =0;


class midi{
  public:
  midi(){  }; //constructor does nothing
  
  void noteOn(uint8_t noteIn){
    midiPacketOn[3]=noteIn;
    pCharacteristic->setValue(midiPacketOn, 5); // packet, length in bytes)
    pCharacteristic->notify();
  }
  void noteOff(uint8_t noteIn){
    midiPacketOff[3]=noteIn;
    pCharacteristic->setValue(midiPacketOff, 5); // packet, length in bytes)
    pCharacteristic->notify();
  }
  void updateNotes(byte* notesToTurnOn, byte* notesToTurnOff){
    
    for(int shifteri = 0; shifteri<numShifterToRead; shifteri++){
      int shiftPosCounter=0;
      for(byte j=0x01; j!=0x00; j<<=1){ //for each note on each shifter (stopping at 0x80 misses the last note)
        if(notesToTurnOn[shifteri]&j){
          noteOn(midiNoteDef[shifteri][shiftPosCounter]); 
        }else if(notesToTurnOff[shifteri]&j){
          noteOff(midiNoteDef[shifteri][shiftPosCounter]);
        }
        shiftPosCounter+=1;
      }
    }
  }
  void printByte(byte val){
    //LSB first
    for(byte i=0; i<=7; i++){
      Serial.print(val >> i & 1, BIN); // Magic bit shift, if you care look up the <<, >>, and & operators
    }
    Serial.print("\n"); // Go to the next line, do not collect $200
  }
  private:
  int numShifterToRead=5; //remember to change the shifter one too
  uint8_t midiNoteDef[5][8]={ //notes definiton for each shifter position
    {46,45,49,47,43,44,41,42}, //lowest shifter, orange wires
    {55,53,52,50,54,56,48,51}, //brown wires
    {62,66,63,60,57,58,61,59},//blue wires
    {72,73,69,71,64,65,70,67}, //green wires
    {77,67,74,75,78,79,80,68}  //highest shifetr, assorted wires
  };
  uint8_t midiPacketOn[5] = {
   0x80,  // header
   0x80,  // timestamp, not implemented 
   0x90,  // status
   0x3c,  // 0x3c == 60 == middle c
   127   // velocity
  };
  uint8_t midiPacketOff[5] = {
   0x80,  // header
   0x80,  // timestamp, not implemented 
   0x80,  // status
   0x3c,  // 0x3c == 60 == middle c
   127   // velocity
  };
};
midi myMidi;

class shifter{
  public:
  shifter(midi myMidiIn){
    myMidi=myMidiIn;
    pinMode(shiftPin,OUTPUT);
    pinMode(clkPin,OUTPUT);
    pinMode(clkInhPin,OUTPUT);
    for(int i:shifterPin){
      pinMode(i,INPUT);
    }
    for(int i:directReadPins){
      pinMode(i, INPUT);
    }
    
    //sets up the shifters to default state  
    digitalWrite(shiftPin,1);
    digitalWrite(clkInhPin,1);
    digitalWrite(clkPin, 0);  
  };
  void readShift(){
    for(int i =0;i<5;++i){
      now[i]=0;  
    }
    //latch in the new data
    digitalWrite(shiftPin,0);
    delayMicroseconds(delayTime);
    digitalWrite(shiftPin,1);
    delayMicroseconds(delayTime);
    digitalWrite(clkInhPin, 0);
    delayMicroseconds(delayTime);
    for(int i=0; i<8; ++i){
      for(int j =0; j<numShifterToRead; ++j){
        now[j] |= (digitalRead(shifterPin[j])<<(i));
        delayMicroseconds(delayTime);
      }

      //update next shift value
      digitalWrite(clkPin,1);
      delayMicroseconds(delayTime);
      digitalWrite(clkPin,0);
      delayMicroseconds(delayTime);
    }
    digitalWrite(clkInhPin, 1);
    delayMicroseconds(delayTime);
  }
  void readDirect(){
    //put the individual notes that don't fit on the shifters here

    //both of these bits happen to be the last bit in the byte
    now[3]|=((byte)digitalRead(directReadPins[0])<<7); //pin 34 is G4
    now[4]|=((byte)digitalRead(directReadPins[1])<<7); //pin 35 is A5
  }
  void processShift(){ //gets the on/off status of each note
    //get new data and store previous states
    
    for(int i = 0; i< 5; ++i){
      prev2[i]=prev[i];
      prev[i]=now[i];
    }

    //get new values
    readShift(); //stores new shifted values into "now"
    if(false){
      printByte(now[0]);
      Serial.print("to turn on:");
      printByte(notesToTurnOn[0]);
      Serial.print("to turn off:");
      printByte(notesToTurnOff[0]);
    }

    readDirect();
    //for each shifter determine which notes need to be turned on/off
    for(int i=0; i<numShifterToRead; ++i){
      notesToTurnOff[i]=prev[i]&(~now[i]); //&prev2[i];
      notesToTurnOn[i]=(~prev[i])&now[i]; // &(~prev2[i]);
      /*
      if(notesToTurnOn[i]!=0x0){
        for(int j = 0; j<100; ++j) {Serial.print("adddddddd"); printByte(notesToTurnOn[i]);}
      }
      if(notesToTurnOff[i]!=0x0){
        
        for(int j = 0; j<100; ++j) {Serial.print("fffffffff"); printByte(notesToTurnOff[i]);}
      }
      */
    }
  }
  void updateNotes(){
    myMidi.updateNotes(notesToTurnOn,notesToTurnOff);
  }
  // A function that prints all the 1's and 0's of a byte, so 8 bits +or- 2
  void printByte(byte val){
      byte i;
      for(byte i=0; i<=7; i++){
        Serial.print(val >> i & 1, BIN); // Magic bit shift, if you care look up the <<, >>, and & operators
      }
      Serial.print("\n"); // Go to the next line, do not collect $200
  }
  void printShifter(){
    for(int i =0; i<numShifterToRead;++i) {
      Serial.print(i);
      Serial.print(": ");
      printByte(now[i]);
    }
    Serial.println();
  }
  private:
  midi myMidi;
    int directReadPins[2]={34,35};
    const int delayTime=400; //microseconds between clock pulses
    const int numShifterToRead=5;
    int shifterPin[5]={ 16,2,26,13,27}; //low notes to high notes
    byte prev2[5]={0,0,0,0,0};
    byte prev[5]={0,0,0,0,0};
    byte now[5]={0,0,0,0,0};
    byte* outputAddr=0;
    byte notesToTurnOn[5]={0,0,0,0,0};
    byte notesToTurnOff[5]={0,0,0,0,0};
    const int shiftPin=18;
    const int clkPin=32;
    const int clkInhPin=19;
};
shifter myShifter(myMidi);
void setup() {
  pinMode(21,OUTPUT);
  digitalWrite(21, LOW);
  Serial.begin(115200);
  
  BLEDevice::init("MIDI ACCORDION");
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
}
void loop() {
  //read shifters
  myShifter.processShift(); // Read the shift register
  myShifter.updateNotes();

  if(printOutput){
    myShifter.printShifter();
  }
}
