#line 1 "/Users/francesco/Repository/Citroen-C3-Comfort-Unit/car_comfort_central_unit/functions.cpp"
#include "functions.h"

void simulateCANMessage(const String &command) {
    struct can_frame canMsg;

    if (command.equals("FR")) {  // Use .equals() for comparing String objects
        canMsg.can_id = 0x220;
        canMsg.can_dlc = 8;
        canMsg.data[0] = 0x40; // Front door open code
        Serial.println("Simulating Front Door Open CAN message.");
    } else if (command.equals("RR")) {  // Use .equals() for comparing String objects
        canMsg.can_id = 0x220;
        canMsg.can_dlc = 8;
        canMsg.data[0] = 0x10; // Rear door open code
        Serial.println("Simulating Rear Door Open CAN message.");
    } else if (command.equals("LOCK")) {  // Use .equals() for comparing String objects
        canMsg.can_id = 0x128;
        canMsg.can_dlc = 8;
        canMsg.data[4] = 0x06; // Door locked code
        Serial.println("Simulating Door Locked CAN message.");
    } else if (command.equals("ENGINE")) {  // Use .equals() for comparing String objects
        canMsg.can_id = 0x128;
        canMsg.can_dlc = 8;
        canMsg.data[4] = 0x08; // Engine on code
        Serial.println("Simulating Engine ON CAN message.");
    }

    // Send the CAN message
    mcp2515.sendMessage(&canMsg);
}