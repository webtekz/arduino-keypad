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
#define POLLING_INTERVAL 10  // time delay between polling events
#define DEBOUNCE_ITER 5      // number of consecutive true samples for keypress to register
#define SAMPLE_TIME_US 1000  // time in microseconds to wait before sampling the rows

#define u8 uint8_t
#define u16 uint16_t

#define IN_PORT PORTL   // input port PORT register (sets pullup resistor)
#define IN_DDR DDRL     // input port DDR register (sets port as input/output)
#define IN_PIN PINL     // input port PIN register (contains state of pins)

#define OUT_PORT PORTD  // output port PORT register (sets output state)
#define OUT_DDR DDRD

static const u8 row_ports[4] = {PL7, PL5, PL3, PL1};
static const u8 col_ports[4] = {PD3, PD2, PD1, PD0};

static u8 deb_matrix[4][DEBOUNCE_ITER] = {0};  // place to record last DEBOUNCE_ITER reads of pins
static u8 deb_idx = 0;                         // column index for deb_matrix
static u8 prev_state[4] = {0};                 // previous debounced state for edge detection

static const u8 matrix_chars[4][4] = {
 {'1', '2', '3', 'A'},
 {'4', '5', '6', 'B'},
 {'7', '8', '9', 'C'},
 {'*', '0', '#', 'D'}
};

static u8 pass_buf[PASSWD_LENGTH] = {0};  // password buffer, statically allocated

/* internal function prototypes */
static void init_io(void);
static u8 kp_get_char(void);
static void kp_get_str(u8* str, u16 count);
static void scan_debounce(u8* out_matrix, u8* changed_matrix);


int main(void)
{
  init_io();

  DDRB |= _BV(DDB7);   // PB7 is wired to the Arduino LED on the mega2560. 
  PORTB &= ~_BV(PB7);  // turn off LED

  kp_get_str(pass_buf, PASSWD_LENGTH);

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


/* kp_get_char uses scan_debounce to get both the current state and detected */
/* edges for each button in the matrix. The function repeatedly polls the    */
/* keypad for a character until a keypress is detected.                      */
static u8 kp_get_char(void)
{
  u8 r, c;                                // row and column indices
  u8 state_matrix[4], changed_matrix[4];  // storage for current matrix state and edges
  u8 pin_state, pin_changed;              // temp vars for the current pin's state and edge

  while(true)
  {
    scan_debounce(state_matrix, changed_matrix);  // get debounced output
    _delay_ms(POLLING_INTERVAL);                  // delay for polling rate
    for(c = 0; c < 4; c++)
    {
      for(r = 0; r < 4; r++)
      {
        /* get state for each row pin -- changed is true when an edge is detected */
        pin_state = state_matrix[c] & _BV(row_ports[r]);
        pin_changed = changed_matrix[c] & _BV(row_ports[r]);

        /* When pin_state is true and pin_changed is true, the respective button  */
        /* is pressed and it's on a rising edge, an event that only happens once  */
        /* per keypress.                                                          */
        if(pin_state && pin_changed)
        {
          return matrix_chars[r][c];
        }
      }
    }
  }
}


/* kp_get_str uses kp_get_char to retrieve count characters from the keypad. */
/* kp_get_char is simply run count times to fill the location pointed to by  */
/* str. It is assumed writing count chars to str will not cause an error.    */
static void kp_get_str(u8* str, u16 count)
{
  u16 i;
  for(i = 0; i < count; i++)
  {
    str[i] = kp_get_char();
  }
}


/* scan_debounce reads the entire matrix by probing each column and sampling */
/* the rows. It does this by driving each column LOW and then recording      */
/* the rows which get pulled down by that column.                            */
/*                                                                           */
/* Debouncing is performed by keeping track of the last DEBOUNCE_ITER reads  */
/* for each pin, for each column, for a total of 4 x 4 x DEBOUNCE_ITER bits, */
/* stored in deb_matrix. The debounced output is the bitwise AND of all bits */
/* in the third dimension of that matrix, producing a 1 for each button only */
/* if all previous DEBOUNCE_ITER reads were 1.                               */
/* Edges are detected by keeping track of the debounced output (NOT          */
/* deb_matrix) previously output by scan_debounce, and using XOR to find     */
/* bits that changed between the previous iteration and the current.         */
/*                                                                           */
/* The ideas behind this algorithm were provided by Jack Ganssle of the      */
/* Ganssle Group, in an excellent article on hardware and software           */
/* debouncing.                                                               */
/* It can be found at:                                                       */
/* http://www.ganssle.com/debouncing.htm                                     */
static void scan_debounce(u8* out_matrix, u8* changed_matrix)
{
  u8 i, j;
  for(i = 0; i < 4; i++)
  {
    OUT_PORT &= ~_BV(col_ports[i]);       // set column output to LOW for probing
    _delay_us(SAMPLE_TIME_US);            // wait for signals to propagate
    deb_matrix[i][deb_idx] = ~IN_PIN;     // bitwise NOT is b/c button pressed when pin LOW
    OUT_PORT |= _BV(col_ports[i]);        // restore column to HIGH

    out_matrix[i] = 0xFF;                 // init output, will be AND of the row's last iterations
    for(j = 0; j < DEBOUNCE_ITER; j++)
    {
      out_matrix[i] &= deb_matrix[i][j];  // bitwise AND the past DEBOUNCE_ITER samples
    }

    changed_matrix[i] = out_matrix[i] ^ prev_state[i];  // compare current state with previous
    prev_state[i] = out_matrix[i];
  }

  deb_idx++;
  if(deb_idx >= DEBOUNCE_ITER)  // if deb_idx overflows, reset to 0
    deb_idx = 0;
}