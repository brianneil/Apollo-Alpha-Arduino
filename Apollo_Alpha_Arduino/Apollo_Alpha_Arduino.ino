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
#define STARTLISTENING 1
#define DIDPLAYTONE 1
#define SOMETHINGWRONG 0
#define TESTVOLUME 75
#define STARTINGFREQ f250HZ
#define SILENT 255

enum Freqs {    //Make sure this matches up with the dictionary in the central.
  dummy,
  f250HZ,
  f500HZ,
  f1KHZ,
  f2KHZ,
  f4KHZ,
  f8KHZ
};

enum Ears {
  rightOnly,
  leftOnly,
  bothOfThem
};

enum States {
  notListening,
  listeningForFreq,
  listeningForVol,
  listeningForEar,
};

//Instances
Freqs freq;
Ears ear;
States state = notListening;
SdFat sd; // Create object to handle SD functions
SFEMP3Shield MP3player; // Create Mp3 library object
SoftwareSerial BLE_Shield(4, 5); //Needs to use these pins for TX and RX respectively, these can't be changed.

//Globals
const uint16_t monoMode = 0;  // Mono setting 0 = stereo
int masterVolume;  //This will store the volume, start it off at pretty loud.


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


    switch (state) {
      case notListening:
        if (message == STARTLISTENING) {
          state = listeningForFreq;
        }
        break;

      case listeningForFreq:  //We should be getting in the frequency
        //Cast the int into a hashvalue for freq and grab the correct frequency
        freq = static_cast<Freqs>(message);   //To do, add some bounds checking so we don't crash.
        state = listeningForVol;
        break;

      case listeningForVol:
        masterVolume = message;
        state = listeningForEar;
        break;

      case listeningForEar:
        ear = static_cast<Ears>(message); //To do: Add bounds checking so we don't crash.
        state = notListening;   //Got to the end of the message, go play the tone and send a message to the phone.
        PlayTone();
        SendMessagePlayedTone();

    }
  }
}

void PlayTone() {
  union twobyte volume;  //creates a variable that can has a byte for the left and right ear volumes
  if (ear == bothOfThem) {
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

void SendMessagePlayedTone() {
  BLE_Shield.write(DIDPLAYTONE);   //Tell the central we played a tone. Note for the future, to send a bunch of messages in a row, add a 50 ms delay between.

  Serial.print("Just played track ");
  Serial.print(freq);
  Serial.print(" at ");
  Serial.print(masterVolume);
  Serial.print(" volume in ");
  Serial.print(ear);
  Serial.println(" ear(s)");
}

void initMP3Player() {
  uint8_t result = MP3player.begin(); // init the mp3 player shield
  if (result != 0) // check result, see readme for error codes.
  {
    Serial.println(result); //To do: handle errors here in some way?
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

