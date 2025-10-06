#include "game_comm.h"
#include "Arduino.h"
#include <FastLED.h>
#include <Wire.h>
#include <PCF8574.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
SoftwareSerial mySerial2(32, 33); // RX, TX for second DFPlayer Mini
DFRobotDFPlayerMini myDFPlayer2;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ------------------------ CONSTANTS ------------------------
#define start_button 36
#define buzzer 26
#define NUM_LEDS1 27
#define NUM_LEDS2 27
#define NUM_LEDS3 27
//#define device 2
uint8_t device = 1;
uint8_t adultMode = 1;
// Define pins for DFPlayer

#define I2C_SDA 21
#define I2C_SCL 22

// ------------------------ GLOBALS ------------------------
uint8_t timer = 60;
unsigned long previousMillis = 0;
uint8_t flag = 0;
unsigned long vallue = 0;
unsigned long currentTime = 0;
int randomno = -1;
int previousno = -1;
bool gameRunning = false;

int score1 = 0, score3 = 0;
int relayTimer = 0;
int pass = 1, pass2 = 1;
int in1 = 1;
int in = 1; // Used in gamelogic()
int speed = 1500;
volatile bool stopTimers = false;
//
int Gstatus = 0;
int score2 = 0;

TaskHandle_t timersTaskHandle = NULL;

// External hardware and LED arrays
PCF8574 pcf8574_OUT1(0x3A);
PCF8574 pcf8574_OUT2(0x3C);
PCF8574 pcf8574_IN1(0x38);
PCF8574 pcf8574_IN2(0x39);

CRGB leds1ST[NUM_LEDS1];
CRGB leds2nd[NUM_LEDS2];
CRGB leds3rd[NUM_LEDS3];




String QR_data = "default_qr";  // replace as needed

const uint8_t digits[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1} // 9
};

// ------------------------ FUNCTION DECLARATIONS ------------------------
void startup();
void gamelogic();
void Display1stScore();
void Display2ndScore();
void Display3rdScore();
void timers();
int readswitch(int switchin, int chip);
void randomgenrate(int a, int b);
void game_over();



void setup() {
  Serial.begin(115200);
  setupGameComm();
  // Create a FreeRTOS task on core 0 to run the timers() function
  xTaskCreatePinnedToCore(
    timersTask,     // Function to implement the task
    "Timers Task",  // Name of the task
    2048,           // Stack size in words
    NULL,           // Task input parameter
    1,              // Priority of the task (1 = low)
    &timersTaskHandle,
    0               // Core where the task should run (0 or 1)
  );
  //stopTimers = true;
  vTaskSuspend(timersTaskHandle);
  mySerial2.begin(9600);
  myDFPlayer2.begin(mySerial2);
  Serial.println("Second DFPlayer Mini online.");
  myDFPlayer2.volume(35); // Set initial volume (0-30)

  pinMode(start_button, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  FastLED.addLeds<WS2812, 25, GRB>(leds1ST, NUM_LEDS1);
  FastLED.addLeds<WS2812, 4, GRB>(leds2nd, NUM_LEDS2);
  FastLED.addLeds<WS2812, 14, GRB>(leds3rd, NUM_LEDS3);
  FastLED.clear();
  FastLED.show();


  pcf8574_OUT1.begin();
  pcf8574_OUT2.begin();
  pcf8574_IN1.begin();
  pcf8574_IN2.begin();

  for (int i = 0; i < 8; i++) {
    pcf8574_IN1.pinMode(i, INPUT);
    pcf8574_IN2.pinMode(i, INPUT);
  }
  for (int i = 0; i < 8; i++) {
    pcf8574_OUT1.pinMode(i, OUTPUT);
    pcf8574_OUT2.pinMode(i, OUTPUT);
  }
  for (int i = 0; i < 8; i++) {
    pcf8574_OUT1.digitalWrite(i, HIGH);
    pcf8574_OUT2.digitalWrite(i, HIGH);
  }

  startup();
}

void startup() {

  Serial.println("startup....");
  Gstatus = 0;
  score1 = 0;
  gameRunning = false;
  myDFPlayer2.playMp3Folder(1);
  delay(1000);
  myDFPlayer2.advertise(4); // Play the advertisement track 4 on second player
  delay(2000);

  while (digitalRead(start_button) == HIGH) {

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
      flag = !flag;
      previousMillis = currentMillis;
      for (int i = 0; i < 8; i++) {
        pcf8574_OUT1.digitalWrite(i, flag);
        if (i == 7) {
          pcf8574_OUT2.digitalWrite(6, flag);
        }
        else {
          pcf8574_OUT2.digitalWrite(i, flag);
        }

      }
    }

    if (Gstatus == 100) {
      Gstatus = 0;
      Serial.print("timer=");
      Serial.println(timer);
      Serial.println("started by espnow");
      break;
    }
    if (Gstatus == 300) {
      Gstatus = 0;
      Serial.print("timer=");
      Serial.println(timer);

      for (int i = 0; i < 8; i++) {
        pcf8574_OUT1.digitalWrite(i, HIGH);
        pcf8574_OUT2.digitalWrite(i, HIGH);
      }
      Serial.println("on hold triggered");
      onhold();
      return;

    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  if (device == 1 && digitalRead(start_button) == LOW) {
    Serial.println("started by button");
    timer = 60;
    sendGameData(2, 100, 0, 60);
  }
  if (device == 2 && digitalRead(start_button) == LOW) {
    Serial.println("started by button");
    timer = 60;
    sendGameData(1, 100, 0, 60);
  }
  for (int i = 0; i < 8; i++) {
    pcf8574_OUT1.digitalWrite(i, HIGH);
    pcf8574_OUT2.digitalWrite(i, HIGH);
  }
  digitalWrite(buzzer, HIGH);
  pcf8574_OUT2.digitalWrite(7, LOW);
  delay(200);
  digitalWrite(buzzer, LOW);
  pcf8574_OUT2.digitalWrite(7, HIGH);
  for (int i = 0; i < 21; i++) {
    leds1ST[i] = CRGB::Black;
    leds2nd[i] = CRGB::Black;
  }
  Display1stScore(); FastLED.show();
  // myDFPlayer1.playMp3Folder(12);

  score1 = 0;
  score2 = 0;
  score3 = 0;
  // timer = 60;
  Display1stScore(); FastLED.show();
  Display2ndScore(); FastLED.show();
  Display3rdScore(); FastLED.show();

  myDFPlayer2.stop();
  delay(200);
  myDFPlayer2.playMp3Folder(6);

  delay(6500);
  gameRunning = true;
  Serial.println("game starterd ");
  Serial.print("timer = ");
  Serial.println(timer);
  Serial.print("gstatus = ");
  Serial.println(Gstatus);
  Serial.print("game status = ");
  Serial.println(gameRunning);
  vTaskResume(timersTaskHandle);
  // stopTimers = false;

}
void loop() {
  //sendGameData(2, gameStatus, score, time);
  if (gameRunning) {

    Display2ndScore(); FastLED.show();
    gamelogic();

    if (Gstatus == 200 || timer < 1) {
      // gameRunning = false;
      Serial.println("recived end or time end ");
      sendGameData(device, 200, score1, 0);
      game_over();
    }
  }
  else {
    Serial.println("game running false in loop ");
    startup();
  }
}

void onhold() {
  Serial.println("on hold enter");
  stopTimers = false;
  while (1) {
    if (Gstatus == 200) {
      Serial.println("end during hold ");
      game_over();
      break;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // ✅ yield to other tasks
  }

}

void randomgenrate(int a, int b) {
  do {
    randomno = random(a, b);
  } while (previousno == randomno);
  previousno = randomno;
}

void timers() {
  currentTime = millis();
  if (currentTime - vallue >= 1000) {
    Serial.print(timer);
    Serial.println(" second");

    if (timer < 10) {
      for (int i = 0; i < NUM_LEDS3; i++) {
        leds3rd[i] = CRGB::Black;
      }
      FastLED.show();
    }
    portENTER_CRITICAL(&timerMux);
    timer--;
    score3 = timer;
    portEXIT_CRITICAL(&timerMux);

    Display3rdScore();
    FastLED.show();
    vallue = currentTime;

    if (timer < 40 && timer > 20 ) speed = 1000;
    if (timer < 20 )speed = 800;

    if (timer < 1) {
      //gameRunning = false;
      Serial.println("game over by timer ");
      timer = 0;
      vTaskSuspend(NULL); // suspend itself
    }

  }
}

int readswitch(int switchin, int chip) {
  if (chip == 0)
    return pcf8574_IN1.digitalRead(switchin);
  else
    return pcf8574_IN2.digitalRead(switchin);
}

void timersTask(void *parameter) {
  while (true) {
    timers();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void game_over() {
  portENTER_CRITICAL(&timerMux);
  timer = 0;
  portEXIT_CRITICAL(&timerMux);
  gameRunning = false;
  vTaskSuspend(timersTaskHandle);
  // stopTimers = true;
  Serial.println("Game Over");

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 8; i++) {
      pcf8574_OUT1.digitalWrite(i, (j % 2 == 0) ? LOW : HIGH);
      pcf8574_OUT2.digitalWrite(i, (j % 2 == 0) ? LOW : HIGH);
    }
    delay(500);
    // Serial.println("blink---");
  }
  // Serial.println("ddddd---");
  delay(1000);
  if (score1 > score2) {
    myDFPlayer2.playMp3Folder(19);
    delay(2500);
  }
  if (score2 > score1) {
    myDFPlayer2.playMp3Folder(18);
    delay(2500);
  }

}



void gamelogic() {
  if (!gameRunning) return;
  int randomPCF ;
  //  if (timer > 40 && timer < 60)randomPCF = 0;
  //  if (timer > 60)randomPCF = random(2);
  pass = 1;
  pass2 = 1;

  if (timer > 30) {
    randomPCF = 0;  // OUT1
  } else {
    randomPCF = random(2); // randomly OUT1 or OUT2
  }

  // Select outputs based on timer ranges
  if (timer > 30) {              // 60–31 sec
    randomgenrate(0, 6);         // 6 outputs (0–5)
    pcf8574_OUT1.digitalWrite(randomno, LOW);

  } else if (timer > 20) {       // 30–21 sec
    randomgenrate(0, 8);         // 8 outputs (0–7)
    pcf8574_OUT1.digitalWrite(randomno, LOW);

  } else if (timer > 10) {       // 20–11 sec
    randomgenrate(0, 10);        // 10 outputs (0–9)
    if (randomno < 8) {
      pcf8574_OUT1.digitalWrite(randomno, LOW);
    } else {
      pcf8574_OUT2.digitalWrite(randomno - 8, LOW);
    }

  } else {                       // 10–0 sec
    randomgenrate(0, 12);        // 12 outputs (0–11)
    if (randomno < 8) {
      pcf8574_OUT1.digitalWrite(randomno, LOW);
    } else {
      pcf8574_OUT2.digitalWrite(randomno - 8, LOW);
    }
  }


  // --- Wait for input ---
  unsigned long startLightTime = millis();
  while (millis() - startLightTime < speed) {
    relayTimer++;
    int chip = (randomno < 8) ? 0 : 1;         // Decide IN1 or IN2
    int inputPin = (randomno < 8) ? randomno : (randomno - 8);
    in = readswitch(inputPin, chip);

    delay(20);

    if (in == LOW && pass == 1) {
      score1++;
      sendGameData(device, 100, score1, 0);
      Serial.print("score1 = "); Serial.println(score1);

      digitalWrite(buzzer, HIGH);
      pcf8574_OUT2.digitalWrite(7, LOW);

      Display1stScore();
      FastLED.show();

      relayTimer = 0;
      pass = 0;
      break;
    }

    if (relayTimer == 5) {
      digitalWrite(buzzer, LOW);
      pcf8574_OUT2.digitalWrite(7, HIGH);
    }
  }

  // --- Reset the triggered output ---
  if (randomno < 8) {
    pcf8574_OUT1.digitalWrite(randomno, HIGH);
  } else {
    pcf8574_OUT2.digitalWrite(randomno - 8, HIGH);
  }
}

void Display1stScore() {
  int cursor;
  int digits_count = (score1 < 10) ? 1 : (score1 < 100) ? 2 : 3;
  for (int i = 1; i <= digits_count; i++) {
    int digit = (i == 1) ? score1 % 10 : (i == 2) ? (score1 / 10 % 10) : (score1 / 100 % 10);
    cursor = (i == 1) ? 14 : (i == 2) ? 7 : 0;
    for (int k = 0; k < 7; k++) {
      leds1ST[cursor++] = digits[digit][k] ? CRGB::Red : CRGB::Black;
    }
  }
}

void Display2ndScore() {


  int cursor;
  int digits_count = (score2 < 10) ? 1 : (score2 < 100) ? 2 : 3;
  for (int i = 1; i <= digits_count; i++) {
    int digit = (i == 1) ? score2 % 10 : (i == 2) ? (score2 / 10 % 10) : (score2 / 100 % 10);
    cursor = (i == 1) ? 14 : (i == 2) ? 7 : 0;
    for (int k = 0; k < 7; k++) {
      leds2nd[cursor++] = digits[digit][k] ? CRGB::Green : CRGB::Black;
    }
  }
}

void Display3rdScore() {
  int cursor;
  int digits_count = (score3 < 10) ? 1 : (score3 < 100) ? 2 : 3;
  for (int i = 1; i <= digits_count; i++) {
    int digit = (i == 1) ? score3 % 10 : (i == 2) ? (score3 / 10 % 10) : (score3 / 100 % 10);
    cursor = (i == 1) ? 14 : (i == 2) ? 7 : 0;
    for (int k = 0; k < 7; k++) {
      leds3rd[cursor++] = digits[digit][k] ? CRGB::Blue : CRGB::Black;
    }
  }
}
