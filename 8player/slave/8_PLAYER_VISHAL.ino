#include <Wire.h>
#include "game_comm.h"
#include "Arduino.h"
#include "PCF8574.h"
#include <FastLED.h>

#define BUTTON_PIN  39    // Start button
#define Buzzer 12
uint8_t device = 8;

// PCF8574 addresses
PCF8574 pcf8574_ONE(0x21);
PCF8574 pcf8574_IN1(0x20);

TaskHandle_t timersTaskHandle = NULL;
bool gameRunning = false;
bool onhold1 = false;
unsigned long gameStartTime;
unsigned long currentTime1 = 0;
unsigned long vallue = 0;
unsigned long previousMillis = 0;
unsigned long valueUpdateTime = 0;

int timer = 60;
int Speed = 800;
int Score1 = 0;

int randomno, previousno;
int randomno2, previousno2;

uint8_t Gstatus = 0;
uint8_t score2 = 0;

// ------------------- SEVEN SEGMENT CONFIG -------------------
#define NUM_LEDS_score 42  // 2 display × 21 LEDs
CRGB leds_score[NUM_LEDS_score];
char Fd1 = 0, Fd2 = 0;

byte digits[10][7] =  {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};



// Handle incoming game messages
void onGameMessage(struct_message data) {
  if (data.gameStatus == 100 && !gameRunning) {
    Serial.println(" Remote START received → Starting game!");
    gameRunning = true;
    vTaskResume(timersTaskHandle);
    timer = data.time;   // use sender’s timer
    Score1 = 0;
    valueUpdateTime = millis();
    sendGameData(device, 100, Score1, timer);
    digitalWrite(Buzzer, HIGH);
    delay(200);
    digitalWrite(Buzzer, LOW);
    Score1 = 0;
  }

  if (data.gameStatus == 200) {
    Serial.println(" Remote GAME OVER received → Stopping game!");
    gameRunning = false;
    onhold1 = false;
    timer = 0;
    vTaskSuspend(timersTaskHandle);

    // ✅ send final score
    sendGameData(device, 200, Score1, 0);


  }
  if (data.gameStatus == 300) {
    gameRunning = false;
    vTaskResume(timersTaskHandle);
    timer = data.time;   // use sender’s timer
    for (int i = 0; i < 8; i++) pcf8574_ONE.digitalWrite(i, HIGH);
    onhold1 = true;
    //data.gameStatus = 0;

  }
  if (data.gameStatus == 400) {
    gameRunning = false;
    for (int j = 0; j < 10; j++) {
      for (int i = 0; i < 8; i++) pcf8574_ONE.digitalWrite(i, LOW);
      delay(200);
      for (int i = 0; i < 8; i++) pcf8574_ONE.digitalWrite(i, HIGH);
      delay(200);
    }
  }
}



// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  setupGameComm();
  xTaskCreatePinnedToCore(
    timersTask,     // Function to implement the task
    "Timers Task",  // Name of the task
    2048,           // Stack size in words
    NULL,           // Task input parameter
    1,              // Priority of the task (1 = low)
    &timersTaskHandle,
    0               // Core where the task should run (0 or 1)
  );
  vTaskSuspend(timersTaskHandle);

  if (!pcf8574_ONE.begin()) Serial.println("PCF8574_ONE Failed");
  if (!pcf8574_IN1.begin()) Serial.println("PCF8574_IN1 Failed");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);

  for (int i = 0; i < 8; i++) pcf8574_ONE.pinMode(i, OUTPUT);
  for (int i = 0; i < 8; i++) pcf8574_IN1.pinMode(i, INPUT);
  for (int i = 0; i < 8; i++) pcf8574_ONE.digitalWrite(i, HIGH);


  // ✅ Setup LED Displays
  FastLED.addLeds<WS2812B, 27, GRB>(leds_score, NUM_LEDS_score);
  FastLED.setBrightness(255);


  startup();
}

// ---------------- Startup Animation & Start ----------------
void startup() {
  while (!gameRunning) {

    if(!onhold1){
    // simple LED chase animation
    for (int i = 0; i < 8; i++) {
      pcf8574_ONE.digitalWrite(i, LOW);
      delay(30);
      pcf8574_ONE.digitalWrite(i, HIGH);
    }
    }
    // check button
    if (digitalRead(BUTTON_PIN) == LOW) {
      // buzz to confirm
      digitalWrite(Buzzer, HIGH);
      delay(200);
      digitalWrite(Buzzer, LOW);
      Score1 = 0;

      // reset game values
      // timer = 90;

      previousMillis = 0;
      valueUpdateTime = millis();

      gameRunning = true;  // ✅ allow loop() to run
      sendGameData(device, 100, 0, timer);  // broadcast start
      vTaskResume(timersTaskHandle);
      break;  // ✅ exit while loop
    }
  }
}



void timersTask(void *parameter) {
  while (true) {
    timers();               // checks if 1s passed
    vTaskDelay(5 / portTICK_PERIOD_MS); // avoid starving CPU
  }
}

void timers() {
  currentTime1 = millis();
  if (currentTime1 - vallue >= 1000) {
    Serial.print(timer);
    Serial.println(" second");


    timer--;

    vallue = currentTime1;

    if (timer < 1) {
      onhold1 = false;
      vTaskSuspend(timersTaskHandle);
    }
  }
}
// ---------------- Main Loop ----------------
void loop() {
  if (gameRunning) {

    randomgenrate(0, 7);
    pcf8574_ONE.digitalWrite(randomno, LOW);
    unsigned long lightStart = millis();
    delay(80);

    while (millis() - lightStart < Speed) {
      int in1 = pcf8574_IN1.digitalRead(randomno);


      delay(20);
      if (in1 == LOW) {
        digitalWrite(Buzzer, HIGH);
        delay(100);
        digitalWrite(Buzzer, LOW);
        Score1++;
        sendGameData(device, 100, Score1, timer);
        break;
      }
    }

    pcf8574_ONE.digitalWrite(randomno, HIGH);
    delay(20);
    // ✅ Update Seven Segment Display
    Display1stScore();
    Display2ndTimer();
    FastLED.show();
    if (timer < 1) {
      gameRunning = false;
      previousMillis = 0;
      timer = 60;
      sendGameData(device , 200, Score1, 0);
      //      Game Over sound (3 beeps)
      for (int i = 0; i < 3; i++) {
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(500);
      }
      startup();

      return;
    }
  }
  else {
    startup();
  }

}

// ---------------- Random Generator ----------------
void randomgenrate(int a, int b) {
  randomno = random(a, b);
  while (previousno == randomno) {
    randomno = random(a, b);
  }
  previousno = randomno;
}

// ------------------- DISPLAY FUNCTIONS -------------------
// 1st Display → Score (LED 0–20)
void Display1stScore() {
  int cursor = 21;
  Fd1 = (Score1 < 10) ? 1 : (Score1 < 100) ? 2 : 3;

  for (int i = 1; i <= Fd1; i++) {
    int digit = 0;
    if (i == 1) {
      cursor = 14;
      digit = Score1 % 10;
    } else if (i == 2) {
      cursor = 7;
      digit = (Score1 / 10 % 10);
    } else if (i == 3) {
      cursor = 0;
      digit = (Score1 / 100 % 10);
    }
    for (int k = 0; k <= 6; k++) {
      leds_score[cursor] = digits[digit][k] ? CRGB::Fuchsia : CRGB::Black;
      cursor++;
    }
  }
}

// 2nd Display → Timer (LED 21–41)
void Display2ndTimer() {
  int cursor = 42;
  int digit;

  // Always 3 digits update karenge

  // Hundreds place
  cursor = 21;
  digit = (timer >= 100) ? (timer / 100 % 10) : -1;  // -1 => blank
  for (int k = 0; k <= 6; k++) {
    leds_score[cursor] = (digit == -1) ? CRGB::Black : (digits[digit][k] ? CRGB::Green : CRGB::Black);
    cursor++;
  }

  // Tens place
  cursor = 28;
  digit = (timer >= 10) ? (timer / 10 % 10) : -1;  // -1 => blank
  for (int k = 0; k <= 6; k++) {
    leds_score[cursor] = (digit == -1) ? CRGB::Black : (digits[digit][k] ? CRGB::Green : CRGB::Black);
    cursor++;
  }

  // Ones place (hamesha dikhana hai)
  cursor = 35;
  digit = timer % 10;
  for (int k = 0; k <= 6; k++) {
    leds_score[cursor] = digits[digit][k] ? CRGB::Green : CRGB::Black;
    cursor++;
  }
}
