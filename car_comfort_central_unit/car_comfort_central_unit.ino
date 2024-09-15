#include <SPI.h>
#include <mcp2515.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Define GPIO pins
const int LED_FRONT = 2;  // Front LED pin
const int LED_REAR= 16;     // Back LED pin
const int LED_MIRROR = 15; // Mirror LED pin
const int LED_DASH = 17;  // Dash LED pin
const int LDR_SENSOR = 32; // Light Sensor pin
const int csPin = 5;      // MCP2515 CS pin
const int intPin = 27;    // MCP2515 INT pin
int consecutiveCount = 0;
const int requiredConsecutiveMessages = 5;

// Store the time
unsigned long frontDoorOpenTime = 0;
unsigned long rearDoorOpenTime = 0;
unsigned long UnlockCarTime = 0;
unsigned long brightnessMirrorLedDuration = 3000;
unsigned long brightnessInternalLedDuration = 9000; // 30 seconds in milliseconds

// CAN message buffer
struct can_frame canMsg;

// Create MCP2515 instance
MCP2515 mcp2515(csPin);   // Use CS pin 5

// Brightness levels
int fullBrightness = 255;   // Full brightness when door is open
int lowBrightness = 50;     // Dimmed brightness when door is closed
int CustomDrivingBrightnessFront = 10;
int CustomDrivingBrightnessRear = 10;
int DrivingBrightness = 50;
int DrivingBrightnessDash = 50;
int CustomDrivinBrightnessDash = 10;
int MirrorBrightness = 256;
int LightThreshold = 1000;
int LightLevel = 0;

// Variables to track the door state
bool FrontDoorOpen = false;
bool RearDoorOpen = false;     // Flag to check if door is open
bool FrontDoorOpened = false;
bool RearDoorOpened = false;
bool engineON = false;
bool SeparateControl = true;
bool DoorLocked = false;
bool KeyPressed = false;
bool SensorSwitch = false;

bool isInList(uint8_t value, const uint8_t* list, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (list[i] == value) {
      return true;
    }
  }
  return false;
}

// EEPROM addresses
const int EEPROM_ADDR_FULL_BRIGHTNESS = 0;
const int EEPROM_ADDR_LOW_BRIGHTNESS = 4;
const int EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_FRONT = 8;
const int EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_REAR = 12;
const int EEPROM_ADDR_DRIVING_BRIGHTNESS = 16;
const int EEPROM_ADDR_DRIVING_BRIGHTNESS_DASH = 20;
const int EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_DASH = 24;
const int EEPROM_ADDR_MIRROR_BRIGHTNESS = 28;
const int EEPROM_ADDR_LIGHT_THRESHOLD = 32;
const int EEPROM_ADDR_LIGHT_LEVEL = 36;

void loadParametersFromEEPROM() {
  fullBrightness = EEPROM.read(EEPROM_ADDR_FULL_BRIGHTNESS);
  lowBrightness = EEPROM.read(EEPROM_ADDR_LOW_BRIGHTNESS);
  CustomDrivingBrightnessFront = EEPROM.read(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_FRONT);
  CustomDrivingBrightnessRear = EEPROM.read(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_REAR);
  DrivingBrightness = EEPROM.read(EEPROM_ADDR_DRIVING_BRIGHTNESS);
  DrivingBrightnessDash = EEPROM.read(EEPROM_ADDR_DRIVING_BRIGHTNESS_DASH);
  CustomDrivinBrightnessDash = EEPROM.read(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_DASH);
  MirrorBrightness = EEPROM.read(EEPROM_ADDR_MIRROR_BRIGHTNESS);
  LightThreshold = EEPROM.read(EEPROM_ADDR_LIGHT_THRESHOLD);
  LightLevel = EEPROM.read(EEPROM_ADDR_LIGHT_LEVEL);
}

void saveParametersToEEPROM() {
  EEPROM.write(EEPROM_ADDR_FULL_BRIGHTNESS, fullBrightness);
  EEPROM.write(EEPROM_ADDR_LOW_BRIGHTNESS, lowBrightness);
  EEPROM.write(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_FRONT, CustomDrivingBrightnessFront);
  EEPROM.write(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_REAR, CustomDrivingBrightnessRear);
  EEPROM.write(EEPROM_ADDR_DRIVING_BRIGHTNESS, DrivingBrightness);
  EEPROM.write(EEPROM_ADDR_DRIVING_BRIGHTNESS_DASH, DrivingBrightnessDash);
  EEPROM.write(EEPROM_ADDR_CUSTOM_DRIVING_BRIGHTNESS_DASH, CustomDrivinBrightnessDash);
  EEPROM.write(EEPROM_ADDR_MIRROR_BRIGHTNESS, MirrorBrightness);
  EEPROM.write(EEPROM_ADDR_LIGHT_THRESHOLD, LightThreshold);
  EEPROM.write(EEPROM_ADDR_LIGHT_LEVEL, LightLevel);
}

// Function to set separate control (called when app sends the command)
void setSeparateControl(bool state) {
  SeparateControl = state;
}

void processLightSensor () {
  LightLevel = analogRead(LDR_SENSOR);
  if (LightLevel > LightThreshold) {
    SensorSwitch = true;
  } else {
    SensorSwitch = false;
  }
}

// Function to process the CAN message with ID 0x128
void processDoorLockStatus(const struct can_frame &canMsg, unsigned long currentMillis) {
  // Check the message with ID 0xF6 (for key press)
  if (canMsg.can_id == 0xF6) {

    if (canMsg.data[7] == 0x23) {
      KeyPressed = true;
      Serial.println("Key pressed.");
    }
  }
  // Check for the message with ID 0x128 (for door lock status)
  if (canMsg.can_id == 0x128) {

    if (canMsg.data[4] == 0x06) {
      consecutiveCount++;  // Increment the counter for consecutive 0x06 messages

      // If we have received enough consecutive 0x06 messages
      if ((consecutiveCount >= requiredConsecutiveMessages)) {
        if (!DoorLocked) {
          DoorLocked = true;  // Set DoorLocked to true
          KeyPressed = false;
          Serial.println("Door locked.");
        }
      }
    } else {
      // Reset logic if we receive a value other than 0x06
      if ((DoorLocked && consecutiveCount == 1)) {
        DoorLocked = false;  // Unlock the door if it was locked before
        KeyPressed = false;
        UnlockCarTime = currentMillis;  // Update the unlock time
        Serial.println("Door unlocked.");
      }
      consecutiveCount = 0;  // Reset consecutive counter for any value other than 0x06
    }
  }
}

void processCanMessages(void* pvParameters) {
  (void) pvParameters;
  while (true) {
  unsigned long currentMillis = millis();
  // List of codes for the front and rear doors
  const uint8_t frontDoorCodes[] = {0x40, 0x80, 0xF0, 0xC0, 0xA0, 0x90, 0x50, 0xB0, 0xE0, 0x70, 0xD0};
  const uint8_t rearDoorCodes[] = {0x10, 0x20, 0xF0, 0xA0, 0x90, 0x60, 0x50, 0x30, 0xB0, 0xE0, 0x70, 0xD0};
  
  processLightSensor ();

  // Reading CAN message
  if (digitalRead(intPin) == LOW) {
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      // Process Mirror Status
      processDoorLockStatus(canMsg, currentMillis);
      // Process door status
      if (canMsg.can_id == 0x220) {
        // Front door status
        if (isInList(canMsg.data[0], frontDoorCodes, sizeof(frontDoorCodes))) {
          if (!FrontDoorOpen) {   // Transition to door open
            FrontDoorOpen = true;
            FrontDoorOpened = true;
            frontDoorOpenTime = currentMillis;  // Record the moment the door opened
          }
        } else {
          FrontDoorOpen = false;
        }

        // Rear door status
        if (isInList(canMsg.data[0], rearDoorCodes, sizeof(rearDoorCodes))) {
          if (!RearDoorOpen) {    // Transition to door open
            RearDoorOpen = true;
            RearDoorOpened = true;
            rearDoorOpenTime = currentMillis;  // Record the moment the door opened
          }
        } else {
          RearDoorOpen = false;
        }
      }

      // Process engine status
      if (canMsg.can_id == 0x361) {
        if (canMsg.data[0] == 0x09) { // Adjust this byte/index to your specific CAN data format
          engineON = true;
        } else {
          engineON = false;
        }
      } 
    } else {
      Serial.println("Error reading CAN message.");
    }
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void processCANMessages(void* pvParameters) {
  (void) pvParameters;
  while (true) {
      if (!DoorLocked && engineON == false && SensorSwitch) {
    unsigned long elapsedTime = currentMillis - UnlockCarTime;
    if (elapsedTime <= brightnessMirrorLedDuration) {
      analogWrite(LED_MIRROR, MirrorBrightness);  // Turn on mirror LED
    } else if (engineON == false && (FrontDoorOpen || RearDoorOpen)){
      analogWrite(LED_MIRROR, MirrorBrightness);  // Turn off mirror LED after the duration
    } else {
      analogWrite(LED_MIRROR, 0);
    }
  } else {
    analogWrite(LED_MIRROR, 0);  // Keep mirror LED off if the car is locked
  }

  if (engineON && SensorSwitch) {
    if (SeparateControl) {
      // Control front and rear LEDs separately when the engine is ON
      analogWrite(LED_DASH, CustomDrivinBrightnessDash); // Use front brightness for driving
      analogWrite(LED_FRONT, CustomDrivingBrightnessFront); // Use front brightness for driving
      analogWrite(LED_REAR, CustomDrivingBrightnessRear);   // Use rear brightness for driving
    } else {
      // Unified control: Set both front and rear LEDs to the same brightness
      analogWrite(LED_DASH, DrivingBrightnessDash);
      analogWrite(LED_FRONT, DrivingBrightness);
      analogWrite(LED_REAR, DrivingBrightness);
    }
  } else {
    // If engine is OFF, control the brightness based on door status (same logic as before)
    if (FrontDoorOpen && SensorSwitch) {
      analogWrite(LED_REAR, lowBrightness);
      analogWrite(LED_DASH, fullBrightness);
      analogWrite(LED_FRONT, fullBrightness);
    } else if (FrontDoorOpened && currentMillis - frontDoorOpenTime <= brightnessInternalLedDuration) {
      analogWrite(LED_REAR, lowBrightness);
      analogWrite(LED_DASH, fullBrightness);
      analogWrite(LED_FRONT, fullBrightness);
    } else {
      analogWrite(LED_FRONT, 0);
      analogWrite(LED_REAR, 0);
      FrontDoorOpened = false;
    }

    if (RearDoorOpen && SensorSwitch) {
      analogWrite(LED_FRONT, lowBrightness);
      analogWrite(LED_REAR, fullBrightness);
    } else if (RearDoorOpened && currentMillis - rearDoorOpenTime <= brightnessInternalLedDuration) {
      analogWrite(LED_FRONT, lowBrightness);
      analogWrite(LED_REAR, fullBrightness);
    } else {
      analogWrite(LED_FRONT, 0);
      analogWrite(LED_REAR, 0);
      RearDoorOpened = false;
    }
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);

  EEPROM.begin(512);
  loadParametersFromEEPROM();
  // Set up CAN communication
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);  // Set CAN bus bitrate
  mcp2515.setNormalMode();

  // Set the ledPin as an output
  pinMode(LED_FRONT, OUTPUT);
  pinMode(LED_REAR, OUTPUT);
  pinMode(LED_MIRROR, OUTPUT);
  pinMode(LED_DASH, OUTPUT);
  
  // Set up the interrupt pin
  pinMode(intPin, INPUT);  // INT pin 27 is used for CAN interrupts

  // Start with LED at low brightness (assuming door is closed)
  analogWrite(LED_FRONT, 0);
  analogWrite(LED_REAR, 0);
  analogWrite(LED_MIRROR, 0);
  analogWrite(LED_DASH, 0);

  // Create tasks
  xTaskCreatePinnedToCore(processCANMessages, “CANMessages”, 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(processLogic, “LogicProcessing”, 2048, NULL, 1, NULL, 1);
}

void loop() {
  // unsigned long currentMillis = millis();
  // // List of codes for the front and rear doors
  // const uint8_t frontDoorCodes[] = {0x40, 0x80, 0xF0, 0xC0, 0xA0, 0x90, 0x50, 0xB0, 0xE0, 0x70, 0xD0};
  // const uint8_t rearDoorCodes[] = {0x10, 0x20, 0xF0, 0xA0, 0x90, 0x60, 0x50, 0x30, 0xB0, 0xE0, 0x70, 0xD0};
  
  // processLightSensor ();

  // // Reading CAN message
  // if (digitalRead(intPin) == LOW) {
  //   if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
  //     // Process Mirror Status
  //     processDoorLockStatus(canMsg, currentMillis);
  //     // Process door status
  //     if (canMsg.can_id == 0x220) {
  //       // Front door status
  //       if (isInList(canMsg.data[0], frontDoorCodes, sizeof(frontDoorCodes))) {
  //         if (!FrontDoorOpen) {   // Transition to door open
  //           FrontDoorOpen = true;
  //           FrontDoorOpened = true;
  //           frontDoorOpenTime = currentMillis;  // Record the moment the door opened
  //         }
  //       } else {
  //         FrontDoorOpen = false;
  //       }

  //       // Rear door status
  //       if (isInList(canMsg.data[0], rearDoorCodes, sizeof(rearDoorCodes))) {
  //         if (!RearDoorOpen) {    // Transition to door open
  //           RearDoorOpen = true;
  //           RearDoorOpened = true;
  //           rearDoorOpenTime = currentMillis;  // Record the moment the door opened
  //         }
  //       } else {
  //         RearDoorOpen = false;
  //       }
  //     }

  //     // Process engine status
  //     if (canMsg.can_id == 0x361) {
  //       if (canMsg.data[0] == 0x09) { // Adjust this byte/index to your specific CAN data format
  //         engineON = true;
  //       } else {
  //         engineON = false;
  //       }
  //     } 
  //   } else {
  //     Serial.println("Error reading CAN message.");
  //   }
  // }

  //LOGIC
  // if (!DoorLocked && engineON == false && SensorSwitch) {
  //   unsigned long elapsedTime = currentMillis - UnlockCarTime;
  //   if (elapsedTime <= brightnessMirrorLedDuration) {
  //     analogWrite(LED_MIRROR, MirrorBrightness);  // Turn on mirror LED
  //   } else if (engineON == false && (FrontDoorOpen || RearDoorOpen)){
  //     analogWrite(LED_MIRROR, MirrorBrightness);  // Turn off mirror LED after the duration
  //   } else {
  //     analogWrite(LED_MIRROR, 0);
  //   }
  // } else {
  //   analogWrite(LED_MIRROR, 0);  // Keep mirror LED off if the car is locked
  // }

  // if (engineON && SensorSwitch) {
  //   if (SeparateControl) {
  //     // Control front and rear LEDs separately when the engine is ON
  //     analogWrite(LED_DASH, CustomDrivinBrightnessDash); // Use front brightness for driving
  //     analogWrite(LED_FRONT, CustomDrivingBrightnessFront); // Use front brightness for driving
  //     analogWrite(LED_REAR, CustomDrivingBrightnessRear);   // Use rear brightness for driving
  //   } else {
  //     // Unified control: Set both front and rear LEDs to the same brightness
  //     analogWrite(LED_DASH, DrivingBrightnessDash);
  //     analogWrite(LED_FRONT, DrivingBrightness);
  //     analogWrite(LED_REAR, DrivingBrightness);
  //   }
  // } else {
  //   // If engine is OFF, control the brightness based on door status (same logic as before)
  //   if (FrontDoorOpen && SensorSwitch) {
  //     analogWrite(LED_REAR, lowBrightness);
  //     analogWrite(LED_DASH, fullBrightness);
  //     analogWrite(LED_FRONT, fullBrightness);
  //   } else if (FrontDoorOpened && currentMillis - frontDoorOpenTime <= brightnessInternalLedDuration) {
  //     analogWrite(LED_REAR, lowBrightness);
  //     analogWrite(LED_DASH, fullBrightness);
  //     analogWrite(LED_FRONT, fullBrightness);
  //   } else {
  //     analogWrite(LED_FRONT, 0);
  //     analogWrite(LED_REAR, 0);
  //     FrontDoorOpened = false;
  //   }

  //   if (RearDoorOpen && SensorSwitch) {
  //     analogWrite(LED_FRONT, lowBrightness);
  //     analogWrite(LED_REAR, fullBrightness);
  //   } else if (RearDoorOpened && currentMillis - rearDoorOpenTime <= brightnessInternalLedDuration) {
  //     analogWrite(LED_FRONT, lowBrightness);
  //     analogWrite(LED_REAR, fullBrightness);
  //   } else {
  //     analogWrite(LED_FRONT, 0);
  //     analogWrite(LED_REAR, 0);
  //     RearDoorOpened = false;
  //   }
  // }
}