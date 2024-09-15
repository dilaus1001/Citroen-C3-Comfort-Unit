#include <SPI.h>
#include <mcp2515.h>

//------------------------------------------------------------------------------
// CAN baud rates to test (in kbps)
const uint32_t CAN_BAUD_RATES[] = {33E3, 95E3, 125E3, 250E3, 500E3};
const int NUM_BAUD_RATES = sizeof(CAN_BAUD_RATES) / sizeof(CAN_BAUD_RATES[0]);

MCP2515 mcp2515(5);  // CS pin for MCP2515 on GPIO5
bool canMessageReceived = false;

//------------------------------------------------------------------------------
// Initialize MCP2515 with a given baud rate
bool initCAN(uint32_t baudRate) {
  mcp2515.reset();
  if (mcp2515.setBitrate(baudRate, MCP_8MHZ) == MCP2515::ERROR_OK) {
    mcp2515.setNormalMode();
    Serial.print("Testing Baud Rate: ");
    Serial.print(baudRate / 1000);
    Serial.println(" kbps");
    return true;
  } else {
    Serial.println("Failed to initialize CAN with baud rate");
    return false;
  }
}

//------------------------------------------------------------------------------
// Check if a CAN message is received
void checkCANMessage() {
  struct can_frame canMsg;
  
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {  // Message received
    Serial.println("CAN message received!");
    Serial.print("ID: ");
    Serial.println(canMsg.can_id, HEX);
    Serial.print("DLC: ");
    Serial.println(canMsg.can_dlc);
    
    Serial.print("Data: ");
    for (int i = 0; i < canMsg.can_dlc; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    canMessageReceived = true;
  }
}

//------------------------------------------------------------------------------
// Test different baud rates
void testBaudRates() {
  for (int i = 0; i < NUM_BAUD_RATES; i++) {
    if (initCAN(CAN_BAUD_RATES[i])) {
      Serial.println("Listening for CAN messages...");
      
      unsigned long startMillis = millis();
      while (millis() - startMillis < 5000) {  // Wait for 5 seconds at each baud rate
        checkCANMessage();
        if (canMessageReceived) {
          Serial.print("Success with Baud Rate: ");
          Serial.print(CAN_BAUD_RATES[i] / 1000);
          Serial.println(" kbps");
          return;  // Exit the loop if message received
        }
      }
    }
    Serial.println("No messages received at this baud rate.\n");
  }
  Serial.println("No valid baud rate found!");
}

//------------------------------------------------------------------------------
// Setup
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect
  }
  
  Serial.println("Starting CAN Baud Rate Test");
  SPI.begin();
  
  testBaudRates();  // Start testing CAN baud rates
}

//------------------------------------------------------------------------------
// Loop
void loop() {
  // Do nothing. All work is done in setup.
}
