#include "game_comm.h"
#define ir1 35
#define ir2 36
#define ir3 39

#define out1 14
#define out2 13
#define out3 4

#define solenoide 5

const int deviceID = 5;   // constant ID
int gameStatus = 200;
void setup() {
  Serial.begin(115200);
  setupGameComm();
  pinMode(ir1, INPUT_PULLUP);
  pinMode(ir2, INPUT_PULLUP);
  pinMode(ir3, INPUT_PULLUP);

  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  pinMode(out3, OUTPUT);
  pinMode(solenoide, OUTPUT);
 
  startup();
}


// Handle incoming game messages
void onGameMessage(struct_message data) {
  //recivedid= data.id;
  //recivedscore = data.score;
  //recived status = data.gameStatus;
  //recived time = data.time;
  gameStatus = data.gameStatus;
}

void startup() {
  int i = 0;
  digitalWrite(solenoide, LOW);
  while (1) {
    
    
    i++;
    Serial.println(i);
    if (i == 1)digitalWrite(out1, HIGH);
    if (i == 2)digitalWrite(out2, HIGH);
    if (i == 3)digitalWrite(out3, HIGH);
    if (i == 4)i = 0;
    delay(50);
    digitalWrite(out1, LOW);
    digitalWrite(out2, LOW);
    digitalWrite(out3, LOW);
    if(gameStatus==100)break;
  }
  Serial.println("game started");
  
  digitalWrite(out1, HIGH);
  digitalWrite(out2, HIGH);
  digitalWrite(out3, HIGH);
  digitalWrite(solenoide, HIGH);
}

void loop() {
if(gameStatus==200)startup();

if(digitalRead(ir1)==LOW){
    sendGameData(deviceID, gameStatus, 1, 0);
    delay(50);
  }
  if(digitalRead(ir2)==LOW){
    sendGameData(deviceID, gameStatus, 2, 0);
    delay(50);
  }
  if(digitalRead(ir3)==LOW){
    sendGameData(deviceID, gameStatus, 3, 0);
    delay(50);
  }
  
}
