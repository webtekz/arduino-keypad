/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* ---KEYPAD---                                                                                  */
/* A quick program for getting input from a 4x4 matrix keypad on an Arduino.                     */
/* Mostly just learning how to program for AVR in C. Platform is assumed to be Arduino Mega2560, */
/* although it can be made to work with a host of other AVR platforms by varying the #defines at */
/* the top.                                                                                      */
/*                                                                                               */
/* Currently WIP until I can get an LCD display.                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

#define PASSWD_LENGTH 4
#define DEBOUNCE_TIME 100

#define u8 uint8_t

#define IN_PORT PORTL   // input port PORT register (sets pullup resistor)
#define IN_DDR DDRL     // input port DDR register (sets port as input/output)
#define IN_PIN PINL     // input port PIN register (contains state of pins)

#define OUT_PORT PORTD  // output port PORT register (sets output state)
#define OUT_DDR DDRD

static const u8 row_ports[4] = {PL7, PL5, PL3, PL1};
static const u8 col_ports[4] = {PD3, PD2, PD1, PD0};

static const u8 matrix_chars[4][4] = {
 {'1', '2', '3', 'A'},
 {'4', '5', '6', 'B'},
 {'7', '8', '9', 'C'},
 {'*', '0', '#', 'D'}
};

/* matrix_ statetracks when a button is pressed or not pressed. */
/* Statically allocated to ensure that held-down buttons are    */
/* tracked over multiple calls of get_pass().                   */
static bool matrix_state[4][4] = {0};

static u8 pass_buf[PASSWD_LENGTH] = {0};  // password buffer, statically allocated

/* internal function prototypes */
static void init_io(void);
static void get_pass(void);


int main(void)
{
  init_io();

  DDRB |= _BV(DDB7);   // PB7 is wired to the Arduino LED on the mega2560. 
  PORTB &= ~_BV(PB7);  // turn off LED

  get_pass();

  /* When 4 chars are entered, flash the LED. */
  /* This is a placeholder for until I get a  */
  /* good LCD display to use.                 */
  while(true)  
  {
    PORTB ^= _BV(PB7);
    _delay_ms(1000);
  }
}


/* init_io initialzes the ports to their correct states for the application. */
/* Columns are set to outputs and normally output HIGH. Rows are set to      */
/* inputs and have their pullup resistors on, making them normally HIGH.     */
static void init_io(void)
{
  u8 i;
  for(i = 0; i < 4; i++)
  {
    IN_DDR  &= ~_BV(row_ports[i]);  // set rows as inputs
    IN_PORT |= _BV(row_ports[i]);   // activate internal pullup resistor for each row
  }

  for(i = 0; i < 4; i++)
  {
    OUT_DDR  |= _BV(col_ports[i]);  // set columns as outputs
    OUT_PORT |= _BV(col_ports[i]);  // set each column to output HIGH
  }
}


/* get_pass probes each row for each column and determines what key is is    */
/* pressed in the matrix. Each column is pulled LOW, which will pull a       */
/* respective row LOW if that row and column are connected by a pressed      */
/* keypad switch. Swtiches are debounced on both press and release for       */
/* DEBOUNCE_TIME ms. PASSWD_LENGTH characters are accepted and then the      */
/* function returns. The password is then stored in pass_buf.                */
static void get_pass(void)
{
  u8 r, c;          // row and column indices
  u8 state;         // tempvar for storing logical pin state of each row
  u8 pass_idx = 0;  // current index in pass_buf

  while(pass_idx < PASSWD_LENGTH)
  {
    for(c = 0; c < 4; c++)
    {
      OUT_PORT &= ~_BV(col_ports[c]);  // set column output to LOW for probing
      for(r = 0; r < 4; r++)
      {
        /* get state for each row pin -- button is pressed if pin is pulled low */
        state = !(IN_PIN & _BV(row_ports[r]));
        if(state && !matrix_state[r][c])  // if button is pressed and it has not been processed
        {
          _delay_ms(DEBOUNCE_TIME);                 // let switch stablize
          matrix_state[r][c] = true;                // button has now been processed
          pass_buf[pass_idx] = matrix_chars[r][c];  // add char to the password
          pass_idx++;
        }
        else if (!state && matrix_state[r][c])
        {
          _delay_ms(DEBOUNCE_TIME);    // let switch stablize
          matrix_state[r][c] = false;  // switch is no longer pressed and is ready for new cycle
        }
      }
      OUT_PORT |= _BV(col_ports[c]);   // restore column to HIGH
    }
  }
}