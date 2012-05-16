#include <avr/io.h>

#include "I2C.h"
#include "ds1307.h"

/***************************************************

Function To Read Internal Registers of DS1307
---------------------------------------------

address : address of the register.
data: variable to fill with register value.


Returns:
0= Failure
1= Success
***************************************************/

uint8_t DS1307Read(uint8_t address, uint8_t *data)
{
    uint8_t res;	// result

    // Start
    I2CStart();

    // SLA+W
    res = I2CWriteByte(0b11010000);	//DS1307 address + W (to set up register pointer)

    // Error
    if(!res)	return FALSE;

    // Now send the address of required register
    res = I2CWriteByte(address);

    // Error
    if(!res)	return FALSE;

    // Repeat Start
    I2CStart();

    // SLA + R
    res=I2CWriteByte(0b11010001);	//DS1307 Address + R

    // Error
    if(!res)	return FALSE;

    // Now read the value with NACK
    res = I2CReadByte(data, 0);

    // Error
    if(!res)	return FALSE;

    // STOP
    I2CStop();

    return TRUE;
}

/***************************************************

Function To Write Internal Registers of DS1307
---------------------------------------------

address : address of the register.
data: value to write to register.


Returns:
0= Failure
1= Success
***************************************************/

uint8_t DS1307Write(uint8_t address, uint8_t data)
{
    uint8_t res;	// result

    //Start
    I2CStart();

    //SLA+W
    res = I2CWriteByte(0b11010000);	//DS1307 address + W

    // Error
    if(!res)	return FALSE;

    // Now send the address of required register
    res = I2CWriteByte(address);

    // Error
    if(!res)	return FALSE;

    // Now write the value
    res = I2CWriteByte(data);

    // Error
    if(!res)	return FALSE;

    // STOP
    I2CStop();

    return TRUE;
}



