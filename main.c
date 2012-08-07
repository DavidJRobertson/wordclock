#define __HAS_DELAY_CYCLES 0

#define HOURFLASH_ANIMATION_VALUE 600
#define HOURFLASH_BLANK_VALUE 600
#define HOURFLASH_COUNT 5

#define HOURFLASH_TOP_VALUE (HOURFLASH_BLANK_VALUE + HOURFLASH_ANIMATION_VALUE)


#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <string.h>
#include "I2C.h"
#include "ds1307.h"
#include "compiletime.h"

void shiftRegisterSendByte(uint8_t data);
void shiftRegisterStrobe(void);
void displayRow(uint16_t row);
static uint8_t reverse(uint8_t b);
void prepareScreen(uint8_t hour, uint8_t minute);
void getTimeFromRTC(void);
void setRTC(uint8_t hour, uint8_t minute);
uint8_t decodeBCD(uint8_t val);
void flash(uint16_t stage);

uint8_t currentrow = 0;
uint8_t currenthour = 0;
uint8_t currentminute = 0;

volatile uint16_t hourflash = 0;
volatile uint8_t  hourflashcount = 0;

uint16_t screen[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int main(void)
{
    sei(); // Enable interrupts

    DDRC   = 0b1111111;         // Port C all outputs
    DDRB   = 0b0000111;         // Pins B0 and B1 outputs for column I + J drive, plus B2 for 4017 POR
    DDRD  &= ~(_BV(PD4));      // Set PD4/T0 as input. Used for 1Hz input from RTC to timer/counter0.
    DDRD  &= ~(_BV(PD2));      // Set INT0 as input
    DDRD  &= ~(_BV(PD3));      // Set INT1 as input
    PORTD |= _BV(PD4);        // Turn on internal pull-up resistor for PD4/T0


    EICRA = 0b00001010;       // Set INT0 and INT1 to trigger on falling edge.
    EIMSK = 0b00000011;       // Enable INT0 and INT1


    // Initialize RTC
    I2CInit();

    uint8_t tempsecs;
    DS1307Read(0x00, &tempsecs);
    tempsecs &= 0b01111111;
    DS1307Write(0x00, tempsecs); // Start the clock
    //DS1307Write(0x01, 0b00000000); // Set minutes to 0
    //DS1307Write(0x02, 0b00000000); // Set hours to 0 + set to 24 hr clock

    DS1307Read(0x08, &tempsecs);
    if (tempsecs != 42)
    {
        //RTC has not been set up, use compile time
        //These constants are defined by a script which generates compiletime.h upon building in Code::Blocks IDE.
        setRTC(COMPILE_HOUR, COMPILE_MIN);
        TCNT0 = COMPILE_SEC;
        DS1307Write(0x08, 42); // Mark RTC as set up
    }
    getTimeFromRTC();

    DS1307Write(0x07, 0b00010000); // Turn on 1Hz output

    OCR0A = 59;               // Timer/counter0 compare register A to 59 for counting to a minute.

    TCCR0A &= ~(_BV(WGM00));  // Set timer/counter0 to CTC mode
    TCCR0A |= _BV(WGM01);
    TCCR0B &= ~(_BV(WGM02));

    TIMSK0 |= _BV(OCIE0A);    // Fire interrupt on  timer/counter0 compare match A.

    TCCR0B |= _BV(CS00);      // Set timer/counter0 clock source to PD4/T0, rising edge.
    TCCR0B |= _BV(CS01);      // For falling edge, change CS00 bit to 0.
    TCCR0B |= _BV(CS02);


    PORTC &= ~(_BV(PC1)); // Shift register data line low
    PORTB |= _BV(PB2);    // POR 4017
    _delay_us(1);
    PORTB &= ~(_BV(PB2));

    int i;
    for (i = 0; i < 9; i++)
    {
        PORTC |= _BV(PC3); // 4017 Clock line high
        _delay_us(1);
        PORTC &= ~(_BV(PC3)); //4017 Clock line low
    }


    prepareScreen(currenthour, currentminute);

    while(1)
    {
        if (hourflash == 1)
        {

              prepareScreen(currenthour, currentminute);
              hourflash--;

        }
        else if (hourflash > 1)
        {
            if (hourflashcount > 1 && hourflash == HOURFLASH_BLANK_VALUE) {
              hourflashcount--;
              hourflash = HOURFLASH_TOP_VALUE;
            }
            flash(hourflash);
            hourflash--;
        }

        displayRow(screen[currentrow]);
        _delay_ms(1);
    }
    return 0;
}


void displayRow(uint16_t row)
{
    shiftRegisterSendByte(row & 0xff);

    if ((row >> 8) & 0x01)
    {
        PORTB |= _BV(PB1);
    }
    else
    {
        PORTB &= ~(_BV(PB1));
    }

    if ((row >> 9) & 0x01)
    {
        PORTB |= _BV(PB0);
    }
    else
    {
        PORTB &= ~(_BV(PB0));
    }
    PORTC |= _BV(PC2); // Strobe shift register
    PORTC &= ~(_BV(PC2)); // as above

    PORTC |= _BV(PC3); // 4017 Clock line high
    PORTC &= ~(_BV(PC3)); //4017 Clock line low
    currentrow++;
    if (currentrow > 9)
    {
        currentrow = 0;
    }
}
void shiftRegisterSendByte(uint8_t data)
{
    data = reverse((uint8_t)data);
    int i;
    for(i = 0; i < 8; i++)
    {
        if (((data >> i) & 0x01) == 1)
        {
            PORTC |= _BV(PC1); // Data line high
        }
        else
        {
            PORTC &= ~(_BV(PC1)); // Data line low
        }

        PORTC |= _BV(PC0); // Clock line high
        PORTC &= ~(_BV(PC0)); // Clock line low
    }
}


static uint8_t reverse(uint8_t b)
{
    int rev = (b >> 4) | ((b & 0xf) << 4);
    rev = ((rev & 0xcc) >> 2) | ((rev & 0x33) << 2);
    rev = ((rev & 0xaa) >> 1) | ((rev & 0x55) << 1);
    return (uint8_t)rev;
}



void prepareScreen(uint8_t hour, uint8_t minute)
{
    screen[0] = 0b0000000000011011; // IT IS

    screen[9] = 0; //Blank o'clock line.
    if (minute < 35)
    {
        if (minute < 5)
        {
            screen[9] = 0b0000001111110000; // O'CLOCK
            screen[3] = 0b0000000000000000; // Blank to/past
        }
        else
        {
            screen[3] = 0b0000000111100000; // PAST
        }
    }
    else
    {
        screen[3] = 0b0000001100000000; // TO
        hour++; // Increment hour because we are saying it is x minutes to the next hour
        if (hour > 23)
        {
            hour = 0;
        }
    }

    screen[4]  = 0; // Blank hour words
    screen[5]  = 0;
    screen[6]  = 0;
    screen[7]  = 0;
    screen[8]  = 0;
    screen[9] &= 0b0000001111110000;

    switch (hour)
    {
    case 0:
    case 12:
        screen[7]  = 0b0000001111110000; // TWELVE
        break;
    case 1:
    case 13:
        screen[4]  = 0b0000000111000000; // ONE
        break;
    case 2:
    case 14:
        screen[5]  = 0b0000000000000111; // TWO
        break;
    case 3:
    case 15:
        screen[4]  = 0b0000000000111110; // THREE
        break;
    case 4:
    case 16:
        screen[8]  = 0b0000001111000000; // FOUR
        break;
    case 5:
    case 17:
        screen[5]  = 0b0000001111000000; // hFIVE
        break;
    case 6:
    case 18:
        screen[5]  = 0b0000000000111000; // SIX
        break;
    case 7:
    case 19:
        screen[6]  = 0b0000000000011111; // SEVEN
        break;
    case 8:
    case 20:
        screen[6]  = 0b0000001111100000; // EIGHT
        break;
    case 9:
    case 21:
        screen[7]  = 0b0000000000001111; // NINE
        break;
    case 10:
    case 22:
        screen[9] |= 0b0000000000000111; // hTEN
        break;
    case 11:
    case 23:
        screen[8]  = 0b0000000000111111; // ELEVEN
        break;
    }

    screen[1]  = 0; // Blank minute words
    screen[2]  = 0;
    screen[3] &= 0b0000001111100000;
    if (minute < 5)
    {
        // do nothing
    }
    else if (minute < 10)
    {
        screen[3] |= 0b0000000000001111; // mFIVE
    }
    else if (minute < 15)
    {
        screen[0] |= 0b0000001110000000; // mTEN
    }
    else if (minute < 20)
    {
        screen[2] = 0b0000001111111000; // QUARTER
    }
    else if (minute < 25)
    {
        screen[1] = 0b0000000000111111; // TWENTY
    }
    else if (minute < 30)
    {
        screen[1] = 0b0000000000111111; // TWENTY
        screen[3] |= 0b0000000000001111; // mFIVE
    }
    else if (minute < 35)
    {
        screen[1] = 0b0000001111000000; // HALF
    }
    else if (minute < 40)
    {
        screen[1] = 0b0000000000111111; // TWENTY
        screen[3] |= 0b0000000000001111; // mFIVE
    }
    else if (minute < 45)
    {
        screen[1] = 0b0000000000111111; // TWENTY
    }
    else if (minute < 50)
    {
        screen[2] = 0b0000001111111000; // QUARTER
    }
    else if (minute < 55)
    {
        screen[0] |= 0b0000001110000000; // mTEN
    }
    else if (minute < 60)
    {
        screen[3] |= 0b0000000000001111; // mFIVE
    }
}

void getTimeFromRTC()
{
    DS1307Read(0x01, &currentminute);
    currentminute = decodeBCD(currentminute);

    DS1307Read(0x02, &currenthour);
    currenthour   = decodeBCD(currenthour);

    uint8_t tempsecs;
    DS1307Read(0x00, &tempsecs);
    tempsecs &= 0b01111111; // Mask away clock halt bit
    tempsecs = decodeBCD(tempsecs);
    TCNT0 = tempsecs;
}


uint8_t decodeBCD(uint8_t val)
{
    // Decode binary coded decimal
    return ( (val/16*10) + (val%16) );
}


void setRTC(uint8_t hour, uint8_t minute)
{
    uint8_t unitmins  = minute % 10;
    uint8_t tenmins   = (minute - unitmins) / 10;

    uint8_t minsdata  = 0b00001111 & unitmins;
    uint8_t tempm      = tenmins << 4;
    minsdata = minsdata | tempm;

    DS1307Write(0x01, minsdata);


    uint8_t unithours = hour % 10;
    uint8_t tenhours  = (hour - unithours) / 10;


    uint8_t hoursdata  = 0b00001111 & unithours;
    uint8_t temph      = tenhours << 4;
    hoursdata = hoursdata | temph;
    hoursdata = hoursdata & 0b00111111;

    DS1307Write(0x02, hoursdata);


    DS1307Write(0x00, 0b00000000); // Zero seconds
}

void flash(uint16_t stage)
{
    screen[0] = 0;
    screen[1] = 0;
    screen[2] = 0;
    screen[3] = 0;
    screen[4] = 0;
    screen[5] = 0;
    screen[6] = 0;
    screen[7] = 0;
    screen[8] = 0;
    screen[9] = 0;
    if (stage < HOURFLASH_BLANK_VALUE)
    {
        return;
    }
    //Form is       (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    uint8_t level = (stage - HOURFLASH_BLANK_VALUE) * 5 / (HOURFLASH_TOP_VALUE - HOURFLASH_BLANK_VALUE);
    uint16_t temp = 0b0000000000000000;
    uint8_t fi;

    for (fi = level; fi < (10 - level); fi++)
    {
        temp |= _BV(fi);
        screen[fi] = 0b0000000000000000 | _BV(level) | _BV(9-level);
    }
    screen[level] = screen [9-level] = temp;

}

ISR(TIMER0_COMPA_vect)
{
    // Interrupt service routine for t/c0 compare match A. Fires every minute.

    currentminute++;
    if (currentminute > 59)
    {
        currentminute = 0;
        currenthour++;
        hourflashcount = HOURFLASH_COUNT;
        hourflash = HOURFLASH_TOP_VALUE;
        if (currenthour > 23)
        {
            currenthour = 0;
            getTimeFromRTC();
        }
    }

    prepareScreen(currenthour, currentminute);
}


ISR(INT0_vect)
{
    //hourflashcount = HOURFLASH_COUNT;
    //hourflash = HOURFLASH_TOP_VALUE;
    // Add a minute
    currentminute++;
    if (currentminute > 59)
    {
        currentminute = 0;
        currenthour++;
        if (currenthour > 23)
        {
            currenthour = 0;
        }
    }
    TCNT0 = 0; // Zero seconds
    setRTC(currenthour, currentminute);
    prepareScreen(currenthour, currentminute);
    _delay_ms(20);
}

ISR(INT1_vect)
{
    // Add an hour
    currenthour++;
    if (currenthour > 23)
    {
        currenthour = 0;
    }
    TCNT0 = 0; // Zero seconds
    setRTC(currenthour, currentminute);
    prepareScreen(currenthour, currentminute);
    _delay_ms(20);
}
