#include <avr/io.h>
#include <util/delay.h>

#include "I2C.h"


void I2CInit()
{
    //Set up TWI module for approx 100kHz clock
    TWBR = 40;
    TWSR &= ~(_BV(TWPS1)|_BV(TWPS0));

    //Enable the TWI module
    TWCR|=(1<<TWEN);


}

void I2CClose()
{
    // Disable TWI module
    TWCR &= ~(_BV(TWEN));
}


void I2CStart()
{
    // Send start condition
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA);

    // Wait for start to complete
    while(!(TWCR & _BV(TWINT)));
}

void I2CStop()
{
    // Send stop condition
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

    // Wait for stop to complete
    while(TWCR & _BV(TWSTO));
}

uint8_t I2CWriteByte(uint8_t data)
{
    TWDR = data;

    // Initiate transfer
    TWCR = _BV(TWEN) | _BV(TWINT);

    // Wait for completion
    while(!(TWCR & _BV(TWINT)));

    // Check Status
    if((TWSR & 0xF8) == 0x18 || (TWSR & 0xF8) == 0x28 || (TWSR & 0xF8) == 0x40)
    {
        // SLA+W Transmitted and ACK received
        // or
        // SLA+R Transmitted and ACK received
        // or
        // DATA Transmitted and ACK recived

        return TRUE;
    }
    else
        return FALSE;	// Error
}

uint8_t I2CReadByte(uint8_t *data, uint8_t ack)
{
    // Set up ack
    if(ack)
    {
        // Return ack after reception
        TWCR |= _BV(TWEA);
    }
    else
    {
        // Return NACK after reception.
        // Signals slave to stop sending more data.
        // Usually used for last byte read.
        TWCR &= ~(_BV(TWEA));
    }

    // Now enable Reception of data by clearing TWINT
    TWCR |= _BV(TWINT);

    // Wait for completion
    while(!(TWCR & _BV(TWINT)));

    // Check status
    if((TWSR & 0xF8) == 0x58 || (TWSR & 0xF8) == 0x50)
    {
        // Data received and ACK returned
        // or
        // Data received and NACK returned

        // Read the data
        *data = TWDR;
        return TRUE;
    }
    else
        return FALSE;	// Error

}



