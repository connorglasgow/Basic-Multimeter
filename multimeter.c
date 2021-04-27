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
#define AIN7 PORTD,0

//defines for commandline interface
#define MAX_CHARS 80
#define MAX_FIELDS 5

//debug
#define DEBUG

//----------------------------------------
// TEMP GLOBAL VARS
//----------------------------------------
//for time values for measuring components
uint32_t time = 0;
uint64_t average = 0;
//for commandline interface
extern bool enterPressed;
//for sprintf stuff
char outstr[200];

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
    selectPinAnalogInput(AIN7);

    //initialize comparator
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;
    _delay_cycles(3);

    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN | COMP_ACREFCTL_VREF_M; //reference voltage for comparator
    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF | COMP_ACCTL0_CINV
            | COMP_ACCTL0_ISEN_RISE; //set comparator to reference voltage, inverted, and on rising edge
    COMP_ACINTEN_R &= ~COMP_ACINTEN_IN0;                 //disable the interrupt

    //initalize the timer
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    //timer1 config (resistance implementation)
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;        //disable timer before configuration
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;                        //32 bit timer
    TIMER1_TAMR_R |= TIMER_TAMR_TACDIR | TIMER_TAMR_TAMR_PERIOD; //periodic mode timer count up
    TIMER1_CTL_R = 0;
    TIMER1_IMR_R = 0;
    TIMER1_TAV_R = 0;                                   //set initial value to 0
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                           //enable the timer
    NVIC_EN0_R &= ~(1 << (INT_TIMER1A - 16));             //disable interrupt 37
}

void measure_resistance()
{
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        //set inital pin values
        setPinValue(MEAS_LR, 1);
        setPinValue(MEAS_C, 0);
        setPinValue(HIGHSIDE_R, 0);
        setPinValue(INTEGRATE, 1);
        setPinValue(LOWSIDE_R, 1);
        //wait 50000 microseconds
        waitMicrosecond(50000);

        setPinValue(LOWSIDE_R, 0);
        //set the timer to 0
        TIMER1_TAV_R = 0;
        while (COMP_ACSTAT0_R == 0)
        {
        }
        time = TIMER1_TAV_R;
        if (i == 0)
        {
            average = time;
        }
        else
        {
            average = (time + average) / 2;
        }
    }

#ifdef DEBUG
    sprintf(outstr, "average resistance val is %li\n", average);
    putsUart0(outstr);
#endif
}

void measure_capacitance()
{
    uint8_t i;
    for (i = 0; i < 1; i++)
    {
        //set inital pin values
        setPinValue(MEAS_LR, 0);
        setPinValue(MEAS_C, 1);
        setPinValue(HIGHSIDE_R, 1);
        setPinValue(INTEGRATE, 0);
        setPinValue(LOWSIDE_R, 1);
        //wait 50000 microseconds
        waitMicrosecond(50000);

        setPinValue(LOWSIDE_R, 0);
        //set the timer to 0
        TIMER1_TAV_R = 0;
        while (COMP_ACSTAT0_R == 0)
        {
        }
        time = TIMER1_TAV_R;
        if (i == 0)
        {
            average = time;
        }
        else
        {
            average = (time + average) / 2;
        }
    }

#ifdef DEBUG
    sprintf(outstr, "average cap val is %li\n", average);
    putsUart0(outstr);
#endif
}

void measure_inductance()
{
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        setPinValue(MEAS_LR, 1);
        setPinValue(MEAS_C, 0);
        setPinValue(HIGHSIDE_R, 0);
        setPinValue(LOWSIDE_R, 0);
        setPinValue(INTEGRATE, 1);

        //turn on measure_lr and turn on lowside_r, test circuit the voltage is rising on 33ohm resistor
        waitMicrosecond(50000);
        setPinValue(LOWSIDE_R, 1);
        /*
         while (COMP_ACSTAT0_R == 1)
         {
         }
         */
        TIMER1_TAV_R = 0;
        while (COMP_ACSTAT0_R == 0)
        {
        }
        time = TIMER1_TAV_R;
        if (i == 0)
        {
            average = time;
        }
        else
        {
            average = (time + average) / 2;
        }
    }
#ifdef DEBUG
    sprintf(outstr, "average inductor val is %li\n", average);
    putsUart0(outstr);
#endif
}

void measure_voltage()
{

}

void measure_esr()
{
    setAdc0Ss3Mux(7);
    uint16_t value1 = readAdc0Ss3();

    float value2 = (3.3*value1)/4096.0;

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
    time = 0;
    float calibrated;

    //clears the 'buffer' that seems to show up at the start
    clear();

    putsUart0("\nMultimeter\n>");

    bool valid;
    while (true)
    {
        if (kbhitUart0())
        {
            //process command
            getsUart0(&data);
            if (enterPressed)
            {
                enterPressed = false;
                parseFields(&data);

                valid = false;

                if (isCommand(&data, "r", 0)
                        || isCommand(&data, "resistance", 0))
                {
                    //measure the resistance
                    measure_resistance();
                    //output the timer values for debugging at this time

                    if (average > 5000000)
                    {
                        calibrated = average * 98400 / 5751721;
                    }
                    else if (average < 15000)
                    {
                        calibrated = average * 217 / 10747;
                    }
                    else
                    {
                        calibrated = average * 5000 / 279801;
                    }
                    sprintf(outstr, "Resistance is %0.0f Ohms\n", calibrated);
                    putsUart0(outstr);

                    //set command to valid
                    valid = true;
                    clear();
                }

                if (isCommand(&data, "c", 0)
                        || isCommand(&data, "capacitance", 0))
                {
                    measure_capacitance();

                    if (average > 200000000)
                    {
                        calibrated = average * 47 / 268465353;
                    }
                    else if (average > 120000000)
                    {
                        calibrated = average * 22 / 129098857;
                    }
                    else
                    {
                        calibrated = average * 0.000000170884;
                    }
                    sprintf(outstr, "Capacitance is %0.1fuF\n", calibrated);
                    putsUart0(outstr);

                    //valid command
                    valid = true;
                    clear();
                }

                if (isCommand(&data, "l", 0)
                        || isCommand(&data, "inductance", 0))
                {
                    measure_inductance();

                    if (average > 250)
                    {
                        calibrated = average * 1000 / 1785;
                    }
                    else
                    {
                        calibrated = average * 100 / 129;
                    }
                    sprintf(outstr, "Inductance is %0.1fuF\n", calibrated);
                    putsUart0(outstr);

                    valid = true;
                    clear();
                }

                if (isCommand(&data, "voltage", 0))
                {
                    measure_voltage();

                    valid = true;
                    clear();
                }

                if (isCommand(&data, "esr", 0))
                {
                    measure_esr();

                    valid = true;
                    clear();
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
