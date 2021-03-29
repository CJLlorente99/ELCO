#include "model.h"
#include "allRGB.h"

void printDetail(uint8_t type, int value);

SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
static DFRobotDFPlayerMini myDFPlayer;

static fsm_t* fsm;
volatile int finished;

enum states{
    IDLE,
    ZERO,
    ONE,
    TWO
};

static void
trackFinished_ISR(){
    finished = 1;
}

static int
guardTrackFinished(fsm_t* fsm){
    return finished;
}

static int
always1(fsm_t* fsm){
    return 1;
}

static void
playONE_df(fsm_t* fsm){
    Serial.println("Play ONE df");
    myDFPlayer.play(3);
    finished = 0;
}

static void
playTWO_df(fsm_t* fsm){
    Serial.println("Play TWO df");
    myDFPlayer.play(1);
    finished = 0;
}

static void
playZERO_df(fsm_t* fsm){
    Serial.println("Play ZERO df");
    myDFPlayer.play(2);
    finished = 0;
}

void setup() {
    mySoftwareSerial.begin(9600);
    Serial.begin(115200);

    finished = 0;

    Serial.println();
    Serial.println(F("DFRobot DFPlayer Mini Demo"));
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
    }
    Serial.println(F("DFPlayer Mini online."));

    myDFPlayer.volume(20);  //Set volume value. From 0 to 30
}

void loop() {
    fsm_trans_t tt[] = {
        {IDLE, always1, ZERO, playZERO_df},
        {ZERO, guardTrackFinished, ONE, playONE_df},
        {ONE, guardTrackFinished, TWO, playTWO_df},
        {TWO, guardTrackFinished, ZERO, playZERO_df},
        {-1, NULL, -1, NULL}
    };  
    
    fsm = fsm_new(IDLE, tt, NULL);

    Serial.println("Entering infinite loop");

    unsigned long lastMillis = millis();
    unsigned long actMillis;

    while(1){
        actMillis = millis();

        if(actMillis - lastMillis >= 100){
            lastMillis = millis();
            fsm_fire(fsm);
            
            if (myDFPlayer.available()) {
                printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
            
                if(myDFPlayer.readType() == DFPlayerPlayFinished){
                    trackFinished_ISR();
                    Serial.println("Track Finished");
                }
            }
        }
    }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}