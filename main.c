/* ************************************** *
 * Project: Mechatronics 2019 Demo        *
 * File: main.c                           *
 *                                        *
 * Author(s): Jordan Whiting              *
 * ************************************** */

#include "el/stm32f10x_conf.h"
#include "el/ports.h"

#include "el/mini/display.h"
#include "el/adc/adc_inst.h"
#include "el/sleep.h"

#include <stdio.h>

#define ADC   el_adc1    //ADC 1
#define CHAN        2    //Channel 2
#define VMAX     5.0f

int cycles = 6;

void color_sensor_init()
{
    ADC.channel = CHAN;      // Color Sensor (Analog)
    el_adc_init(&ADC);
}

int color_sensor_read(bool* error)
{
    if ( EL_ADC_STATUS_SUCCESS == el_adc_selectChannel( &ADC, CHAN ) )
    {
        if( EL_ADC_STATUS_SUCCESS == el_adc_conv( &ADC ) ) // Start conversion
        {
            short raw = 0;
            float analog = 0;
            el_adc_rawValue( &ADC, &raw ); // Get raw value
            el_adc_volt_ref( &ADC, &analog, VMAX ); // Calculate voltage
            *error = false;
            return ( analog * 1024 ); // Calculate color value.
        }
    }
    *error = true;
    return 0;
}
    
/* sleepUpdate() : Idle update OLED display for Inputs and Outputs.
 *
 *      Format will look like the following on the OLED:
 *               _____________
 *              |  Demo 2019  |
 *              |   ooo ooo   |     (Inputs)
 *              |   ooo ooo   |     (Outputs)
 *              | Color       |     (Color Value)
 *              |             |     (Debug Info)
 *               ¯¯¯¯¯¯¯¯¯¯¯¯¯                                      */
void sleepUpdate(int ticks)
{
    bool error;
    while (ticks > 0) {
        el_display_drawCircle(12, 12, 2, true, EL_PORT_GET_PIN(A, 0));
        el_display_drawCircle(20, 12, 2, true, EL_PORT_GET_PIN(A, 1));
        el_display_drawCircle(28, 12, 2, true, EL_PORT_GET_PIN(A, 2));
        el_display_drawCircle(36, 12, 2, true, EL_PORT_GET_PIN(A, 3));
        el_display_drawCircle(44, 12, 2, true, EL_PORT_GET_PIN(B, 0));
        el_display_drawCircle(52, 12, 2, true, EL_PORT_GET_PIN(B, 1));
        el_display_drawCircle(12, 20, 2, true, EL_PORT_GET_PIN(B, 3));
        el_display_drawCircle(20, 20, 2, true, EL_PORT_GET_PIN(B, 4));
        el_display_drawCircle(28, 20, 2, true, EL_PORT_GET_PIN(B, 5));
        el_display_drawCircle(36, 20, 2, true, EL_PORT_GET_PIN(B, 6));
        el_display_drawCircle(44, 20, 2, true, EL_PORT_GET_PIN(B, 8));
        el_display_drawCircle(52, 20, 2, true, EL_PORT_GET_PIN(B, 9));
        el_display_writeMem();
        el_usleep(1);
        --ticks;
    }
}

/* main() : Initialize program, then run Cycle for each piece in stack magazine.
 *
 * Cycle:
 *  [1] Eject Cylinder from the Stack Magazine
 *  [2] Wait for Optical Sensor to detect Cylinder
 *  [3] Start Conveyor
 *  [4] Detect Cylinder color
 *  [5] If color is...
 *      [SILVER] Punch Cylinder &
 *      [PURPLE] Lower Diverter
 *      [BLUE]   Discard Part
 *  [6] Stop Conveyor movement once Cylinder has been sorted
 *
 *  Diagram of assembly line setup is as follows:

                            Color Sensor         /¯¯¯¯¯¯¯¯¯¯\
                                  |   Punch     |  Diverter  |
                          ]¯[     v     v       [____________]
                          |¯|    ]¯[   |¯|         \¯¯¯¯¯¯¯¯|
Piston   .____________.[I]| |====|R|===| |==========\       |================[I]
  v   [} | /¯¯¯¯¯¯¯¯\ | | | |    |G|   | |            ¯\    |     /¯¯¯¯¯¯¯¯\  |
______[} ||          || | | |    |B|  |¯¯¯|             \   |    |          | |
 <--> [} || Cylinder ||-> | | -> (_)  [ X ] ---\-->      )  | -> | Cylinder | |
¯¯¯¯¯¯[} ||          || | | |         |___|    |        /   |    |          | |
      [} | \________/ | | | |                  v       /    |     \________/  |
         '¯¯¯¯¯¯¯¯¯¯¯¯'[I]| |===========             =/_____|================[I]
                ^         |_|           \  /¯¯¯¯¯¯¯¯\ \      To Discard Storage
         Stack Magazine   ]_[            \|          ||
                           ^             || Cylinder ||
                     Optical Sensor      ||          ||
                                         | \________/ |
                                         |            |
                                         |     ||     |
                                         \     \/     /
                                       To Primary Storage
 */
int main(void)
{
    /* Initialize Program */

    system_init();
    gpio_enable();

    // Inputs
    EL_PORT_INPUT_PIN(A, 1); // (dI2) Optical Sensor
    EL_PORT_INPUT_PIN(C, 0); // (jX) Joystick Center

    color_sensor_init();

    // Outputs
    EL_PORT_OUTPUT_PIN(B, 3); // (dO1) Diverter Solenoid
    EL_PORT_OUTPUT_PIN(B, 4); // (dO2) Conveyor
    EL_PORT_OUTPUT_PIN(A, 13); // (dO4) Stack Piston A
    EL_PORT_OUTPUT_PIN(A, 14); // (dO5) Stack Piston B
    EL_PORT_OUTPUT_PIN(A, 15); // (dO6) Pneumatic Punch

    // Turn on OLED display
    el_display_powerup();
    el_display_on();
    el_display_clearScreen(false); // Fill display memory with zeros.

    el_display_printString(" Mech 2019", 0, 0, false, 1, false);
    el_display_printString("J. Bari   ", 0, 9, false, 1, false);
    el_display_printString("J. Whiting", 0, 18, false, 1, false);
    el_display_printString(" Press To ", 0, 27, false, 1, false);
    el_display_printString("Start Demo", 0, 38, false, 1, false);
    el_display_writeMem();

    EL_PORT_SET_PIN(B, 9, 1);
    EL_PORT_SET_PIN(A, 15, 0); // Disable Punch
    EL_PORT_SET_PIN(B, 4, 0); //Deactivate Conveyor

    while (!EL_PORT_GET_PIN(C, 0)) // Don't start until button is pressed.
    {
        if (EL_PORT_GET_PIN(B, 12))
        {
            EL_PORT_SET_PIN(A, 13, 1);
            EL_PORT_SET_PIN(A, 14, 0);
        }

        if (EL_PORT_GET_PIN(A, 11))
        {
            EL_PORT_SET_PIN(A, 13, 0);
            EL_PORT_SET_PIN(A, 14, 1);
        }
        el_usleep(1);
    }

    // Extend Piston
    EL_PORT_SET_PIN(A, 13, 0);
    EL_PORT_SET_PIN(A, 14, 1);

    el_display_clearScreen(false);
    el_display_printString(" Demo 2019", 0, 0, false, 1, false);
    int8_t i;
    for (i = 12; i < 52; i += 8) {
        el_display_drawCircle(i, 12, 3, true, true);
        el_display_drawCircle(i, 20, 3, true, true);
    }
    el_display_writeMem();
    /* Run a cycle for each block in the Stack Magazine */

    while (cycles > 0) {
        // Retract Piston
        EL_PORT_SET_PIN(A, 13, 1);
        EL_PORT_SET_PIN(A, 14, 0);

        sleepUpdate(200);

        // Extend Piston
        EL_PORT_SET_PIN(A, 13, 0);
        EL_PORT_SET_PIN(A, 14, 1);

        // Wait for Cylinder to break Optical Sensor's beam
        while (!EL_PORT_GET_PIN(A, 1)) { sleepUpdate(1); }

        EL_PORT_SET_PIN(B, 4, 1); // Activate Conveyor

        bool error;

        sleepUpdate(100);
        int color = color_sensor_read(&error); // Actually read color value

        char t[8];
        sprintf(t, "%04d", color);
        el_display_printString(t, 30, 36, false, 1, false); // Display color value on OLED display.

        /* Color is SILVER : Punch cylinder, then sort into bucket. */
        if (color < 400 && color > 10) {
            el_display_printString("SILV ", 0, 27, false, 1, false);
            el_display_writeMem();
            EL_PORT_SET_PIN(B, 4, 1); // Activate Conveyor
            sleepUpdate(300);         // Wait for Cylinder under punch
            EL_PORT_SET_PIN(B, 4, 0); // Deactivate Conveyor
            EL_PORT_SET_PIN(A, 15, 1); // Punch Cylinder
            sleepUpdate(300);
            EL_PORT_SET_PIN(A, 15, 0); // Disable Punch
            EL_PORT_SET_PIN(B, 3, 1); // Enable Diverter
            EL_PORT_SET_PIN(B, 4, 1); // Activate Conveyor
            sleepUpdate(1300);         // Wait for Cylinder to be sorted
            EL_PORT_SET_PIN(B, 3, 0); // Disable Diverter
        }

            /* Color is PURPLE : Enable diverter, sorts into tray. */
        else if (color < 900 && color > 700) {
            el_display_printString("PURP", 0, 27, false, 1, false);
            el_display_writeMem();
            EL_PORT_SET_PIN(B, 4, 1); // Activate Conveyor
            EL_PORT_SET_PIN(B, 3, 1); // Enable Diverter
            sleepUpdate(1500);        // Wait for Cylinder to be sorted
            EL_PORT_SET_PIN(B, 3, 0); // Disable Diverter
        }

            /* Color is BLUE : Do nothing, sorts into bucket. */
        else if (color < 1200 && color > 1000) {
            el_display_printString("BLUE", 0, 27, false, 1, false);
            el_display_writeMem();
            EL_PORT_SET_PIN(B, 4, 1); // Activate Conveyor
            sleepUpdate(1750); // Wait for Cylinder to be sorted
        }

        EL_PORT_SET_PIN(B, 4, 0); // Deactivate Conveyor
        --cycles;

        while (!EL_PORT_GET_PIN(C, 0)) // Don't start until button is pressed.
        { el_usleep(1); }
    }

    el_display_clearScreen(false);
    el_display_printString("   Demo ", 0, 0, false, 1, false);
    el_display_printString(" Complete.", 0, 9, false, 1, false);
    el_display_writeMem();

    el_adc_done(&ADC);

    return 0;
}
