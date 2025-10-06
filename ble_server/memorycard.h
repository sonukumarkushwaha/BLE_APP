#ifndef MEMORYCARD_H
#define MEMORYCARD_H

#include <SPI.h>
#include <SD.h>


#define SD_CS 5
File dataFile;
uint8_t Check_status = 0;

bool initSDCard() {
  return SD.begin(SD_CS);
}

bool createLogFile() {
  if (!SD.exists("/log.csv")) {
    dataFile = SD.open("/log.csv", FILE_WRITE);
    if (dataFile) {
      // CSV header: userId, ticketId, score, gameId, mode, round, timestamp
      dataFile.println("userId,ticketId,score,gameId,mode,Qr_mode,round,time");
      dataFile.close();
      Serial.println("Header written to CSV.");
      return true;
    } else {
      Serial.println("Failed to create file!");
      return false;
    }
  }
  return true;
}

void saveData(String userId, int ticketId, int score, int gameId, String mode,String Qr_mode, int round, unsigned long timestamp) {
  Serial.println("saving data");
  dataFile = SD.open("/log.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(userId);
    dataFile.print(",");
    dataFile.print(ticketId);
    dataFile.print(",");
    dataFile.print(score);
    dataFile.print(",");
    dataFile.print(gameId);
    dataFile.print(",");
    dataFile.print(mode);
    dataFile.print(",");
    dataFile.print(Qr_mode);
    dataFile.print(",");
    dataFile.print(round);
    dataFile.print(",");
    dataFile.println(timestamp);
    dataFile.close();
    Serial.println("Data saved to CSV.");
  } else {
    Serial.println("Error opening file for writing!");
  }
}

// Search by ticketId instead of targetId
void findDataByTicketId(int targetTicketId) {
  File file = SD.open("/log.csv", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
   // display_print("E012", 30);
   // waitForBack("E012");
    return;
  }

  bool found = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.startsWith("userId")) continue; // Skip header

    // Split CSV line
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    int thirdComma = line.indexOf(',', secondComma + 1);
    int fourthComma = line.indexOf(',', thirdComma + 1);
    int fifthComma = line.indexOf(',', fourthComma + 1);
    int sixthComma = line.indexOf(',', fifthComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1 || fourthComma == -1 || fifthComma == -1 || sixthComma == -1)
      continue;

    int ticketId = line.substring(firstComma + 1, secondComma).toInt();

    if (ticketId == targetTicketId) {
      String userIdStr = line.substring(0, firstComma);
      String scoreStr  = line.substring(secondComma + 1, thirdComma);
      String gameIdStr = line.substring(thirdComma + 1, fourthComma);
      String modeStr   = line.substring(fourthComma + 1, fifthComma);
      String roundStr  = line.substring(fifthComma + 1, sixthComma);
      String timeStr   = line.substring(sixthComma + 1);

      Serial.println("Data found for Ticket ID: " + String(ticketId));
      Serial.println("User ID : " + userIdStr);
      Serial.println("Score   : " + scoreStr);
      Serial.println("Game ID : " + gameIdStr);
      Serial.println("Mode    : " + modeStr);
      Serial.println("Round   : " + roundStr);
      Serial.println("Time    : " + timeStr);
      found = true;
      Check_status = 2;
      break;
    }
  }

  if (!found) {
    Serial.println("Ticket ID not found in file.");
    Check_status = 1;
  }

  file.close();
}

#endif
