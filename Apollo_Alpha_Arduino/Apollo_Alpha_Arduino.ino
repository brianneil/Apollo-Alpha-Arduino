/*
   Apollo Alpha firmware. Will listen to signals coming via BLE Black Widow shield and play tones
   via the Sparkfun MP3 shield.
*/

//#includes
#include <SPI.h>           // SPI library
#include <SdFat.h>         // SDFat Library
#include <SdFatUtil.h>     // SDFat Util Library
#include <SFEMP3Shield.h>  // Mp3 Shield Library
#include <SoftwareSerial.h> //For running side Serial comms.

//#defines
//Note the highest value that can be sent via the BLE connection is 255
#define LEFT 0
#define RIGHT 1
#define CENTRALSTARTLISTENING 1
#define BOTHEARS 17
#define LEFTEAR 27
#define RIGHTEAR 37
#define VOLUMEDECREMENTAMOUNT 10
#define VOLUMEINCREASEAMOUNT 5
#define TESTVOLUME 75
#define SILENT 255

enum Freqs {    //Make sure this matches up with the dictionary in the central.
  dummy,
  f500HZ,
  f1KHZ,
  f2KHZ
};

enum Ears {
  bothOfThem,
  leftOnly,
  rightOnly
};

enum IncomingMessages {
  startListening,
  sameFreq,
  nextFreq,
  lastFreq,
  sameVol,
  higherVol,
  lowerVol,
  bothEars,
  leftEar,
  rightEar,
  testBeep
};

enum States {
  notListening,
  listeningForFreq,
  listeningForVol,
  listeningForEar,
};

//Instances
Freqs freq = f500HZ;
Ears ear;
IncomingMessages message;
States state = notListening;
SdFat sd; // Create object to handle SD functions
SFEMP3Shield MP3player; // Create Mp3 library object
SoftwareSerial BLE_Shield(4, 5); //Needs to use these pins for TX and RX respectively, these can't be changed.

//Globals
const uint16_t monoMode = 0;  // Mono setting 0 = stereo
int masterVolume = 70;  //This will store the volume, start it off at pretty loud.


void setup() {
  Serial.begin(9600);
  BLE_Shield.begin(9600);
  initSD();
  initMP3Player();
}

void loop() {
  if (BLE_Shield.available()) //Something came in on the BLE comms.
  {
    uint8_t message = BLE_Shield.read(); //Take in the reading from the BLE;
    
    
    switch(state) {
      case notListening:
        if (message == startListening) {
          state = listeningForFreq;
        } else if (message == testBeep) {
          TestBeep();
        }
        break;
        
      case listeningForFreq:  //We should be getting in the frequency
        switch(message) {
          case sameFreq:      //Do nothing. Might be worth putting some bounds checking on these values.
            break;
          case nextFreq:      //Bump up a frequency.
            freq = static_cast<Freqs>(static_cast<int>(freq) + 1);  //In order to do math on the enum you need to cast it to an integer and then cast it back to the enum
            break;
          case lastFreq:      //Go back a frequency.
            freq = static_cast<Freqs>(static_cast<int>(freq) - 1);
            break;
          default:
            Serial.println("Got an out of bounds frequency message.");
            break;
        }
        state = listeningForVol;
        break;

       case listeningForVol:
          switch(message) {
           case sameVol:
              break;
           case higherVol:
              masterVolume = masterVolume - VOLUMEINCREASEAMOUNT;   //Lower numbers are louder, higher numbers are softer
              break;
           case lowerVol:
              masterVolume = masterVolume + VOLUMEDECREMENTAMOUNT;
              break;
           default:
              Serial.println("Got an out of bounds volume message.");
              break;
          }
        state = listeningForEar;
        break;
          
       case listeningForEar:
          switch(message) {
            case bothEars:
              ear = bothOfThem;
              break;
            case leftEar:
              ear = leftOnly;
              break;
            case rightEar:
              ear = rightOnly;
              break;
            default:
              Serial.println("Got an out of bounds ear message.");
          }
        state = notListening;   //Got to the end of the message, go play the tone and send a message to the phone.
        PlayTone();
        SendMessage();
          
    }
  }
}

void PlayTone() {
  union twobyte volume;  //creates a variable that can has a byte for the left and right ear volumes
  if(ear == bothOfThem) {
    volume.byte[LEFT] = masterVolume;
    volume.byte[RIGHT] = masterVolume;
  } else if (ear == leftOnly) {
    volume.byte[LEFT] = masterVolume;
    volume.byte[RIGHT] = SILENT;
  } else if (ear == rightOnly) {
    volume.byte[LEFT] = SILENT;
    volume.byte[RIGHT] = masterVolume;
  }

  MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]);  //Pushes the new volumes onto the player
  
  uint8_t result = MP3player.playTrack(freq);
}

void SendMessage() {
  BLE_Shield.write(CENTRALSTARTLISTENING);   //Tell the phone to start listening. Needs some delays in here to keep the message from being overrun.
  delay(50);
  BLE_Shield.write(freq); //Send frequency
  delay(50);
  BLE_Shield.write(masterVolume);   //Send volume
  delay(50);
  if(ear == bothOfThem) {     //Map the ear enum into some other values so that the messages don't all have the same int values. Makes it easier to debug on the other end.
    BLE_Shield.write(BOTHEARS);
  } else if (ear == leftOnly) {
    BLE_Shield.write(LEFTEAR);
  } else if (ear == rightOnly) {
    BLE_Shield.write(RIGHTEAR);
  }
  
  Serial.print("Just played track ");
  Serial.print(freq);
  Serial.print(" at ");
  Serial.print(masterVolume);
  Serial.print(" volume in ");
  Serial.print(ear);
  Serial.println(" ear(s)");
}


void TestBeep() {
  union twobyte volume;  //creates a variable that can has a byte for the left and right ear volumes
  volume.byte[LEFT] = TESTVOLUME;
  volume.byte[RIGHT] = TESTVOLUME;
  MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]); //Pushes the new volumes onto the player
  freq = f1KHZ;
  uint8_t result = MP3player.playTrack(freq);
  SendMessage();
  Serial.println("I played a test beep");
}

void initMP3Player() {
  uint8_t result = MP3player.begin(); // init the mp3 player shield
  if (result != 0) // check result, see readme for error codes.
  {
    Serial.println(result);
  }
  union twobyte volume;  //creates a variable that can has a byte for the left and right ear volumes
  volume.byte[LEFT] = masterVolume;        //Sets some starting values. 0 is full loudness, 255 is full quiet
  volume.byte[RIGHT] = masterVolume;
  MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]); //Pushes the new volumes onto the player
  MP3player.setMonoMode(monoMode);  //Pushes mono settings
}

void initSD() {   //Code taken from the sparkfun example, not sure how it works.
  //Initialize the SdCard.
  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  // depending upon your SdCard environment, SPI_HAVE_SPEED may work better.
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");
}

