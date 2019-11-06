

/*
'C' language code for an electronic ignition for a typical 4-stroke motorcycle or similar engine, 
written for the TM4C123G evaluation board.  Engine crankshaft has a magnet embedded in the 
flywheel, and a fixed two pole magnetic pickup with poles pickup_degrees apart combined with a 1/100 ms
timer is used to detect the speed of the engine to calculate the correct advance and the time to
fire the ignition.  Advance ranges from min_adv to max_adv degrees linerally, beginning at 1000 RPM 
and ending at 5000 RPM. TDC for the engine is 180 degrees from the second pickup pole.  Ignition 
coil firing circuit is connected to PF1, and pickup is connected to PF0.  For production code, values
should be suitably adjusted for the engine and a suitable watchdog timer should be added.  
*/

#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

// Initialize GPIO Port F for negative logic switches on PF0 and
// PF4 as the Launchpad is wired.  Weak internal pull-up
// resistors are enabled, and the NMI functionality on PF0 is disabled.
#define GPIO_PORTF_DATA_R (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R (*((volatile unsigned long *)0x400FE108))

//initial timer value, counts down to 0
const timer_load_value = 0x639C;
//distance between the two pickup poles in degrees
const pickup_degrees = 10;
//maximum advance in degrees to use
const max_adv = 20;
//minimum advance in degrees to use
const min_adv = 0;
//current value of counter to use in current cycle calculations
unsigned long current_counter_value = 0;
//timer_load_value minus current_counter_value (elapsed 1/100 ms)
unsigned long timer_difference = 0;
//rpm in current cycle
unsigned long current_rpm = 0;
//time required to rotate 1 degree in current cycle
unsigned long degree_time = 0;
// time to get to TDC this cycle (is 180 degrees from second pickup pole)
unsigned long tdc_time = 0;
//ignition advance to apply this cycle
unsigned long advance = 0;

unsigned long i;

main()
{

  //init board i/o etc
  initialize_board();
  //set up 1/100 ms timer and init to 0
  set_ms_counter(0);

  while (1)
  {
    // poll for pulses from sensor
    current_counter_value = look_for_pulse();

    //have valid first and second pulse here

    //we are going to fire coil so begin charging it
    //turn on ignition coil here to charge it
    GPIO_PORTF_DATA_R = 0x02;

    //we have valid time in 1/100 ms counter here
    //get time in 1/100 ms to rotate one degree here
    degree_time = current_counter_value / pickup_degrees;
    //get current RPM here
    current_rpm = ((degree_time * 360) / 1000) / 60;

    //determine advance to use from RPM
      if (current_rpm < 1000)
      advance = min_adv;
      else if(current_rpm > 5000
      advance = max_adv;
      else{
      //determine advance to use, using RPM
      //advance is linear from min to max
      i = 5000 / current_rpm;
      advance = max_adv / i;
      }

    // now determine time in 1/100 ms to get to TDC this cycle
    tdc_time = 180 * degree_time;

    //subtract advance to get coil firing time in 1/100 ms from crossing of second pickup pole
    i = tdc_time - advance * degree_time;

    //need to wait for i 1/100 ms to fire
    //load 1/100 ms timer here
    set_ms_counter(i);
 
    //wait i 1/100 ms (wait till counter == 0)
      while (get_ms_counter());

    //turn off coil to fire it
    GPIO_PORTF_DATA_R = 0x00;

    // continue while() loop for next engine cycle
  }
}

int look_for_pulse(void)
{
  while (1)
  {
    // until see pulses
    set_ms_timer(timer_load_value);

    //wait here for sensor line to go low
    while (GPIO_PORTF_DATA_R & 0x01); // read PF0

    //stay till is high again
    while (!GPIO_PORTF_DATA_R & 0x01); // read PF0

    //if counter == 0 enough time elapsed between
    //setting of timer and pulse to indicate it is
    //first pulse
      if (get_ms_counter == 0)
      //if counter was 0, got first pulse
      continue;

    //else got second pulse and timer value is valid
    return (get_ms_counter());
  }

  void initialize_board(void)
  {
    volatile unsigned long delay;
    SYSCTL_RCGC2_R |= 0x00000020;   // 1) F clock
    delay = SYSCTL_RCGC2_R;         // delay
    GPIO_PORTF_LOCK_R = 0x4C4F434B; // 2) unlock PortF PF0
    GPIO_PORTF_CR_R = 0x1F;         // allow changes to PF4-0
    GPIO_PORTF_AMSEL_R = 0x00;      // 3) disable analog function
    GPIO_PORTF_PCTL_R = 0x00000000; // 4) GPIO clear bit PCTL
    GPIO_PORTF_DIR_R = 0x0E;        // 5) PF4,PF0 input, PF3,PF2,PF1 output
    GPIO_PORTF_AFSEL_R = 0x00;      // 6) no alternate function
    GPIO_PORTF_PUR_R = 0x11;        // enable pullup resistors on PF4,PF0
    GPIO_PORTF_DEN_R = 0x1F;        // 7) enable digital pins PF4-PF0

    //make sure coil is turned off
    GPIO_PORTF_DATA_R = 0x00;
  }

  /*******************************************************************************************/
  /*******************************************************************************************/
  /*******************************************************************************************/
  //a down counter of 1/100 ms clicks driven by hardware 1/100 ms timer

  int get_ms_counter(void)
  {
    return ms_timer_val;
  }

  void set_ms_counter(int val)
  {
    //disable Timer
    TIMER0_CTL_R = 0;
    //initialize timer
    init_timer();

    //mstimerval will hit 0 after val 1/100 ms
    ms_timer_val = val;

    //start the timer
    TIMER0_CTL_R |= 0x01;
  }

  //interrupt handler for hardware timer
  void Timer0A_Handler(void)
  {
    TIMER0_ICR_R = TIMER_ICR_TATOCINT; // acknowledge timer0A timeout

    if (ms_timer_val > 0)
      ms_timer_val--;
  }

  //init 1/100 ms hardware timer
  void init_timer(void)
  {
    //enable clock to Timer Block 0
    SYSCTL_RCGCTIMER_R |= 1;
    //disable Timer before initialization
    TIMER0_CTL_R = 0;
    //16-bit option
    TIMER0_CFG_R = 0x04;
    //one-shot mode and down-counter
    TIMER0_TAMR_R = 0x01;
    //Timer A interval load value register for 1/100 ms
    TIMER0_TAILR_R = 160;
    //clear the TimerA timeout flag
    TIMER0_ICR_R = 0x01;

    TIMER0_IMR_R |= 0x00000001;                            //  arm timeout interrupt
    NVIC_PRI4_R = (NVIC_PRI4_R & 0x00FFFFFF) | 0x40000000; //priority 2
    NVIC_EN0_R |= NVIC_EN0_INT19;                          //enable interrupt 19 in NVIC
  }
  /*******************************************************************************************/
  /*******************************************************************************************/
  /*******************************************************************************************/
