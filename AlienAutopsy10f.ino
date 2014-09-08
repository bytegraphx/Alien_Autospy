#include <SD.h>
#include <SPI.h>
#include <Mp3Player.h>
#include <Adafruit_NeoPixel.h>

static Mp3Player mp3p;

#define COUNTOF(array) sizeof(array)/(sizeof(array[0]))
#define PICKATRACK(array) pickATrack(array, COUNTOF(array))
#define pinArm A0
#define pinLeg A1
#define pinHeart A2
#define pinBone A3
#define NeoPixel 5
#define N_LEDS 24
const int RELAY1 =  10;      // the number of the Relay pin

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, NeoPixel, NEO_GRB + NEO_KHZ800);

static char* armMp3Names[]={
  "1.mp3"};

static char* legMp3Names[]={
  "2.mp3"};

static char* heartMp3Names[]={
  "3.mp3"};

static char* boneMp3Names[]={
  "4.mp3"};

// for the idle state
static char* idleMp3Names[]={
  "5.mp3"};

const char* pickATrack(char* names[], int cname){
  const int isz = random(0, cname);
  const char *const sz= names[isz];
  return sz;
}

void RelayOn(){
  // it does no harm to call RelayOn over and over.
  // set the relay control pin to 0
  digitalWrite(RELAY1, LOW);


}
void RelayOff(){
  // it does no harm to call RelayOff over and over.
  // set the relay control pin to 1
  digitalWrite(RELAY1, HIGH);

}

// Constants for ititialization
static const unsigned char vol=0;      // 0 is loudest, 128 is quietest;

// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  mp3p.bInit();
  mp3p.SetVolume(vol,vol); // stereo left and right
  Serial.begin(115200);
  Serial.print("Volume is ");  
  Serial.println(vol);
  
  // it turns out we did not need external resistors and we could have used 
  // pinmode(pinArm, INPUT_PULLUP); which would enable an internal resistor!
  pinMode(pinArm, INPUT_PULLUP);
  pinMode(pinLeg, INPUT_PULLUP);
  pinMode(pinHeart, INPUT_PULLUP);
  pinMode(pinBone, INPUT_PULLUP);
  Serial.println("body pins set to input");
  pinMode(RELAY1, OUTPUT); 
  //theaterChase(strip.Color(127, 127, 127), 20); // White
  //theaterChase(strip.Color(127,   0,   0), 20); // Red
  //theaterChase(strip.Color(  0,   0, 127), 20); // Blue
}
// end of setup()
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////


// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// costants for managing the loop
static const long millisChasePeriod = 2000; // a twp seconds
static const long millisRunRelayDuration = 100; // tenth of a second
static const long millisInhibitRelayDuration = 0; // anything other then 0 makes motor sputter. 
static const long lPlayOnceEvery = 20000; // bigger means less background groaning

#define COLOR_ARM strip.Color( 0, 0, 125)
#define COLOR_LEG strip.Color( 0, 0, 125)
#define COLOR_HEART strip.Color( 0, 0, 255) //blue???
#define COLOR_BONE strip.Color(0, 0, 125)
#define COLOR_IDLE strip.Color(0, 255, 0) // should be Purple
#define COLOR_OFF_IDLE strip.Color(0,0,0) // yellow?
#define COLOR_OFF_CONTACT strip.Color( 0,0,255) // BLUE????
#define LIGHTS_IN_CHASE 3 /* any number 1 or bigger */
enum ePlay {
  kArm,
  kLeg,
  kHeart,
  kBone,
  kIdle,
  kNotPlaying,
};
// end of costants for managing the loop
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////


// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// variables for keeping track of the loop's history
int iChasePhase = 0;
long millisNextChaseChange = 0;
long millisStopRelayTime = 0;
long millisInhibitBuzzerUntilTime = 0;
ePlay playPrevious = kIdle;
ePlay playingNow = kIdle;
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////



// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// the loop itself
void loop(){
  const long millisTimeNow = millis();
  //Serial.print(millisTimeNow);
  //Serial.print("\t");


  ePlay playNext;

  //
  // here we'll look at the pins and decide if one was triggered.
  // we'll set playNext to match the triggered pin,
  // or leave it as kIdle if none is triggered
  //
  if(!digitalRead(pinArm)){
    playNext=kArm;
    Serial.print("arm contacft\t");
  } 
  else if(!digitalRead(pinLeg)){
    Serial.print("leg contact\t");
    playNext=kLeg;
  }
  else
    if(!digitalRead(pinHeart)){
      Serial.print("heart contact\t");
      playNext=kHeart;
    } 
    else
      if(!digitalRead(pinBone)){
        Serial.print("bone contact\t");
        playNext=kBone;
      } 
      else {
        Serial.print("no contacts\t");
        playNext = kIdle;
      }

  // /////////////////////////////////////////////////
  // chase lights
  int colorOnNow = COLOR_IDLE;
  int colorOffNow = COLOR_OFF_IDLE;
  switch(playingNow){
  case kArm: 
    colorOnNow = COLOR_ARM;
    colorOffNow = COLOR_OFF_CONTACT;
    break;
  case kLeg: 
    colorOnNow = COLOR_LEG;
    colorOffNow = COLOR_OFF_CONTACT;
    break;
  case kBone: 
    colorOnNow = COLOR_BONE;
    colorOffNow = COLOR_OFF_CONTACT;
    break;
  case kHeart: 
    colorOnNow = COLOR_HEART;
    colorOffNow = COLOR_OFF_CONTACT;
    break;
  }
  if(millisNextChaseChange <= millisTimeNow){
    // it's time to move on the chase.
    // first calculate the time for the next change:
    millisNextChaseChange = millisTimeNow + millisChasePeriod;
    stripSetPattern(LIGHTS_IN_CHASE, iChasePhase, colorOffNow, colorOnNow);
    // the % means "remainder after dividing by".  9%10 is 9 but 11%10 is one.
    iChasePhase = (iChasePhase+1)% LIGHTS_IN_CHASE;
    Serial.print(iChasePhase);
    Serial.print("\t");
  }
  else{
    // do nothing, wait until next change-time
  }
  // end chase lights
  // /////////////////////////////////////////////////

  // /////////////////////////////////////////////////
  // buzzer
  if(playNext != kIdle){
    if(millisInhibitBuzzerUntilTime <= millisTimeNow){
      Serial.print("RELAY ON\t");
      RelayOn();
      millisStopRelayTime = millisTimeNow + millisRunRelayDuration;
      millisInhibitBuzzerUntilTime = millisTimeNow + millisInhibitRelayDuration;
    }
    else{
      Serial.print("RELAY OFF -- buzzer inhibit\t");
      RelayOff();
    }
  }
  else{
    if(millisStopRelayTime <= millisTimeNow){
      Serial.print("RELAY OFF\t");
      RelayOff();
    } 
    else {
      Serial.print("leaving relay on\t");
    }
  }
  // end buzzer
  // /////////////////////////////////////////////////

  boolean bPlaying = mp3p.bCurrentlyPlaying();
  if(!bPlaying) playingNow = kNotPlaying;

  boolean bInterruptTrackForBuzzer = (playingNow != playNext) && (playNext != kIdle) && (playPrevious == kIdle);
  Serial.print("bInterruptTrackForBuzzer=");
  Serial.print(bInterruptTrackForBuzzer);
  Serial.print("\t");

  if(!bPlaying || bInterruptTrackForBuzzer){
    Serial.print("changing track\t");
    // we are firing off a new play "foul".
    // that plays a sound and runs the relay for a while.

    const char* trackName = NULL;
    if(kArm==playNext) trackName=PICKATRACK(armMp3Names);
    else if(kLeg==playNext) trackName=PICKATRACK(legMp3Names);
    else if(kHeart==playNext) trackName=PICKATRACK(heartMp3Names);
    else if(kBone==playNext) trackName=PICKATRACK(boneMp3Names);
    else if(0 == random(0,lPlayOnceEvery)) trackName=PICKATRACK(idleMp3Names);
    //trackName="forwards.mp3";
    Serial.print("trackName=");
    Serial.print(NULL == trackName ? "Null" : trackName);
    Serial.print("\t");
    playingNow = playNext;
    if(NULL!=trackName){
      mp3p.startPlaying(trackName);
    }
  }
  else{
    Serial.print("same track\t");
  }
  mp3p.keepPlaying();
  playPrevious = playNext;
  Serial.println("");
}
// end of loop()
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////
// /////////////////////////////////////////////////


//Theatre-style crawling lights.

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10000; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

void stripSetPattern(int pixelsBeforeRepeat, int pixelStartThisTime, int colorOff, int colorOn){
  for (int i=0; i < strip.numPixels(); i++) {
    const boolean bOn = pixelStartThisTime == (i % pixelsBeforeRepeat);
    const int color = bOn ? colorOn : colorOff;
    //Serial.print(color);
    strip.setPixelColor(i, color);    //turn every third pixel on
  }
  strip.show();
  //Serial.println();
}








