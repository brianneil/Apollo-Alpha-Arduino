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
#define FREQ500HZ 1
#define FREQ1KHZ 2
#define FREQ2KHZ 3

//Instances
SdFat sd; // Create object to handle SD functions
SFEMP3Shield MP3player; // Create Mp3 library object
SoftwareSerial BLE_Shield(4, 5); //Needs to use these pins for TX and RX respectively, these can't be changed.

//Globals
const uint16_t monoMode = 0;  // Mono setting 0 = stereo
union twobyte volume;  //creates a variable that can has a byte for the left and right ear volumes
bool lefty = true;
int trackNumber = 1;
int testTrack = 2;


void setup() {
  Serial.begin(9600);
  BLE_Shield.begin(9600);
  initSD();
  initMP3Player();
}

void loop() {
  if (BLE_Shield.available()) //Something came in on the BLE comms.
  {
    uint8_t readin = BLE_Shield.read();
    Serial.println(readin);
    if (readin == 1) { //Play a simple beep test on both ears. Medium volume.
      volume.byte[LEFT] = 75;
      volume.byte[RIGHT] = 75;

      uint8_t result = MP3player.playTrack(testTrack);
      
      BLE_Shield.write(CENTRALSTARTLISTENING);   //Tell the phone to start listening.
      delay(50);
      BLE_Shield.write(FREQ1KHZ); //Send frequency
      delay(50);
      BLE_Shield.write(75);   //Send volume
      delay(50);
      BLE_Shield.write(BOTHEARS);
      delay(50);
      Serial.println("I played a test tone");
//      if (lefty) {    //This controls what side the sound is played on. Note, that if you blast the sound on one ear, it leaks to the other.
//        volume.byte[LEFT] = 75;
//        volume.byte[RIGHT] = 255;
//        MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]); //Pushes the new volumes onto the player
//        lefty = !lefty;
//        BLE_Shield.write(5);  //Ping the phone so that it knows something played.
//      } else {
//        volume.byte[LEFT] = 255;
//        volume.byte[RIGHT] = 75;
//        MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]); //Pushes the new volumes onto the player
//        lefty = !lefty;
//        BLE_Shield.write(4);  //Ping the phone so that it knows something played.
//      }
//      uint8_t result = MP3player.playTrack(trackNumber);
//      trackNumber++;
//      if (result == 0)  { //No error case
//        Serial.println("I played track 1");
//      }
//      else { // Otherwise there's an error, check the code
//        Serial.println(result);
//      }
    }
  }
}

void initMP3Player() {
  uint8_t result = MP3player.begin(); // init the mp3 player shield
  if (result != 0) // check result, see readme for error codes.
  {
    Serial.println(result);
  }
  volume.byte[LEFT] = 70;        //Sets some starting values. 0 is full loudness, 255 is full quiet
  volume.byte[RIGHT] = 70;
  MP3player.setVolume(volume.byte[LEFT], volume.byte[RIGHT]); //Pushes the new volumes onto the player
  MP3player.setMonoMode(monoMode);  //Pushes mono settings
}

void initSD() {   //Code taken from the sparkfun example, not sure how it works.
  //Initialize the SdCard.
  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  // depending upon your SdCard environment, SPI_HAVE_SPEED may work better.
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");
}
