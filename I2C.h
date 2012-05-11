#ifndef I2C_H
#define I2C_H

#define TRUE 1
#define FALSE 0

void I2CInit();
void I2CClose();

void I2CStart();
void I2CStop();

uint8_t I2CWriteByte(uint8_t data);
uint8_t I2CReadByte(uint8_t *data,uint8_t ack);	

#endif



