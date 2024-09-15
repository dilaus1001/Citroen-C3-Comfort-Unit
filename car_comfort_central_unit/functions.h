#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>

// Function declarations
void simulateCANMessage(const String &command);

// Other necessary variable declarations and constants
extern MCP2515 mcp2515; // Declare the MCP2515 instance for external use

#endif