# timer_helper

A bare-metal CTC timer library for the ATmega2560. No Arduino framework, no `millis()`, no `delay()` instead this is just a direct register manipulation and interrupt-driven timing.

---

## What it is

`timer_helper` wraps the ATmega2560's 16-bit hardware timers (Timer1, Timer3, Timer4, Timer5) into a simple interface. You give it a frequency, it figures out the prescaler and compare value, configures the timer in CTC mode, and gets out of your way.

Timer0 and Timer2 are intentionally excluded — the Arduino framework uses them for `millis()` and `tone()` respectively.

---

## What I learned by making this library

- How hardware timers work at the register level (TCCR, TCNT, OCR, TIMSK)
- CTC (Clear Timer on Compare) mode vs Normal mode
- How prescalers divide the MCU clock to achieve long periods
- The difference between busy-wait delays and interrupt-driven timing
- Why `volatile` matters when sharing variables between `main()` and ISRs

---

## Files

```
timer_helper.h    — types, macros, and function declarations
timer_helper.cpp  — implementation
```

---

## API

```cpp
// Initialize a timer at a given frequency (Hz) in CTC mode
// Returns without doing anything if the frequency is not achievable
void initialize_timer(TimerID timer, uint32_t freq);

// Re-enable the compare match interrupt (after a stop)
void start_timer(TimerID timer);

// Disable the compare match interrupt without losing configuration
void stop_timer(TimerID timer);

// Reset the counter register (TCNTn) to 0
void reset_timer(TimerID timer);
```

### Available timers

```cpp
typedef enum {
    Timer1,
    Timer3,
    Timer4,
    Timer5
} TimerID;
```

---

## Example 1 — Repeating interrupt (LED blink at 1Hz)

Set the timer to 2Hz. The ISR toggles the LED on every interrupt, so the LED completes a full on/off cycle every 2 interrupts = 1Hz visible blink.

```cpp
#include "timer_helper.h"
#include <avr/interrupt.h>

int main(void) {
    DDRB |= (1 << PB7);            // LED as output

    initialize_timer(Timer1, 2);   // 2Hz → 1Hz visible blink

    while (1) {
        // main loop is free — all timing handled by ISR
    }
}

ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << PB7);           // toggle LED
}
```

---

## Example 2 — One-shot countdown ("do X after N seconds")

The timer still fires repeatedly, but a counter inside the ISR tracks elapsed ticks. Once the target is reached, the action runs and the timer stops itself.

```cpp
#include "timer_helper.h"
#include <avr/interrupt.h>

#define FREQ        10              // 10Hz tick rate
#define WAIT_SEC    5               // trigger after 5 seconds
#define TARGET      (FREQ * WAIT_SEC)

volatile uint32_t tick_count = 0;

int main(void) {
    DDRB |= (1 << PB7);

    initialize_timer(Timer1, FREQ);

    while (1) {
        // do other work here while timer counts in background
    }
}

ISR(TIMER1_COMPA_vect) {
    tick_count++;

    if (tick_count >= TARGET) {
        PORTB |= (1 << PB7);        // turn LED on after 5 seconds
        stop_timer(Timer1);         // one-shot — stop after firing
        tick_count = 0;
    }
}
```

## Two timers running simultaneously

Each timer is fully independent. They can run at different frequencies and have their own ISRs without interfering.

```cpp
initialize_timer(Timer1, 2);     // 2Hz — LED blink
initialize_timer(Timer3, 1000);  // 1kHz — some fast task

ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << PB7);
}

ISR(TIMER3_COMPA_vect) {
    // fast 1kHz task
}
```

---

## Prescaler math

The library automatically selects the smallest prescaler that gives a valid 16-bit OCR value using:

$$OCR = \frac{F_{CPU}}{N \cdot f_{target}} - 1$$

Valid prescaler values: `1, 8, 64, 256, 1024`  
Valid OCR range: `1 to 65535` (16-bit)

If no prescaler produces a valid integer OCR, `initialize_timer` returns silently without configuring anything.
