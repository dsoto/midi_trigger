/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the MIDI demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "MIDI.h"

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = 1,

				.DataINEndpointNumber      = MIDI_STREAM_IN_EPNUM,
				.DataINEndpointSize        = MIDI_STREAM_EPSIZE,
				.DataINEndpointDoubleBank  = false,

				.DataOUTEndpointNumber     = MIDI_STREAM_OUT_EPNUM,
				.DataOUTEndpointSize       = MIDI_STREAM_EPSIZE,
				.DataOUTEndpointDoubleBank = false,
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

    // output LED on 8u2 board on PB4
    DDRB = 1 << 4;
    PORTB = 0xFF;

    // set entire port d to input to read button presses
    DDRD = 0x00;


	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
        // polling interval for button presses
        _delay_ms(20);
        // check botton press
		CheckJoystickMovement();

		MIDI_EventPacket_t ReceivedMIDIEvent;
		while (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
			if ((ReceivedMIDIEvent.Command == (MIDI_COMMAND_NOTE_ON >> 4)) && (ReceivedMIDIEvent.Data3 > 0))
			  LEDs_SetAllLEDs(ReceivedMIDIEvent.Data2 > 64 ? LEDS_LED1 : LEDS_LED2);
			else
			  LEDs_SetAllLEDs(LEDS_NO_LEDS);
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	//clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	//Joystick_Init();
	LEDs_Init();
	//Buttons_Init();
	USB_Init();
}

/** Checks for changes in the position of the board joystick, sending MIDI events to the host upon each change. */
void CheckJoystickMovement(void) {

    // bottom three bits of each of these variables is the button press state
    static uint8_t currState = 0;
    static uint8_t prevState = 0;
    static uint8_t playState = 0;

    // array of pitches {kick, hi-hat, snare}
    static int pitch[3] = {0x24, 0x2A, 0x26};

	uint8_t MIDICommand = 0;
	uint8_t MIDIPitch;
    uint8_t Channel = MIDI_CHANNEL(10);

    // enable pullup resistor
    // i can't move this into the main routine for some reason i don't understand
    PORTD = 0xFF;

    // read pin d and shift to lowest 3 bits
    // button press will pull state to low
    currState = PIND >> 4;

    // play note if currState is true (bit low) and prevState was false (bit high)
    playState = ~currState & prevState;

    for (int i=0; i<=2; i++) {
        // check i-th bit for button press
        if (playState & 1 << i) {
            MIDICommand = MIDI_COMMAND_NOTE_ON;
            MIDIPitch = pitch[i];
            MIDI_EventPacket_t MIDIEvent = (MIDI_EventPacket_t) {
                    .CableNumber = 0,
                    .Command     = (MIDICommand >> 4),

                    .Data1       = MIDICommand | Channel,
                    .Data2       = MIDIPitch,
                    .Data3       = MIDI_STANDARD_VELOCITY,
            };
            MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent);
            MIDI_Device_Flush(&Keyboard_MIDI_Interface);
            // if button press pull LED pin low and light LED
            PORTB = 0x00;
        }
    }
    // turn LED off
    _delay_ms(10);
    PORTB = 1 << 4;

    // advance state of button press variables
    prevState = currState;
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

