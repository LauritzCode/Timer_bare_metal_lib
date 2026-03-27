#include "support/timer_helper.h"
#include <avr/interrupt.h>

volatile uint32_t tick_count = 0;

int main(void) {

  // set LED as output
  DDRB |= (1 << PB7);

  initialize_timer(Timer1, 1);

 
  while(1) {
    // keep it
  }
}



ISR(TIMER1_COMPA_vect) {
  tick_count++;
  PORTB ^= (1 << PB7);

  if(tick_count >= 10) {
    stop_timer(Timer1);
    tick_count = 0;
  }
}