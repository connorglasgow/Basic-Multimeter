//Connor Glasgow
//1001322122

//----------------------------------------
//  Includes, etc.
//----------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "adc0.h"
#include "wait.h"
#include "gpio.h"
#include "commandline.h"

//define pinouts
#define COMPARATOR PORTC,7
#define MEAS_LR PORTA,7
#define MEAS_C  PORTA,6
#define HIGHSIDE_R PORTA,5
#define LOWSIDE_R PORTA,2
#define INTEGRATE PORTA,3

//analog input for ADC stuff
#define AIN1 PORTE,2

//defines for commandline interface
#define MAX_CHARS 80
#define MAX_FIELDS 5

//'state machine' for interfacing with which measurement is taken
//also possibly for AUTO command
typedef enum _STATE
{
    resistance, capacitance, inductance, none
} STATE;

//----------------------------------------
// TEMP GLOBAL VARS
//----------------------------------------
//for time values for measuring components
uint32_t time = 0;
//for commandline interface
extern bool enterPressed;
STATE state;
//for sprintf stuff
char outstr[100];

//----------------------------------------
//  Functions
//----------------------------------------

void initHW()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    //init ADC
    initAdc0Ss3();

    // Enable clocks
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);

    // Configure pins
    selectPinPushPullOutput(MEAS_LR);
    selectPinPushPullOutput(MEAS_C);
    selectPinPushPullOutput(HIGHSIDE_R);
    selectPinPushPullOutput(LOWSIDE_R);
    selectPinPushPullOutput(INTEGRATE);
    selectPinPushPullOutput(INTEGRATE);
    selectPinAnalogInput(COMPARATOR);
    selectPinAnalogInput(AIN1);

    //initialize comparator
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;
    _delay_cycles(3);

    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN | COMP_ACREFCTL_VREF_M;     //reference voltage for comparator
    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF | COMP_ACCTL0_CINV
            | COMP_ACCTL0_ISEN_RISE;                                //set comparator to reference voltage, inverted, and on rising edge
    COMP_ACINTEN_R &= ~COMP_ACINTEN_IN0;                            //disable the interrupt
    NVIC_EN0_R |= 1 << (INT_COMP0 - 16);                            //interrupt for this implementation

    //initalize the timer
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    //timer1 config (resistance implementation)
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                                //disable timer before configuration
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;                          //32 bit timer
    TIMER1_TAMR_R |= TIMER_TAMR_TACDIR | TIMER_TAMR_TAMR_1_SHOT;    //periodic mode timer count up
    TIMER1_CTL_R = 0;
    TIMER1_IMR_R = 0;
    TIMER1_TAV_R = 0;                                               //set initial value to 0
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                                 //enable the timer
    NVIC_EN0_R &= ~(1 << (INT_TIMER1A - 16));                       //disable interrupt 37
}

void compInterrupt()
{
    //set time equal to timer value
    time = TIMER1_TAV_R;
    //disable the timer
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    //clear interrupt fag
    COMP_ACMIS_R |= COMP_ACMIS_IN0;

    switch (state)
    {
    //to be added properl, might not even be needed
    case resistance:
        break;
    }

}
void measure_resistance()
{
    //set inital pin values
    setPinValue(MEAS_LR, 1);
    setPinValue(MEAS_C, 0);
    setPinValue(HIGHSIDE_R, 0);
    setPinValue(INTEGRATE, 1);
    setPinValue(LOWSIDE_R, 1);
    //wait 50000 microseconds
    waitMicrosecond(50000);
    COMP_ACINTEN_R |= COMP_ACINTEN_IN0;
    //set LOWSIDE_R to low to discharge cap
    setPinValue(LOWSIDE_R, 0);
    //set the timer to 0
    TIMER1_TAV_R = 0;
    //start the timer, interrupt will trigger when comparator value is reached
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

double timertest()
{
    //testing stuff
    //need to remove
    TIMER1_TAV_R = 0;
    waitMicrosecond(500000);
    time = TIMER1_TAV_R;
    double result = time;
    return result;
}

//this will set all pins to off
void clear()
{
    setPinValue(MEAS_LR, 0);
    setPinValue(MEAS_C, 0);
    setPinValue(HIGHSIDE_R, 0);
    setPinValue(LOWSIDE_R, 0);
    setPinValue(INTEGRATE, 0);
}

int main(void)
{
    USER_DATA data;
    initHW();
    initUart0();
    initAdc0Ss3();
    setUart0BaudRate(115200, 40e6);
    //initialize the state to an idle state
    STATE state = none;
    time = 0;

    //clears the 'buffer' that seems to show up at the start
    clear();
    measure_resistance();
    //still not sure this does anything

    putsUart0("\n>");

    bool valid;
    while (true)
    {
        if (kbhitUart0())
        {
            //process command
            getsUart0(&data);
            if (enterPressed)
            {
                //putsUart0("Enter Command : ");
                enterPressed = false;
                //putsUart0("Command Entered: ");
                //putsUart0(data.buffer);
                //putsUart0("\n\r");
                parseFields(&data);

                valid = false;

                if (isCommand(&data, "r", 0) || isCommand(&data, "resistance", 0))
                {
                    //set the state to resistance
                    state = resistance;
                    //measure the resistance
                    measure_resistance();
                    //output the timer values for debugging at this time
                    sprintf(outstr, "timer val is %u\n", time);
                    putsUart0(outstr);

                    //set the state to none
                    state = none;
                    //set command to valid
                    valid = true;
                }

                //for easy testing purposes
                else if (isCommand(&data, "test", 0))
                {
                    valid = true;
                }

                //classic help command, will give the user instructions
                //on how to use the utility
                else if (isCommand(&data, "help", 0))
                {

                    valid = true;
                }

                //will let the user know if the command they entered didnt exist
                if (!valid)
                {
                    putsUart0("Invalid command\n\r");
                }
                putsUart0(">");
            }
        }
    }
}
