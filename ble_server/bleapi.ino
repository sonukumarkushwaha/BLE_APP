#include "memorycard.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
// ==== Forward Declarations ====
void handleVerify(String ticket);
void handleSubmit(String qrDataAll);
void handleStatus(String gameId);
void handleTrigger(String command);
void handletimer(String timing);
int filterdata(String incoming_data);

void sendDataToUART(int players, bool start, int gametime);
void sendBLE(String msg);
void uartreceive();

// ================== BLE DEFINES ==================
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

// ================== GAME VARIABLES ==================
HardwareSerial MySerial(2);
int recivedtime = 60;
int counting = 0;
int players = 1;
int id = 0;
int status = 0;
int score = 0;
int timeValue = 0;
bool start = true;
String triggerCommand = "";
int triggertime = 0;
String userID;
String gameID;
String ticketID;


String gamestat = "end";
int recivedscore1 = 0;
int recivedscore2 = 0;
int recivedscore3 = 0;
int recivedscore4 = 0;
int recivedscore5 = 0;
int recivedscore6 = 0;
int recivedscore7 = 0;
int recivedscore8 = 0;

// ================== BLE SERVER CALLBACKS ==================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected");
      sendDataToUART(100, false, recivedtime);

    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client Disconnected, restarting advertising...");
      pServer->startAdvertising();
    }
};

// ================== BLE WRITE CALLBACK ==================
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        String cmd = String(rxValue.c_str());
        cmd.trim();   // 🔑 always trim
        Serial.println("Received over BLE: " + cmd);

        int sep = cmd.indexOf(':');
        if (sep == -1) return;  // invalid command

        String prefix = cmd.substring(0, sep);
        String data   = cmd.substring(sep + 1);

        if (prefix == "verify") {
          handleVerify(data);
        } else if (prefix == "submit") {
          handleSubmit(data);
        } else if (prefix == "status") {
          handleStatus(data);
        } else if (prefix == "trigger") {
          handleTrigger(data);
        } else if (prefix == "timer") {
          handletimer(data);
        }
      }
    }
};


// ================== HELPER: send data over UART2 ==================
void sendDataToUART(int players, bool start, int gametime) {
  String data = "{";
  data += "\"players\":" + String(players) + ",";
  data += "\"timer\":" + String(gametime) + ",";
  data += "\"start\":" + String(start ? "true" : "false");
  data += "}";
  MySerial.println(data);
  Serial.println("Sent over UART: " + data);
}

//// ================== BLE RESPONSE HELPER ==================
void sendBLE(String msg) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
    Serial.println("Sent over BLE: " + msg);
  }
}


// ================== OPERATIONS ==================

// Verify ticket
void handleVerify(String ticket) {
  Serial.println("Verify Request for Ticket: " + ticket);

  filterdata(ticket);
//  Serial.print("verifyind = ");
//  Serial.println(ticketID.toInt());
  // Simulated check (replace with your DB logic)
  findDataByTicketId(ticketID.toInt());

  if (Check_status == 2) {
    sendBLE("found");
  } else {
    sendBLE("notfound");

  }
}

// Submit multiple QR entries
void handleSubmit(String qrDataAll) {
  Serial.println("Submit Request: " + qrDataAll);

  int startIndex = 0;
  int endIndex = qrDataAll.indexOf(',');
  counting = 0;

  while (endIndex != -1) {
    String qrData = qrDataAll.substring(startIndex, endIndex);
    qrData.trim();
    Serial.println("Processing QR: " + qrData);

    startIndex = endIndex + 1;
    endIndex = qrDataAll.indexOf(',', startIndex);
    counting++;
    filterdata(qrData);
    saveData(userID, ticketID.toInt(), 0, gameID.toInt(), "men", "Qr_mode", 1, 0);
  }

  // Last entry
  String qrData = qrDataAll.substring(startIndex);
  qrData.trim();
  if (qrData.length() > 0) {
    Serial.println("Processing QR: " + qrData);
    counting++;

    players = counting;
    filterdata(qrData);
    saveData(userID, ticketID.toInt(), 0, gameID.toInt(), "men", "Qr_mode", 1, 0);
    sendDataToUART(counting, true, recivedtime);

  }

  sendBLE("ok");
}

// Game status
// Define scores for up to 8 players (global or local)
int scoresAll[8] = {
  recivedscore1, recivedscore2, recivedscore3, recivedscore4,
  recivedscore5, recivedscore6, recivedscore7, recivedscore8
};

void handleStatus(String gameId) {
  Serial.println("Status Request for Game: " + gameId);

  String gameStatus = gamestat;
  int gameTime = recivedtime;

  // ⚡ Build JSON dynamically based on "players"
  String jsonResponse = "{";
  jsonResponse += "\"game_status\":\"" + gameStatus + "\",";
  jsonResponse += "\"game_time\":" + String(gameTime) + ",";
  jsonResponse += "\"players\":" + String(players) + ",";
  jsonResponse += "\"scores\":[";

  for (int i = 0; i < players; i++) {
    jsonResponse += String(scoresAll[i]);
    if (i < players - 1) jsonResponse += ",";
  }

  jsonResponse += "]";
  jsonResponse += "}";

  sendBLE(jsonResponse);
}


// Trigger action
void handleTrigger(String command) {
  triggerCommand = command;
  triggerCommand.trim();
  Serial.println("Trigger command received: " + triggerCommand);
  //Serial.println("Game STOP triggered!"+ triggerCommand == "stop" );
  if (triggerCommand == "stop") {
    Serial.println("Game STOP triggered!");
    sendDataToUART(counting, false, recivedtime);
    gamestat = "end";
  } else if (triggerCommand == "restart") {
    Serial.println("Game START triggered!");
    gamestat = "running";
    sendDataToUART(counting, true, recivedtime);
  }
  sendBLE(command);
}

void handletimer(String timing) {
  timing.trim();
  Serial.print("Raw timing string: '");
  Serial.print(timing);
  Serial.println("'");

  triggertime = timing.toInt();

  Serial.print("Trigger time received: ");
  Serial.println(triggertime);

  if (triggertime > 10) {
    recivedtime = triggertime;
    Serial.print("timersaved=");
    Serial.println(recivedtime);
    sendDataToUART(0, false, recivedtime);
  }
  sendBLE("ok");
}




// Parse incoming QR data
int filterdata(String incoming_data) {
  incoming_data.trim();
  userID   = incoming_data.substring(0, incoming_data.indexOf('_'));
  gameID   = incoming_data.substring(incoming_data.indexOf('_') + 1, incoming_data.lastIndexOf('_'));
  ticketID = incoming_data.substring(incoming_data.lastIndexOf('_') + 1);

  Serial.println("User ID: " + userID);
  Serial.println("Game ID: " + gameID);
  Serial.println("Ticket ID: " + ticketID);


  return ticketID.toInt();
}

// UART receive from game hardware
void uartreceive() {
  if (MySerial.available()) {

    String data = MySerial.readStringUntil('\n');
    data.trim();
    Serial.println("Received UART: " + data);

    int idIndex = data.indexOf("\"id\":");
    if (idIndex != -1) {
      int commaIndex = data.indexOf(",", idIndex);
      String idStr = data.substring(idIndex + 5, commaIndex);
      id = idStr.toInt();
    }

    int statusIndex = data.indexOf("\"status\":");
    if (statusIndex != -1) {
      int commaIndex = data.indexOf(",", statusIndex);
      if (commaIndex == -1) commaIndex = data.indexOf("}", statusIndex);
      String statusStr = data.substring(statusIndex + 9, commaIndex);
      statusStr.replace("\"", "");
      status = statusStr.toInt();
    }

    int scoreIndex = data.indexOf("\"score\":");
    if (scoreIndex != -1) {
      int commaIndex = data.indexOf(",", scoreIndex);
      String scoreStr = data.substring(scoreIndex + 8, commaIndex);
      score = scoreStr.toInt();
    }

    int timeIndex = data.indexOf("\"time\":");
    if (timeIndex != -1) {
      String timeStr = data.substring(timeIndex + 7);
      timeStr.replace("}", "");
      timeValue = timeStr.toInt();
    }
    if (id >= 1 && id <= players) {
      scoresAll[id - 1] = score;   // store the score in correct index
      if (status == 100) gamestat = "running";
      if (status == 200) gamestat = "end";
    }

    if (id != 20) {
      handleStatus("20");
    }
    if (id == 20) {
      delay(2000);
      String jsonResponse = "{";
      jsonResponse += "\"max_players\":\"" + String(status) + "\",";
      jsonResponse += "\"GameID\":\"" + String(score) + "\",";
      jsonResponse += "\"default_time\":\"" + String(timeValue) + "\"";
      jsonResponse += "}";

      sendBLE(jsonResponse);

    }

    Serial.printf("ID=%d, Status=%d, Score=%d, Time=%d\n", id, status, score, timeValue);
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  initSDCard();
  createLogFile();
  MySerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("UART2 started on pins 16(RX),17(TX)");
  BLEDevice::init("Funtoo_batak");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristicRX = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                         );
  pCharacteristicRX->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE server started, waiting for client...");
}

// ================== LOOP ==================
void loop() {
  uartreceive();

}
