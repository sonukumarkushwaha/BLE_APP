#ifndef GAME_COMM_H
#define GAME_COMM_H

#include <WiFi.h>
#include <esp_now.h>

// ========== Configuration ==========
#define NUM_PEERS 5
uint8_t peerAddresses[NUM_PEERS][6] = {
  {0xF8, 0xB3, 0xB7, 0xC6, 0x2F, 0xFC}, // 1st
  {0xF8, 0xB3, 0xB7, 0xC6, 0x00, 0x7C},   //2nd
  {0xF8, 0xB3, 0xB7, 0xC5, 0xFB, 0x64}, //3rd
  {0xF8, 0xB3, 0xB7, 0xC6, 0x2F, 0x30},  //4TH
  {0xF8, 0xB3, 0xB7, 0xC6, 0x00, 0x84} //5TH
};
// ===================================

typedef struct struct_message {
  int id;
  int gameStatus;
  int score;
  int time;
} struct_message;

extern void onGameMessage(struct_message data);

// Internal function
inline void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  // Optional debug:
  
    Serial.printf("Received from %02X:%02X:%02X:%02X:%02X:%02X - ID: %d, Status: %d, Score: %d, Time: %d\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                receivedData.id, receivedData.gameStatus, receivedData.score, receivedData.time);
  

  onGameMessage(receivedData);
}

void sendGameDataToPeer(int peerIndex, int fixedId, int status, int currentScore, int currentTime) {
  if (peerIndex < 0 || peerIndex >= NUM_PEERS) {
    Serial.println("❌ Invalid peer index");
    return;
  }

  struct_message gameData = {fixedId, status, currentScore, currentTime};

  esp_err_t result = esp_now_send(peerAddresses[peerIndex], (uint8_t *)&gameData, sizeof(gameData));
  if (result == ESP_OK) {
    Serial.printf("✅ Sent to peer[%d] %02X:%02X:%02X:%02X:%02X:%02X\n",
                  peerIndex + 1,
                  peerAddresses[peerIndex][0], peerAddresses[peerIndex][1],
                  peerAddresses[peerIndex][2], peerAddresses[peerIndex][3],
                  peerAddresses[peerIndex][4], peerAddresses[peerIndex][5]);
  } else {
    Serial.printf("❌ Send error to peer[%d], code: %d\n", peerIndex + 1, result);
  }
}

inline void setupGameComm() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    ESP.restart();
  }

  esp_now_register_recv_cb(OnDataRecv);

  for (int i = 0; i < NUM_PEERS; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.printf("❌ Failed to add peer %02X:%02X:%02X:%02X:%02X:%02X\n",
                    peerAddresses[i][0], peerAddresses[i][1], peerAddresses[i][2],
                    peerAddresses[i][3], peerAddresses[i][4], peerAddresses[i][5]);
    } else {
      Serial.printf("✅ Added peer %02X:%02X:%02X:%02X:%02X:%02X\n",
                    peerAddresses[i][0], peerAddresses[i][1], peerAddresses[i][2],
                    peerAddresses[i][3], peerAddresses[i][4], peerAddresses[i][5]);
    }
  }
}

inline void sendGameData(int fixedId, int status, int currentScore, int currentTime) {
  struct_message gameData = {fixedId, status, currentScore, currentTime};

  for (int i = 0; i < NUM_PEERS; i++) {
    esp_err_t result = esp_now_send(peerAddresses[i], (uint8_t *)&gameData, sizeof(gameData));
    if (result == ESP_OK) {
      Serial.printf("✅ Send success to %02X:%02X:%02X:%02X:%02X:%02X\n",
                    peerAddresses[i][0], peerAddresses[i][1], peerAddresses[i][2],
                    peerAddresses[i][3], peerAddresses[i][4], peerAddresses[i][5]);
    } else {
      Serial.printf("❌ Send error to %02X:%02X:%02X:%02X:%02X:%02X, code: %d\n",
                    peerAddresses[i][0], peerAddresses[i][1], peerAddresses[i][2],
                    peerAddresses[i][3], peerAddresses[i][4], peerAddresses[i][5], result);
    }
  }
}

#endif  // GAME_COMM_H
