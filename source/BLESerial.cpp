#include "MicroBit.h"
#include "BirdBrain.h"
#include "BLESerial.h"
#include "Hummingbird.h"
#include "Finch.h"
#include "SpiControl.h"

//uint8_t ble_read_buff[BLE__MAX_PACKET_LENGTH]; // incoming commands from the tablet/computer
//uint8_t sensor_vals[BLE__MAX_PACKET_LENGTH]; // sensor data to send to the tablet/computer
bool bleConnected = false; // Holds if connected over BLE
bool notifyOn = false; // Holds if notifications are being sent
bool calibrationSuccess = false;
MicroBitUARTService *bleuart;

uint8_t sense_count = 0;

// Sends BLE sensor data every 30 ms
void send_ble_data()
{
    while(notifyOn) {
        assembleSensorData(); // assembles and sends a sensor packet
        fiber_sleep(30);
    }
    release_fiber();
}


void onConnected(MicroBitEvent)
{
    bleConnected = true;
    playConnectSound();
}

void onDisconnected(MicroBitEvent)
{
    bleConnected = false;
    notifyOn = false; // in case this was not reset by the computer/tablet
    flashOn = false; // Turning off any current message being printed to the screen
    playDisconnectSound();
}

void flashInitials()
{
    uint8_t count = 0;
    while(1)
    {
        if(!bleConnected) {
            // Print one of three initials
            uBit.display.printAsync(initials_name[count]);
            fiber_sleep(400);
            uBit.display.clear();
            fiber_sleep(200);
            count++;
            // If you're at 3, spend a longer time with display cleared so it's obvious which initial is first
            if(count ==3)
            {
                fiber_sleep(500);
                count = 0;
            }
        }
        else {
            fiber_sleep(1000);
            count = 0;
        }
    }
}

// Initializes the UART
void bleSerialInit(ManagedString devName) 
{
    bleuart = new MicroBitUARTService(*uBit.ble, 32, 32);

    fiber_sleep(10); //Give the UART service a moment to register

    // Configure advertising with the UART service added, and with our prefix added
    uBit.ble->configAdvertising(devName);
    
    // Waiting for the BLE stack to stabilize
    fiber_sleep(10);
    // Error checking code - uncomment if you need to check this
    //err= uBit.ble->configAdvertising(devName);
    //uint32_t *err;
    //char buffer[9];    
    /*
    for(int i = 0; i < 4; i++) {
        sprintf(buffer,"%lX",*(err+i)); //buffer now contains sn as a null terminated string
        ManagedString serialNumberAsHex(buffer);
        uBit.display.scroll(serialNumberAsHex);
    }*/
    uBit.ble->setTransmitPower(7); 
    uBit.ble->advertise();
    
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_DISCONNECTED, onDisconnected);
    create_fiber(flashInitials); // Start flashing since we're disconnected
}

// Checks what command (setAll, get firmware, etc) is coming over BLE, then acts as necessary
void bleSerialCommand()
{
    uint8_t buff_length = bleuart->rxBufferedSize(); // checking how many bytes we have in the buffer
    // Execute a command if we have data in our buffer and we're connected to a device
    if(bleConnected && (buff_length > 0))
    {
        uint8_t ble_read_buff[buff_length];
        bleuart->read(ble_read_buff, buff_length, ASYNC); 
        switch(ble_read_buff[0])
        {
            case SET_LEDARRAY:
                decodeAndSetDisplay(ble_read_buff, buff_length);
                break;
            case SET_FIRMWARE:
            case FINCH_SET_FIRMWARE:
                returnFirmwareData();
                //notifyOn = true; // hack to make BirdBlox happy
                break;
            case NOTIFICATIONS:
                //uBit.display.printAsync(ble_read_buff[1]);
                if(ble_read_buff[1] == START_NOTIFY) {
                    notifyOn = true;
                    create_fiber(send_ble_data); // Sends sensor data every 30 ms
                }
                else if(ble_read_buff[1] == STOP_NOTIFY) {
                    notifyOn = false;
                }
                break;
            case MICRO_IO:
                decodeAndSetPins(ble_read_buff);
                break;
            case STOP_ALL:
                stopMB(); // Stops the LED screen and buzzer, and if a MB sets edge connector pins to inputs
                if(whatAmI == A_HB)
                {
                    stopHB();
                }
            /*    uBit.display.print(buff_length);
                fiber_sleep(1000);
                for(int i = 0; i < buff_length; i++) {
                    uBit.display.print(ble_read_buff[i]);
                    fiber_sleep(1000);
                }*/
                break;
            case SET_CALIBRATE:
                // Turn off notifications while calibrating
                notifyOn = false;
                uBit.compass.calibrate();
                calibrationSuccess = true; // = uBit.compass.isCalibrated(); // probably not necessary
                notifyOn = true;
                create_fiber(send_ble_data);
                break;
                // Print unrecognized commands for debugging
            // Sets the Hummingbird outputs
            case SETALL_SPI:
                if(whatAmI == A_HB)
                {
                    setAllHB(ble_read_buff, buff_length);
                }
                break;
            // Sets the Finch LEDs + buzzer
            case FINCH_SETALL_LED:
                if(whatAmI == A_FINCH)
                {
                    setAllFinchLEDs(ble_read_buff, buff_length);
                }
                break;
            // Sets the Finch motors + LED screen, depending on mode
            case FINCH_SETALL_MOTORS_MLED:
                if(whatAmI == A_FINCH)
                {
                    setAllFinchMotorsAndLEDArray(ble_read_buff, buff_length);
                }
                break;    
            default:
                //uBit.display.printAsync(ble_read_buff[0]);
                break;
        }
    }

}

// Collects the notification data and sends it to the computer/tablet
void assembleSensorData()
{
    if(bleConnected && notifyOn)
    {
        // return dummy sensor data for now
        if(whatAmI == A_FINCH)
        {
            uint8_t sensor_vals[FINCH_SENSOR_SEND_LENGTH];
            uint8_t spi_sensors_only[FINCH_SPI_SENSOR_LENGTH];
            memset(sensor_vals, 0, FINCH_SENSOR_SEND_LENGTH);    
            
            spiReadFinch(spi_sensors_only);
            arrangeFinchSensors(spi_sensors_only, sensor_vals);

            getAccelerometerValsFinch(sensor_vals);
            getMagnetometerValsFinch(sensor_vals);
            getButtonValsFinch(sensor_vals);
            
            // Probably not necessary as we get feedback from the LED screen            
            if(calibrationSuccess)
            {
                sensor_vals[16] = sensor_vals[16] | 0x04;
            }

            bleuart->send(sensor_vals, sizeof(sensor_vals), ASYNC);
        }
        else
        {
            uint8_t sensor_vals[SENSOR_SEND_LENGTH];
            memset(sensor_vals, 0, SENSOR_SEND_LENGTH);
            // hard coding some sensor data
            if(whatAmI == A_MB)
            {
                getEdgeConnectorVals(sensor_vals);
                sensor_vals[3] = 0xFF; // no battery level reported
            }
            if(whatAmI == A_HB)
            {
                // reading Hummingbird sensors + battery level via SPI
                spiReadHB(sensor_vals);
            }
            getAccelerometerVals(sensor_vals);
            getMagnetometerVals(sensor_vals);
            getButtonVals(sensor_vals);
            
            // Probably not necessary as we get feedback from the LED screen            
            if(calibrationSuccess)
            {
                sensor_vals[7] = sensor_vals[7] | 0x04;
            }

            //send the data asynchronously
            bleuart->send(sensor_vals, sizeof(sensor_vals), ASYNC);
        }
    }
}

void returnFirmwareData()
{
    // hardware version is 1 for NXP, 2 for LS - currently uses LS
    // second byte is micro_firmware_version - 0x02 on V1
    // third byte is SAMD firmware version - 0 for MB, 3 for HB, 7 or 44 for Finch
    // Hard coding this for now
    uint8_t return_buff[3];
    return_buff[0] = 2;
    return_buff[1] = 2;
    if(whatAmI == A_MB)
    {
        return_buff[2] = 0xFF;
    } 
    else if(whatAmI == A_HB)
    {
        return_buff[2] = 3;
    } 
    else if(whatAmI == A_FINCH)
    {
        return_buff[2] = 44;
    } 
    else
    {
        return_buff[2] = 0;
    }
    bleuart->send(return_buff, 3, ASYNC);
}


// Plays the connect sound
void playConnectSound()
{
    // Plays the BirdBrain connect song, over the built-in speaker if a micro:bit, or over Finch/HB buzzer if not 
    if(whatAmI == A_MB)
    {
        uBit.io.speaker.setAnalogValue(512);
        uBit.io.speaker.setAnalogPeriodUs(3039);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(1912);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(1703);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(1351);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogValue(0);
    }
    else 
    {
        uBit.io.P0.setAnalogValue(512);
        uBit.io.P0.setAnalogPeriodUs(3039);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(1912);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(1703);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(1351);
        fiber_sleep(100);
        uBit.io.P0.setAnalogValue(0);
    }    
}

// Plays the disconnect sound
void playDisconnectSound()
{
    // Plays the BirdBrain disconnect song
    if(whatAmI == A_MB)
    {
        uBit.io.speaker.setAnalogValue(512);
        uBit.io.speaker.setAnalogPeriodUs(1702);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(2024);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(2551);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogPeriodUs(3816);
        fiber_sleep(100);
        uBit.io.speaker.setAnalogValue(0);
    }
    else
    {
        uBit.io.P0.setAnalogValue(512);
        uBit.io.P0.setAnalogPeriodUs(1702);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(2024);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(2551);
        fiber_sleep(100);
        uBit.io.P0.setAnalogPeriodUs(3816);
        fiber_sleep(100);
        uBit.io.P0.setAnalogValue(0);
    }    
}

