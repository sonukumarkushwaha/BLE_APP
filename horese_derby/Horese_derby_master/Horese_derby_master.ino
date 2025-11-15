#include "game_comm.h"   // your ESP-NOW communication header
#include "Arduino.h"
#include "PCF8574.h"
#include <Wire.h>

#include <FastLED.h>
char Fd = 0;


#define NUM_LEDS_score 168  // Total number of LEDs for five displays (21 LEDs each)
#define NUM_LEDS_timer 21
CRGB leds_score[NUM_LEDS_score];  // Define LEDs strip for all displays
int timer;
CRGB ledssec[NUM_LEDS_timer];          // Define LEDs strip

#define single_step 2000
#define Buzzer 15
#define Dir 25
#define Step 4
#define start_button 36

PCF8574 pcf8574_Limit1(0x38);
PCF8574 pcf8574_Limit2(0x39);
PCF8574 pcf8574_Stepper(0x3A);

int GameID = 101;
int Maxplayers = 5;
int gametime = 60;
int players = 5;

bool start = false;

byte digits[10][7] =  {{1, 1, 1, 1, 1, 1, 0}, // Digit 0Timers
  {0, 1, 1, 0, 0, 0, 0}, // Digit 1
  {1, 1, 0, 1, 1, 0, 1}, // Digit 2
  {1, 1, 1, 1, 0, 0, 1}, // Digit 3
  {0, 1, 1, 0, 0, 1, 1}, // Digit 4
  {1, 0, 1, 1, 0, 1, 1}, // Digit 5
  {1, 0, 1, 1, 1, 1, 1}, // Digit 6
  {1, 1, 1, 0, 0, 0, 0}, // Digit 7
  {1, 1, 1, 1, 1, 1, 1}, // Digit 8
  {1, 1, 1, 1, 0, 1, 1}  // Digit 9
};  // 2D Array for numbers on 7-segment
const unsigned long gameDuration = 90 * 1000; // Game duration of 2 minutes in milliseconds
unsigned long startTime;
unsigned long lastCountdownTime = 0; // Tracks the last time the countdown was printed
TaskHandle_t Task1Handle = NULL;
int Timers;
int val1;
int val2;
int val3;
int steps = 200;
int limitpin = -1;
int input1 = -1;
int input2 = -1;
int pass1 = 0;
int pass2 = 0;
int pass3 = 0;
int pass4 = 0;
int pass5 = 0;
bool player1 = 1;
bool player2 = 1;
bool player3 = 1;
bool player4 = 1;
bool player5 = 1;
volatile bool datapass = false;
volatile bool button_pressed_limit = false;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;
unsigned long previousMillis5 = 0;

int scores[5];
int ranks[5];
int Score1 = 0, Score2 = 0, Score3 = 0, Score4 = 0, Score5 = 0;
int rank1 = 0, rank2 = 0, rank3 = 0, rank4 = 0, rank5 = 0;
int incoming1 = 0, incoming2 = 0, incoming3 = 0, incoming4 = 0, incoming5 = 0;
int passlimit1 = 0;
int passlimit2 = 0;
int passlimit3 = 0;
int passlimit4 = 0;
int passlimit5 = 0;
bool reached1 = false;
bool reached2 = false;
bool reached3 = false;
bool reached4 = false;
bool reached5 = false;
bool reachedList[5] = { reached1, reached2, reached3, reached4, reached5 };
bool allReached = true;
// Use UART2 for receiving
HardwareSerial MySerial(2);

void Task1(void *pvParameters) {
  (void) pvParameters; // Parameter not used
  //digitalWrite(Dir, HIGH);  // Step pulse ON


  while (true) {
    //Serial.println("inside while loop");
    digitalWrite(Step, HIGH);  // Step pulse ON
    vTaskDelay(pdMS_TO_TICKS(1));            // Step delay
    digitalWrite(Step, LOW);   // Step pulse OFF
    vTaskDelay(pdMS_TO_TICKS(1));                 // Step delay
  }
}

void SerialTask(void *pvParameters) {
  (void) pvParameters;

  String serialdata = "";
  while (true) {
    if (MySerial.available()) {
      serialdata = MySerial.readStringUntil('\n');
      serialdata.trim();

      if (serialdata.length() > 0) {
        Serial.println("Received: " + serialdata);

        // Parse players
        int playersIndex = serialdata.indexOf("\"players\":");
        if (playersIndex != -1) {
          int commaIndex = serialdata.indexOf(",", playersIndex);
          String playersStr = serialdata.substring(playersIndex + 10, commaIndex);
          players = playersStr.toInt();
        }

        // Parse timer
        int timerIndex = serialdata.indexOf("\"timer\":");
        if (timerIndex != -1) {
          int commaIndex = serialdata.indexOf(",", timerIndex);
          String timersStr = serialdata.substring(timerIndex + 8, commaIndex);
          timersStr.trim();
          gametime = timersStr.toInt();
          Serial.print("game time = ");
          Serial.println(gametime);
        }

        // Parse start
        int startIndex = serialdata.indexOf("\"start\":");
        if (startIndex != -1) {
          String startStr = serialdata.substring(startIndex + 8);
          startStr.trim();
          startStr.replace("}", "");
          if (startStr == "true") start = true;
          else start = false;
        }

        Serial.print("Players = ");
        Serial.println(players);
        Serial.print("Start = ");
        Serial.println(start ? "true" : "false");

        // === Execute actions based on received serialdata ===
        if (start == false && players == 100) {
          sendDataToUART(20, Maxplayers, GameID, gametime);
          players = 0;
        }

        if (start == true) {
          Serial.println("started by ble");
          Score1 = 0;
          Score2 = 0;
          Score3 = 0;
          Score4 = 0;
          Score5 = 0;
          for (int i = 1; i <= players; i++) {
            int peerIndex = i - 1;
            sendGameDataToPeer(peerIndex, i, 100, 0, gametime);
            sendDataToUART(i, 100, 0, gametime);
            vTaskDelay(pdMS_TO_TICKS(10));

          }

          for (int j = players + 1; j <= Maxplayers; j++) {
            int peerIndex = j - 1;
            sendGameDataToPeer(peerIndex, j, 300, 0, gametime);
            vTaskDelay(pdMS_TO_TICKS(10));
          }
        }

        if (start == false && players == 0) {
          Serial.println("time saved");
          Serial.println(gametime);
        }

        if (start == false && players >= 1 && players < 9) {
          for (int i = 1; i <= Maxplayers; i++) {
            int peerIndex = i - 1;
            Serial.println("stoped by ble");
            sendGameDataToPeer(peerIndex, i, 200, 0, gametime);
            sendDataToUART(1, 200, scores[i - 1], gametime);
            vTaskDelay(pdMS_TO_TICKS(10));

          }
          vTaskDelay(pdMS_TO_TICKS(5000));

        }
      }
    }

    // Game timer countdown handling
    //    if (start) {
    //      static unsigned long previousMillis = 0;
    //      unsigned long currentMillis = millis();
    //      if (currentMillis - previousMillis >= 1000) {
    //        previousMillis = currentMillis;
    //        gametime--;
    //
    ////        Serial.print("Time left: ");
    ////        Serial.println(gametime);
    //        timer = gametime;
    //        Displaytimerr();
    //        FastLED.show();
    //
    //        //        if (gametime < 1) {
    //        //
    //        //         // sendDataToUART(1, 200, Score1, gametime);
    //        //          vTaskDelay(pdMS_TO_TICKS(5000));
    //        //          start = false;
    //        //        }
    //      }
    //
    //      Display1stScore();
    //      Display2ndScore();
    //      Display3rdScore();
    //      Display4thScore();
    //      Display5thScore();
    //      FastLED.show();
    //    }

    vTaskDelay(pdMS_TO_TICKS(20)); // yield CPU, no blocking
  }
}

// Handle incoming game messages
void onGameMessage(struct_message data) {

  if (data.gameStatus == 100) {
    if (data.id == 1)incoming1 = data.score;
//    Serial.print("incoming1 = ");
//    Serial.println(incoming1);
    if (data.id == 2)incoming2 = data.score;
//    Serial.print("incoming2 = ");
//    Serial.println(incoming2);
    if (data.id == 3)incoming3 = data.score;
//    Serial.print("incoming3 = ");
//    Serial.println(incoming3);
    if (data.id == 4)incoming4 = data.score;
//    Serial.print("incoming4 = ");
//    Serial.println(incoming4);
    if (data.id == 5)incoming5 = data.score;
//    Serial.print("incoming5 = ");
//    Serial.println(incoming5);
    data.gameStatus = 0;
    datapass = true;
  }
}


void setup() {
  Serial.begin(115200);
  delay(50);
  Wire.begin();
  setupGameComm();
  pinMode(Dir, OUTPUT);
  pinMode(Step, OUTPUT);
  pinMode(start_button, INPUT_PULLUP);
  FastLED.addLeds<WS2812B, 14, GRB>(leds_score, NUM_LEDS_score);
  FastLED.addLeds<WS2812B, 26, GRB>(ledssec, NUM_LEDS_timer);
  FastLED.setBrightness(255);
  MySerial.begin(9600, SERIAL_8N1, 16, 17);  // Baud, config, RX, TX
  Serial.println("UART Receiver ready on pins 16 (RX) and 17 (TX)");
  pinMode(Buzzer, OUTPUT);

  digitalWrite(Buzzer, LOW);

  pcf8574_Stepper.begin();
  pcf8574_Limit1.begin();
  pcf8574_Limit2.begin();

  // Set all pins to INPUT
  for (int i = 0; i < 8; i++) {
    pcf8574_Limit1.pinMode(i, INPUT);
    pcf8574_Limit2.pinMode(i, INPUT);
  }
  for (int i = 0; i < 8; i++) {
    pcf8574_Stepper.pinMode(i, OUTPUT);
  }


  // Create Task 1: Blink LED
  xTaskCreate(
    Task1,             // Function that implements the task
    "Blink Task",       // Task name (for debugging purposes)
    2048,               // Stack size (in words, not bytes)
    NULL,               // Parameter passed to the task
    1,                  // Priority (1 is low, higher number is higher priority)
    &Task1Handle        // Task handle (can be used to manipulate the task later)
  );
  xTaskCreate(
    SerialTask,
    "SerialTask",
    4096,   // Stack size for string handling
    NULL,
    2,      // Slightly higher priority than Task1
    NULL
  );

  for (int i = 0; i < 8; i++) {
    Serial.println(" ALL PIN HIGH ");
    pcf8574_Stepper.digitalWrite(i, LOW);
  }
  delay(1000);

  startup();
}

void sendDataToUART(int id, int status, int score, int time) {
  String data = "{";
  data += "\"id\":" + String(id) + ",";
  data += "\"status\":\"" + String(status) + "\",";
  data += "\"score\":" + String(score) + ",";
  data += "\"time\":" + String(time);
  data += "}";

  MySerial.println(data);                   // Send via UART
  Serial.println("Sent over UART: " + data); // Debug print
}

void startup() {
  Score1 = 0;
  Score2 = 0;
  Score3 = 0;
  Score4 = 0;
  Score5 = 0;
  passlimit1 = 0;
  passlimit2 = 0;
  passlimit3 = 0;
  passlimit4 = 0;
  passlimit5 = 0;

  for (int i = 0; i < 8; i++) {
    pcf8574_Stepper.digitalWrite(i, HIGH);  // start the stepper
  }

  while (1) {
    if (passlimit1 == 1 && passlimit2 == 1 && passlimit3 == 1 && passlimit4 == 1 && passlimit5 == 1) {
      break;
    }
    else {
      Serial.println("returnig to original position ");
      digitalWrite(Dir, HIGH);               // Step reverse coming to start point
      startpoint();
    }
  }

  Serial.println("while loop break");
  start = false;

  delay(2000);

  while (true) {
    if (digitalRead(start_button) == 0) {
      if (passlimit1 == 1 && passlimit2 == 1 && passlimit3 == 1 && passlimit4 == 1 && passlimit5 == 1) {
        digitalWrite(Dir, LOW);  // Step forward toward gameplay
        passlimit1 = 0;
        passlimit2 = 0;
        passlimit3 = 0;
        passlimit4 = 0;
        passlimit5 = 0;
        digitalWrite(Buzzer, HIGH);
        Serial.print("players = ");
        Serial.println(players);
        for (int i = 1; i <= players; i++) {
          int peerIndex = i - 1;  // because array starts from 0
          Serial.println("started by button");
          sendGameDataToPeer(peerIndex, i, 100, 0, gametime);
          sendDataToUART(i, 100, 0, gametime);
          delay(10);
        }
        for (int i = 0; i < Maxplayers; i++) {
          reachedList[i] = 0;
        }
        reached1 = false;
        reached2 = false;
        reached3 = false;
        reached4 = false;
        reached5 = false;
        delay(900);
        digitalWrite(Buzzer, LOW);
        break;
      }

    }
    else if (start == true) {
      digitalWrite(Dir, LOW);  // Step forward toward gameplay
      digitalWrite(Buzzer, HIGH);
      delay(1000);
      digitalWrite(Buzzer, LOW);
      break;
    }

  }
  startTime = millis();  // Record game start time
  for (int i = 0; i < Maxplayers; i++) {
    reachedList[i] = 0;
  }
  reached1 = false;
  reached2 = false;
  reached3 = false;
  reached4 = false;
  reached5 = false;
  Serial.println("game started ");
}

void startpoint() {
  for (int i = 0; i < 8; i++) {
    // Use forceReadNow to read the status
    uint8_t limitStatus = pcf8574_Limit1.digitalRead(i, true);  // Force read now

    if (limitStatus == 0) {                                    // Limit pressed
      //      Serial.print("Limit pressed on pin: ");
      //      Serial.println(i);
      limitpin = i;
      button_pressed_limit = true;
    }
    if (button_pressed_limit == true) {
      if (limitpin == 0) {
        Serial.println("stepper 1 startpoint ");

        pcf8574_Stepper.digitalWrite(0, LOW);
        limitpin = -1;
        button_pressed_limit = false;
        passlimit1 = 1;
      }
      if (limitpin == 1) {
        Serial.println("stepper 2 startpoint ");
        pcf8574_Stepper.digitalWrite(1, LOW);
        limitpin = -1;
        button_pressed_limit = false;
        passlimit2 = 1;

      }
      if (limitpin == 2) {
        Serial.println("stepper 3 startpoint ");
        pcf8574_Stepper.digitalWrite(2, LOW);
        limitpin = -1;
        button_pressed_limit = false;
        passlimit3 = 1;

      }
      if (limitpin == 3) {
        Serial.println("stepper 4 startpoint ");
        pcf8574_Stepper.digitalWrite(3, LOW);
        limitpin = -1;
        button_pressed_limit = false;
        passlimit4 = 1;

      }
      if (limitpin == 4) {
        Serial.println("stepper 5 startpoint ");
        pcf8574_Stepper.digitalWrite(4, LOW);
        limitpin = -1;
        button_pressed_limit = false;
        passlimit5 = 1;

      }
    }
  }
}

void loop() {


  for (int i = 0; i < 8; i++) {
    // Use forceReadNow to read the status
    uint8_t limitStatus2 = pcf8574_Limit2.digitalRead(i, true);  // Force read now

    if (limitStatus2 == 0) {  // Limit pressed
      //             Serial.print("Limit pressed on pin: ");
      //            Serial.println(i);
      if (i == 0 && reached1 == false) {
        Serial.println("player1 reached ");
        sendGameDataToPeer(0, 1, 200, 0, gametime);
        sendDataToUART(1, 400, Score3, 5);
        pcf8574_Stepper.digitalWrite(0, LOW);
        button_pressed_limit = false;
        reached1 = true;
        reachedList[0] = true;
        limitpin = -1;
      }
      if (i == 1 && reached2 == false) {
        Serial.println("player2 reached ");
        sendGameDataToPeer(1, 2, 200, 0, gametime);
        sendDataToUART(2, 400, Score3, 5);
        pcf8574_Stepper.digitalWrite(1, LOW);
        button_pressed_limit = false;
        reached2 = true;
        reachedList[1] = true;
        limitpin = -1;
      }
      if (i == 2 && reached3 == false) {
        Serial.println("player3 reached ");
        sendGameDataToPeer(2, 3, 200, 0, gametime);
        sendDataToUART(3, 400, Score3, 5);
        pcf8574_Stepper.digitalWrite(2, LOW);
        button_pressed_limit = false;
        reached3 = true;
        reachedList[2] = true;
        limitpin = -1;
      }
      if (i == 3 && reached4 == false ) {
        sendGameDataToPeer(3, 4, 200, 0, gametime);
        sendDataToUART(4, 400, Score3, 5);
        Serial.println("player4 reached ");
        pcf8574_Stepper.digitalWrite(3, LOW);
        button_pressed_limit = false;
        reachedList[3] = true;
        reached4 = true;
        limitpin = -1;
      }
      if (i == 4 && reached5 == false ) {
        Serial.println("player5 reached ");
        sendGameDataToPeer(4, 5, 200, 0, gametime);
        sendDataToUART(5, 400, Score3, 5);
        pcf8574_Stepper.digitalWrite(4, LOW);
        button_pressed_limit = false;
        reached5 = true;
        reachedList[4] = true;
        limitpin = -1;
      }
    }
  }


  allReached = true;

  for (int i = 0; i < players; i++) {
    if (!reachedList[i]) {

      allReached = false;
      break;
    }
  }
  //    Serial.print("reached1 array = ");
  //    Serial.print(reachedList[0]);
  //    Serial.print("reached1 default = ");
  //    Serial.println(reached1);
  //      Serial.print("reached2 array = ");
  //    Serial.print(reachedList[1]);
  //    Serial.print("reached2 default = ");
  //    Serial.println(reached2);

  if (allReached) {
    Serial.println("Stepper end point*** Game over ");
    for (int i = 0; i < 5; i++) {
      pcf8574_Stepper.digitalWrite(i, LOW);
    }
    digitalWrite(Buzzer, HIGH);
    for (int i = 1; i <= players; i++) {
      int peerIndex = i - 1;  // because array starts from 0
      Serial.println("stoped by gameover");
      sendGameDataToPeer(peerIndex, i, 200, 0, gametime);
      sendDataToUART(i, 200, scores[i - 1], gametime);
      delay(10);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    digitalWrite(Buzzer, LOW);
    vTaskDelay(pdMS_TO_TICKS(8000));
    digitalWrite(Dir, HIGH);  // coming to startpoint
    limitpin = -1;
    button_pressed_limit = false;
    startup();
    start = false;

  }


  vTaskDelay(70 / portTICK_PERIOD_MS);

  Serial.print("datatpass = ");
  Serial.println(datapass);
  if (datapass == true) {
    //motor starts
    if (incoming1 > pass1 && reached1 == false ) {
      Score1 = Score1 + incoming1;
      sendDataToUART(1, 100, Score1, 5);
      pass1 = incoming1;
      incoming1 = 0;
      Serial.println("Stepper one start ");
      pcf8574_Stepper.digitalWrite(0, HIGH);
    }
    if (incoming2 > pass2  && reached2 == false  ) {
      Score2 = Score2 + incoming2;
      sendDataToUART(2, 100, Score2, 5);
      pass2 = incoming2;
      incoming2 = 0;
      Serial.println("Stepper two start ");
      pcf8574_Stepper.digitalWrite(1, HIGH);
    }
    if (incoming3 > pass3  && reached3 == false  ) {
      Score3 = Score3 + incoming3;
      sendDataToUART(3, 100, Score3, 5);
      pass3 = incoming3;
      incoming3 = 0;
      Serial.println("Stepper three start ");
      pcf8574_Stepper.digitalWrite(2, HIGH);
    }
    if (incoming4 > pass4  && reached4 == false  ) {
      Score4 = Score4 + incoming4;
      sendDataToUART(4, 100, Score4, 5);
      pass4 = incoming4;
      incoming4 = 0;
      Serial.println("Stepper four start ");
      pcf8574_Stepper.digitalWrite(3, HIGH);
    }
    if (incoming5 > pass5  && reached5 == false ) {

      Score5 = Score5 + incoming5;
      sendDataToUART(5, 100, Score5, 5);
      pass5 = incoming5;
      incoming5 = 0;
      Serial.println("Stepper five start ");
      pcf8574_Stepper.digitalWrite(4, HIGH);
    }
    datapass = false;
  }



  if (pass1 >= 1) {
    Serial.print("pass1= ");
    Serial.println(pass1);
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis1 >= pass1 * single_step) {
      previousMillis1 = currentMillis;
      pass1 = 0;
      sendDataToUART(1, 100, Score1, 5);
      Serial.println("stepper1 stops ");
      pcf8574_Stepper.digitalWrite(0, LOW);

    }
  }
  if (pass2 >= 1) {
    //motor stops
    unsigned long currentMillis2 = millis();
    if (currentMillis2 - previousMillis2 >= pass2 * single_step) {
      previousMillis2 = currentMillis2;
      pass2 = 0;
      sendDataToUART(2, 100, Score2, 5);
      Serial.println("stepper2 stops ");
      pcf8574_Stepper.digitalWrite(1, LOW);

    }
  }
  if (pass3 >= 1) {

    unsigned long currentMillis3 = millis();
    if (currentMillis3 - previousMillis3 >= pass3 * single_step) {
      previousMillis3 = currentMillis3;
      sendDataToUART(3, 100, Score3, 5);
      pass3 = 0;
      Serial.println("stepper3 stops ");
      pcf8574_Stepper.digitalWrite(2, LOW);

    }
  }

  if (pass4 >= 1) {

    unsigned long currentMillis4 = millis();
    if (currentMillis4 - previousMillis4 >= pass4 * single_step) {
      previousMillis4 = currentMillis4;
      sendDataToUART(4, 100, Score4, 5);
      pass4 = 0;
      Serial.println("stepper4 stops ");
      pcf8574_Stepper.digitalWrite(3, LOW);

    }
  }
  if (pass5 >= 1) {

    unsigned long currentMillis5 = millis();
    if (currentMillis5 - previousMillis5 >= pass5 * single_step) {
      previousMillis5 = currentMillis5;
      pass5 = 0;
      sendDataToUART(5, 100, Score5, 5);
      Serial.println("stepper5 stops ");
      pcf8574_Stepper.digitalWrite(4, LOW);

    }
  }
  //********************************************rankupdate*************************
  scores[0] = Score1;
  scores[1] = Score2;
  scores[2] = Score3;
  scores[3] = Score4;
  scores[4] = Score5;

  for (int i = 0; i < 5; i++) {
    int rank = 1;
    for (int j = 0; j < 5; j++) {
      if (scores[j] > scores[i]) {
        rank++;
      }
    }
    ranks[i] = rank;
  }

  //  // Print scores and ranks
  //  for (int i = 0; i < 5; i++) {
  //    Serial.print("Player ");
  //    Serial.print(i + 1);
  //    Serial.print(" Score: ");
  //    Serial.print(scores[i]);
  //    Serial.print("  Rank: ");
  //    Serial.println(ranks[i]);
  //
  //
  //  }
  //********************************************************************************
}



void Display1stScore() {
  for (int i = 0; i < 21; i++) leds_score[i] = CRGB::Black;

  int cursor;
  int Fd = (rank1 < 10) ? 1 : (rank1 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 14;
      digit = rank1 % 10;
    }
    else if (i == 2) {
      cursor = 7;
      digit = (rank1 / 10 % 10);
    }
    else {
      cursor = 0;
      digit = (rank1 / 100 % 10);
    }

    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Fuchsia : CRGB::Black;
      cursor++;
    }
  }
}

// Display 2: LEDs 21-41
void Display2ndScore() {
  for (int i = 21; i <= 41; i++) leds_score[i] = CRGB::Black;

  int cursor;
  int Fd = (rank2 < 10) ? 1 : (rank2 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 35;
      digit = rank2 % 10;
    }
    else if (i == 2) {
      cursor = 28;
      digit = (rank2 / 10 % 10);
    }
    else {
      cursor = 21;
      digit = (rank2 / 100 % 10);
    }

    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Red : CRGB::Black;
      cursor++;
    }
  }
}

// Display 3: LEDs 42-62
void Display3rdScore() {
  for (int i = 42; i <= 62; i++) leds_score[i] = CRGB::Black;

  int cursor;
  int Fd = (rank3 < 10) ? 1 : (rank3 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 56;
      digit = rank3 % 10;
    }
    else if (i == 2) {
      cursor = 49;
      digit = (rank3 / 10 % 10);
    }
    else {
      cursor = 42;
      digit = (rank3 / 100 % 10);
    }

    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Blue : CRGB::Black;
      cursor++;
    }
  }
}

// Display 4: LEDs 63-83
void Display4thScore() {
  for (int i = 63; i <= 83; i++) leds_score[i] = CRGB::Black;

  int cursor;
  int Fd = (rank4 < 10) ? 1 : (rank4 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 77;
      digit = rank4 % 10;
    }
    else if (i == 2) {
      cursor = 70;
      digit = (rank4 / 10 % 10);
    }
    else {
      cursor = 63;
      digit = (rank4 / 100 % 10);
    }

    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Green : CRGB::Black;
      cursor++;
    }
  }
}

// Display 5: LEDs 84-104
void Display5thScore() {
  for (int i = 84; i <= 104; i++) leds_score[i] = CRGB::Black;

  int cursor;
  int Fd = (rank5 < 10) ? 1 : (rank5 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 98;
      digit = rank5 % 10;
    }
    else if (i == 2) {
      cursor = 91;
      digit = (rank5 / 10 % 10);
    }
    else {
      cursor = 84;
      digit = (rank5 / 100 % 10);
    }

    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Yellow : CRGB::Black;
      cursor++;
    }
  }
}

void Displaytimerr() {
  fill_solid(ledssec, NUM_LEDS_timer, CRGB::Black);

  int digitsToDisplay[3] = { (timer / 100) % 10, (timer / 10) % 10, timer % 10 };
  int startPositions[3] = { 0, 7, 14 };
  CRGB colors[3] = { CRGB::Green, CRGB::Fuchsia, CRGB::Red };

  int numDigits = (timer < 10) ? 1 : (timer < 100 ? 2 : 3);

  for (int i = 0; i < numDigits; i++) {
    int cursor = startPositions[3 - numDigits + i];
    int digit = digitsToDisplay[3 - numDigits + i];

    for (int k = 0; k < 7; k++) {
      ledssec[cursor++] = digits[digit][k] ? colors[3 - numDigits + i] : CRGB::Black;
    }
  }
}
