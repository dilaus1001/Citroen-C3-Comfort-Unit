#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(5);  // Set the CS pin (GPIO5)

void setup() {
  Serial.begin(115200);
  
  // Initialize SPI communication and CAN controller
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);  // Set bitrate to 500 kbps, clock to 8MHz
  mcp2515.setNormalMode();  // Set the MCP2515 to normal mode

  Serial.println("MCP2515 Initialized for CAN communication.");
}

void loop() {
  // Check if there is a CAN message
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    // A message was read successfully
    Serial.print("CAN ID: ");
    Serial.println(canMsg.can_id, HEX);

    Serial.print("Data: ");
    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}