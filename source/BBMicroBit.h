#ifndef BBMICROBIT_H
#define BBMICROBIT_H

#include "BLESerial.h"

//ElecFreaks PlanetX AiLens
#define AILENS											(0x14 << 1)
#define AILENS_DATA_LENGTH								9
//ElecFreaks Speech Recognition
#define ASR												(0x0B << 1)
//Commands for ElecFreaks
#define NO_COMMAND										0
#define ASR_LEARN										50
#define ASR_CLEAR_LEARNING								60
#define AILENS_LEARN_OBJECT1							101
#define AILENS_LEARN_OBJECT2							102
#define AILENS_LEARN_OBJECT3							103
#define AILENS_LEARN_OBJECT4							104
#define AILENS_LEARN_OBJECT5							105




// Function that decodes the display command
void BBMicroBitInit();
void decodeAndSetDisplay(uint8_t displayCommands[], uint8_t commandLength);
// This function sets the edge connector pins or internal buzzer
void decodeAndSetPins(uint8_t displayCommands[]);

// Gets the edge connector pins as analog inputs, if they have been set as an analog inputs
void getEdgeConnectorVals(uint8_t (&sensor_vals)[V2_SENSOR_SEND_LENGTH]);

// Get and convert the accelerometer values to 8 bit format, and check if the shake bit should be set
void getAccelerometerVals(uint8_t (&sensor_vals)[V2_SENSOR_SEND_LENGTH]);

// Get and convert the magnetometer values to a 16 bit format
void getMagnetometerVals(uint8_t (&sensor_vals)[V2_SENSOR_SEND_LENGTH]);

// Get the state of the buttons
void getButtonVals(uint8_t (&sensor_vals)[V2_SENSOR_SEND_LENGTH], bool V2Notification);

// Get and convert the accelerometer values to 8 bit format, and check if the shake bit should be set
void getAccelerometerValsFinch(uint8_t (&sensor_vals)[FINCH_SENSOR_SEND_LENGTH]);

// Get and convert the magnetometer values to a 16 bit format
void getMagnetometerValsFinch(uint8_t (&sensor_vals)[FINCH_SENSOR_SEND_LENGTH]);

// Get the state of the buttons
void getButtonValsFinch(uint8_t (&sensor_vals)[FINCH_SENSOR_SEND_LENGTH], bool V2Notification);


// Turns off edge connector, buzzer, LED screen
void stopMB();

// Function used for setting the buzzer for Hummingbird and Finch
void setBuzzer(uint16_t period, uint16_t duration);

// ElecFreaks Function
void getI2CVals(uint8_t (&sensor_vals)[V2_SENSOR_SEND_LENGTH]);

#endif