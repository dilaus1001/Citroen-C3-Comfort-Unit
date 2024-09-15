// // Developer: 
// //        Adam Varga, 2020, All rights reserved.
// // Licence: 
// //        Licenced under the MIT licence. See LICENCE file in the project root.
// // Usage of this code: 
// //        This code creates the interface between the car
// //        and the canSniffer_GUI application. If the RANDOM_CAN
// //        define is set to 1, this code is generating random
// //        CAN packets in order to test the higher level code.
// //        The received packets will be echoed back. If the 
// //        RANDOM_CAN define is set to 0, the CAN_SPEED define 
// //        has to match the speed of the desired CAN channel in
// //        order to receive and transfer from and to the CAN bus.
// //        Serial speed is 250000baud <- might need to be increased.
// // Required arduino packages: 
// //        - CAN by Sandeep Mistry (https://github.com/sandeepmistry/arduino-CAN)
// // Required modifications: 
// //        - MCP2515.h: 16e6 clock frequency reduced to 8e6 (depending on MCP2515 clock)
// //        - MCP2515.cpp: extend CNF_MAPPER with your desired CAN speeds
// //------------------------------------------------------------------------------
// #include <SPI.h>
// #include <mcp2515.h>

// //------------------------------------------------------------------------------
// // Settings
// #define RANDOM_CAN 0
// #define CAN_SPEED (125E3) //LOW=33E3, MID=95E3, HIGH=500E3 (for Vectra)
// //------------------------------------------------------------------------------
// // Inits, globals
// MCP2515 mcp2515(5);  // CS pin for MCP2515 on GPIO5

// typedef struct {
//   long id;
//   byte rtr;
//   byte ide;
//   byte dlc;
//   byte dataArray[20];
// } packet_t;

// const char SEPARATOR = ',';
// const char TERMINATOR = '\n';
// const char RXBUF_LEN = 100;

// //------------------------------------------------------------------------------
// // Printing a packet to serial
// void printHex(long num) {
//   if (num < 0x10) {
//     Serial.print("0");
//   }
//   Serial.print(num, HEX);
// }

// void printPacket(packet_t *packet) {
//   // packet format (hex string): [ID],[RTR],[IDE],[DATABYTES 0..8B]\n
//   // example: 014A,00,00,1A002B003C004D\n
//   printHex(packet->id);
//   Serial.print(SEPARATOR);
//   printHex(packet->rtr);
//   Serial.print(SEPARATOR);
//   printHex(packet->ide);
//   Serial.print(SEPARATOR);
//   // DLC is determined by number of data bytes, format: [00]
//   for (int i = 0; i < packet->dlc; i++) {
//     printHex(packet->dataArray[i]);
//   }
//   Serial.print(TERMINATOR);
// }

// //------------------------------------------------------------------------------
// // CAN packet simulator
// void CANsimulate(void) {
//   packet_t txPacket;

//   long sampleIdList[] = {0x110, 0x18DAF111, 0x23A, 0x257, 0x412F1A1, 0x601, 0x18EA0C11};
//   int idIndex = random(sizeof(sampleIdList) / sizeof(sampleIdList[0]));
//   int sampleData[] = {0xA, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0xA0, 0xB1};

//   txPacket.id = sampleIdList[idIndex];
//   txPacket.ide = txPacket.id > 0x7FF ? 1 : 0;
//   txPacket.rtr = 0;
//   txPacket.dlc = random(1, 9);

//   for (int i = 0; i < txPacket.dlc; i++) {
//     int changeByte = random(4);
//     if (changeByte == 0) {
//       sampleData[i] = random(256);
//     }
//     txPacket.dataArray[i] = sampleData[i];
//   }

//   printPacket(&txPacket);
// }

// //------------------------------------------------------------------------------
// // CAN RX, TX
// void onCANReceive() {
//   // Create a structure for received data
//   struct can_frame rxFrame;
  
//   if (mcp2515.readMessage(&rxFrame) == MCP2515::ERROR_OK) { // Successfully received
//     packet_t rxPacket;
//     rxPacket.id = rxFrame.can_id;
//     rxPacket.rtr = rxFrame.can_id & CAN_RTR_FLAG ? 1 : 0;
//     rxPacket.ide = rxFrame.can_id & CAN_EFF_FLAG ? 1 : 0;
//     rxPacket.dlc = rxFrame.can_dlc;

//     for (int i = 0; i < rxFrame.can_dlc; i++) {
//       rxPacket.dataArray[i] = rxFrame.data[i];
//     }

//     printPacket(&rxPacket);
//   }
// }

// void sendPacketToCan(packet_t *packet) {
//   struct can_frame txFrame;
//   txFrame.can_id = packet->id;
//   txFrame.can_dlc = packet->dlc;

//   for (int i = 0; i < packet->dlc; i++) {
//     txFrame.data[i] = packet->dataArray[i];
//   }

//   mcp2515.sendMessage(&txFrame);  // Send the CAN message
// }

// //------------------------------------------------------------------------------
// // Serial parser
// char getNum(char c) {
//   if (c >= '0' && c <= '9') {
//     return c - '0';
//   }
//   if (c >= 'a' && c <= 'f') {
//     return c - 'a' + 10;
//   }
//   if (c >= 'A' && c <= 'F') {
//     return c - 'A' + 10;
//   }
//   return 0;
// }

// char *strToHex(char *str, byte *hexArray, byte *len) {
//   byte *ptr = hexArray;
//   char *idx;
//   for (idx = str; *idx != SEPARATOR && *idx != TERMINATOR; ++idx, ++ptr) {
//     *ptr = (getNum(*idx++) << 4) + getNum(*idx);
//   }
//   *len = ptr - hexArray;
//   return idx;
// }

// void rxParse(char *buf, int len) {
//   packet_t rxPacket;
//   char *ptr = buf;
//   // All elements have to have leading zero!

//   // ID
//   byte idTempArray[8], tempLen;
//   ptr = strToHex(ptr, idTempArray, &tempLen);
//   rxPacket.id = 0;
//   for (int i = 0; i < tempLen; i++) {
//     rxPacket.id |= (long)idTempArray[i] << ((tempLen - i - 1) * 8);
//   }

//   // RTR
//   ptr = strToHex(ptr + 1, &rxPacket.rtr, &tempLen);

//   // IDE
//   ptr = strToHex(ptr + 1, &rxPacket.ide, &tempLen);

//   // DATA
//   ptr = strToHex(ptr + 1, rxPacket.dataArray, &rxPacket.dlc);

// #if RANDOM_CAN == 1
//   // echo back
//   printPacket(&rxPacket);
// #else
//   sendPacketToCan(&rxPacket);
// #endif
// }

// void RXcallback(void) {
//   static int rxPtr = 0;
//   static char rxBuf[RXBUF_LEN];

//   while (Serial.available() > 0) {
//     if (rxPtr >= RXBUF_LEN) {
//       rxPtr = 0;
//     }
//     char c = Serial.read();
//     rxBuf[rxPtr++] = c;
//     if (c == TERMINATOR) {
//       rxParse(rxBuf, rxPtr);
//       rxPtr = 0;
//     }
//   }
// }

// //------------------------------------------------------------------------------
// // Setup
// void setup() {
//   Serial.begin(250000);
//   while (!Serial) {
//     ;  // wait for serial port to connect. Needed for native USB port only
//   }

//   // Initialize MCP2515
//   SPI.begin();
//   mcp2515.reset();
//   mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);  // Adjust bitrate and clock as needed
//   mcp2515.setNormalMode();                    // Start the MCP2515 in normal mode

// #if RANDOM_CAN == 1
//   randomSeed(12345);
//   Serial.println("randomCAN Started");
// #else
//   Serial.println("CAN RX TX Started");
// #endif
// }

// //------------------------------------------------------------------------------
// // Main
// void loop() {
//   RXcallback();

// #if RANDOM_CAN == 1
//   CANsimulate();
//   delay(100);
// #else
//   onCANReceive();  // Continuously check for CAN messages
// #endif
// }



#include <SPI.h>
#include <mcp_can.h>

//------------------------------------------------------------------------------
// Settings
#define RANDOM_CAN 1
#define CAN_SPEED (125E3) //LOW=33E3, MID=95E3, HIGH=500E3 (for Vectra)
#define CAN_CS_PIN 5  // Chip select pin for MCP2515

//------------------------------------------------------------------------------
// Inits, globals
MCP_CAN CAN(CAN_CS_PIN);  // Set CS pin for MCP2515

typedef struct {
  long id;
  byte rtr;
  byte ide;
  byte dlc;
  byte dataArray[8];
} packet_t;

const char SEPARATOR = ',';
const char TERMINATOR = '\n';
const char RXBUF_LEN = 100;

//------------------------------------------------------------------------------
// Printing a packet to serial
void printHex(long num) {
  if (num < 0x10) {
    Serial.print("0");
  }
  Serial.print(num, HEX);
}

void printPacket(packet_t *packet) {
  printHex(packet->id);
  Serial.print(SEPARATOR);
  printHex(packet->rtr);
  Serial.print(SEPARATOR);
  printHex(packet->ide);
  Serial.print(SEPARATOR);
  for (int i = 0; i < packet->dlc; i++) {
    printHex(packet->dataArray[i]);
  }
  Serial.print(TERMINATOR);
}

//------------------------------------------------------------------------------
// CAN packet simulator
void CANsimulate(void) {
  packet_t txPacket;

  long sampleIdList[] = {0x110, 0x18DAF111, 0x23A, 0x257, 0x412F1A1, 0x601, 0x18EA0C11};
  int idIndex = random(sizeof(sampleIdList) / sizeof(sampleIdList[0]));
  int sampleData[] = {0xA, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0xA0, 0xB1};

  txPacket.id = sampleIdList[idIndex];
  txPacket.ide = txPacket.id > 0x7FF ? 1 : 0;
  txPacket.rtr = 0;
  txPacket.dlc = random(1, 9);

  for (int i = 0; i < txPacket.dlc; i++) {
    int changeByte = random(4);
    if (changeByte == 0) {
      sampleData[i] = random(256);
    }
    txPacket.dataArray[i] = sampleData[i];
  }

  printPacket(&txPacket);
}

//------------------------------------------------------------------------------
// CAN RX, TX
void onCANReceive() {
  packet_t rxPacket;
  unsigned long rxId = 0;
  byte len = 0;
  byte rxBuf[8];

  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&rxId, &len, rxBuf);  // Read the message

    rxPacket.id = rxId;
    rxPacket.rtr = (rxId & 0x40000000) ? 1 : 0;
    rxPacket.ide = (rxId & 0x80000000) ? 1 : 0;
    rxPacket.dlc = len;

    for (int i = 0; i < len; i++) {
      rxPacket.dataArray[i] = rxBuf[i];
    }

    printPacket(&rxPacket);
  }
}

void sendPacketToCan(packet_t *packet) {
  CAN.sendMsgBuf(packet->id, packet->ide, packet->dlc, packet->dataArray);
}

//------------------------------------------------------------------------------
// Serial parser
char getNum(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return 0;
}

char *strToHex(char *str, byte *hexArray, byte *len) {
  byte *ptr = hexArray;
  char *idx;
  for (idx = str; *idx != SEPARATOR && *idx != TERMINATOR; ++idx, ++ptr) {
    *ptr = (getNum(*idx++) << 4) + getNum(*idx);
  }
  *len = ptr - hexArray;
  return idx;
}

void rxParse(char *buf, int len) {
  packet_t rxPacket;
  char *ptr = buf;
  byte idTempArray[8], tempLen;

  // ID
  ptr = strToHex(ptr, idTempArray, &tempLen);
  rxPacket.id = 0;
  for (int i = 0; i < tempLen; i++) {
    rxPacket.id |= (long)idTempArray[i] << ((tempLen - i - 1) * 8);
  }

  // RTR
  ptr = strToHex(ptr + 1, &rxPacket.rtr, &tempLen);

  // IDE
  ptr = strToHex(ptr + 1, &rxPacket.ide, &tempLen);

  // DATA
  ptr = strToHex(ptr + 1, rxPacket.dataArray, &rxPacket.dlc);

#if RANDOM_CAN == 1
  // echo back
  printPacket(&rxPacket);
#else
  sendPacketToCan(&rxPacket);
#endif
}

void RXcallback(void) {
  static int rxPtr = 0;
  static char rxBuf[RXBUF_LEN];

  while (Serial.available() > 0) {
    if (rxPtr >= RXBUF_LEN) {
      rxPtr = 0;
    }
    char c = Serial.read();
    rxBuf[rxPtr++] = c;
    if (c == TERMINATOR) {
      rxParse(rxBuf, rxPtr);
      rxPtr = 0;
    }
  }
}

//------------------------------------------------------------------------------
// Setup
void setup() {
  Serial.begin(250000);
  while (!Serial) {
    ;  // wait for serial port to connect
  }

  // Initialize MCP2515
  SPI.begin();
  if (CAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Initialized");
  } else {
    Serial.println("CAN Init Failed");
  }
  CAN.setMode(MCP_NORMAL);

#if RANDOM_CAN == 1
  randomSeed(12345);
  Serial.println("randomCAN Started");
#else
  Serial.println("CAN RX TX Started");
#endif
}

//------------------------------------------------------------------------------
// Main
void loop() {
  RXcallback();

#if RANDOM_CAN == 1
  CANsimulate();
  delay(100);
#else
  onCANReceive();
#endif
}