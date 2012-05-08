#define __HAS_DELAY_CYCLES 0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <stdbool.h>
#include <avr/interrupt.h>

void shiftRegisterSendBit(bool bit);
void shiftRegisterSendByte(uint8_t data);
void shiftRegisterStrobe(void);
void displayRow(uint16_t row);
void nextRow(void);
static uint8_t reverse(uint8_t b);
void prepareScreen(uint8_t hour, uint8_t minute);

uint8_t currentrow = 0;
uint8_t currenthour = 0;
uint8_t currentminute = 0;

uint16_t screen[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int main(void)
{
    sei(); // Enable interrupts
    //TODO: make program initialize RTC and get current time over I2C

    DDRC = 0b1111111;         // Port C all outputs
    DDRB = 0b0000111;         // Pins B0 and B1 outputs for column I + J drive, plus B2 for 4017 POR
    DDRD &= ~(_BV(PD4));      // Set PD4/T0 as input. Used for 1Hz input from RTC to timer/counter0.
    PORTD |= _BV(PD4);        // Turn on internal pull-up resistor for PD4/T0

    OCR0A = 60;               // Timer/counter0 compare register A to 60 for counting to a minute.

    TCCR0A &= ~(_BV(WGM00));  // Set timer/counter0 to CTC mode
    TCCR0A |= _BV(WGM01);
    TCCR0B &= ~(_BV(WGM02));

    TIMSK0 |= _BV(OCIE0A);    // Fire interrupt on  timer/counter0 compare match A.

    TCCR0B |= _BV(CS00);      // Set timer/counter0 clock source to PD4/T0, rising edge.
    TCCR0B |= _BV(CS01);      // For falling edge, change CS00 bit to 0.
    TCCR0B |= _BV(CS02);





    PORTC &= ~(_BV(PC0)); // Shift register data line low

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

    displayRow(0);
    prepareScreen(currenthour, currentminute);
    while(1)
    {
        displayRow(screen[currentrow]);
        nextRow();

        _delay_ms(1);
    }
    return 0;
}

void nextRow(void)
{
    PORTC |= _BV(PC3); // 4017 Clock line high
    _delay_us(1);
    PORTC &= ~(_BV(PC3)); //4017 Clock line low

    currentrow++;
    if (currentrow > 9)
    {
        currentrow = 0;
    }
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
    shiftRegisterStrobe();
}
void shiftRegisterSendByte(uint8_t data)
{
    data = reverse((uint8_t)data);
    int i;
    for(i = 0; i < 8; i++)
    {
        shiftRegisterSendBit((data >> i) & 0x01);
    }
}

void shiftRegisterSendBit(bool bit)
{
    if (bit == 1)
    {
        PORTC |= _BV(PC1); // Data line high
    }
    else
    {
        PORTC &= ~(_BV(PC1)); // Data line low
    }

    // _delay_us(1);
    PORTC |= _BV(PC0); // Clock line high
    //_delay_us(1);
    PORTC &= ~(_BV(PC0)); // Clock line low
    //_delay_us(1); // TODO: check if these delays are really necessary
}

void shiftRegisterStrobe()
{
    PORTC |= _BV(PC2);
    _delay_us(1);
    PORTC &= ~(_BV(PC2));
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
            screen[9] = 0b0000001111111000; // O'CLOCK
            screen[3] &= 0b1111111111000000; // Blank to/past
        }
        else
        {
            screen[3] = 0b0000000000111100; // PAST
        }
    }
    else
    {
        screen[3] = 0b0000000000000011; // TO
        hour++; // Increment hour because we are saying it is x minutes to the next hour
    }

    screen[4] = 0; // Blank hour words
    screen[5] = 0;
    screen[6] = 0;
    screen[7] = 0;
    screen[8] = 0;

    switch (hour)
    {
    case 0:
    case 12:
        screen[8] = 0b0000001111110000; // TWELVE
        break;
    case 1:
    case 13:
        screen[3] |= 0b0000001110000000; // ONE
        break;
    case 2:
    case 14:
        screen[5] = 0b0000000000000111; // TWO
        break;
    case 3:
    case 15:
        screen[6] = 0b0000001111100000; // THREE
        break;
    case 4:
    case 16:
        screen[7] = 0b0000001111000000; // FOUR
        break;
    case 5:
    case 17:
        screen[5] = 0b0000000001111000; // hFIVE
        break;
    case 6:
    case 18:
        screen[4] = 0b0000000111000000; // SIX
        break;
    case 7:
    case 19:
        screen[4] = 0b0000000000111110; // SEVEN
        break;
    case 8:
    case 20:
        screen[6] = 0b0000000000011111; // EIGHT
        break;
    case 9:
    case 21:
        screen[8] = 0b0000000000001111; // NINE
        break;
    case 10:
    case 22:
        screen[5] = 0b0000001110000000; // hTEN
        break;
    case 11:
    case 23:
        screen[7] = 0b0000000000111111; // ELEVEN
        break;
    }

    screen[1] = 0; // Blank minute words
    screen[2] = 0;

    if (minute < 5)
    {
        // do nothing
    }
    else if (minute < 10)
    {
        screen[1] = 0b0000001111000000; // mFIVE
    }
    else if (minute < 15)
    {
        screen[2] = 0b0000000000000111; // mTEN
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
        screen[1] = 0b0000001111111111; // TWENTYFIVE
    }
    else if (minute < 35)
    {
        screen[0] |= 0b0000001111000000; // HALF
    }
    else if (minute < 40)
    {
        screen[1] = 0b0000001111111111; // TWENTYFIVE
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
        screen[2] = 0b0000000000000111; // mTEN
    }
    else if (minute < 60)
    {
        screen[1] = 0b0000001111000000; // mFIVE
    }
}


ISR(TIMER0_COMPA_vect) {
  // Interrupt service routine for t/c0 compare match A. Fires every minute.
  currentminute++;
  if (currentminute > 59) {
    currentminute = 0;
    currenthour++;
    if (currenthour > 23){
      currenthour = 0;
      //TODO: make software get current time from RTC over I2C here. This happens daily at midnight.
    }
  }
  prepareScreen(currenthour, currentminute);
}

