/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *
 *	@brief Example using the wiiuse API.
 *
 *	This file is an example of how to use the wiiuse library.
 */

#include <stdio.h> /* for printf */
#include "wiiuse.h" /* for wiimote_t, classic_ctrl_t, etc */

#ifndef WIIUSE_WIN32
#include <unistd.h> /* for usleep */
#endif
#define MAX_WIIMOTES 4
int firstRun = 0;
double frontLeftCal, frontRightCal, backLeftCal, backRightCal;
double frontLeft, frontRight, backLeft, backRight;


/*
 *	@brief Callback that handles an event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	This function is called automatically by the wiiuse library when an
 *	event occurs on the specified wiimote.
 */
void handle_event(struct wiimote_t *wm)
{
    printf("\n\n--- EVENT [id %i] ---\n", wm->unid);

    /* if a button is pressed, report it */
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_A))
    {
        printf("A pressed\n");
    }

    /* show events specific to supported expansions */
    /* wii balance board */
    struct wii_board_t *wb = (wii_board_t *)&wm->exp.wb;

    if (!firstRun)
    {
        frontLeftCal  = wb->tl;
        frontRightCal = wb->tr;
        backLeftCal   = wb->bl;
        backRightCal  = wb->br;
        firstRun      = 1;
    }
    frontLeft  = wb->tl - frontLeftCal;
    frontRight = wb->tr - frontRightCal;
    backLeft   = wb->bl - backLeftCal;
    backRight  = wb->br - backRightCal;

    float total = frontLeft + frontRight + backLeft + backRight;
    float x     = ((frontRight + backRight) / total) * 2 - 1;
    float y     = ((frontLeft + frontRight) / total) * 2 - 1;
    printf("Weight: %f kg @ (%f, %f)\n", total, x, y);
    printf("Interpolated weight: frontLeft:%f  frontRight:%f  backLeft:%f  backRight:%f\n", frontLeft,
           frontRight, backLeft, backRight);
    printf("Raw: frontLeft:%d  frontRight:%d  backLeft:%d  backRight:%d\n", wb->rtl, wb->rtr, wb->rbl, wb->rbr);
}

/**
 *	@brief Callback that handles a read event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Pointer to the filled data block.
 *	@param len		Length in bytes of the data block.
 *
 *	This function is called automatically by the wiiuse library when
 *	the wiimote has returned the full data requested by a previous
 *	call to wiiuse_read_data().
 *
 *	You can read data on the wiimote, such as Mii data, if
 *	you know the offset address and the length.
 *
 *	The \a data pointer was specified on the call to wiiuse_read_data().
 *	At the time of this function being called, it is not safe to deallocate
 *	this buffer.
 */
void handle_read(struct wiimote_t *wm, byte *data, unsigned short len)
{
    int i = 0;

    printf("\n\n--- DATA READ [wiimote id %i] ---\n", wm->unid);
    printf("finished read of size %i\n", len);
    for (; i < len; ++i)
    {
        if (!(i % 16))
        {
            printf("\n");
        }
        printf("%x ", data[i]);
    }
    printf("\n\n");
}

/**
 *	@brief Callback that handles a controller status event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *	@param attachment		Is there an attachment? (1 for yes, 0 for no)
 *	@param speaker			Is the speaker enabled? (1 for yes, 0 for no)
 *	@param ir				Is the IR support enabled? (1 for yes, 0 for no)
 *	@param led				What LEDs are lit.
 *	@param battery_level	Battery level, between 0.0 (0%) and 1.0 (100%).
 *
 *	This occurs when either the controller status changed
 *	or the controller status was requested explicitly by
 *	wiiuse_status().
 *
 *	One reason the status can change is if the nunchuk was
 *	inserted or removed from the expansion port.
 */
void handle_ctrl_status(struct wiimote_t *wm)
{
    printf("\n\n--- CONTROLLER STATUS [wiimote id %i] ---\n", wm->unid);

    printf("attachment:      %i\n", wm->exp.type);
    printf("speaker:         %i\n", WIIUSE_USING_SPEAKER(wm));
    printf("ir:              %i\n", WIIUSE_USING_IR(wm));
    printf("leds:            %i %i %i %i\n", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2),
           WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
    printf("battery:         %f %%\n", wm->battery_level);
}

/**
 *	@brief Callback that handles a disconnection event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *
 *	This can happen if the POWER button is pressed, or
 *	if the connection is interrupted.
 */
void handle_disconnect(wiimote *wm) { printf("\n\n--- DISCONNECTED [wiimote id %i] ---\n", wm->unid); }
short any_wiimote_connected(wiimote **wm, int wiimotes)
{
    int i;
    if (!wm)
    {
        return 0;
    }

    for (i = 0; i < wiimotes; i++)
    {
        if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i]))
        {
            return 1;
        }
    }

    return 0;
}

/**
 *	@brief main()
 *
 *	Connect to up to two wiimotes and print any events
 *	that occur on either device.
 */
int main(int argc, char **argv)
{
    wiimote **wiimotes;
    int found, connected;

    /*
     *	Initialize an array of wiimote objects.
     *
     *	The parameter is the number of wiimotes I want to create.
     */
    wiimotes = wiiuse_init(MAX_WIIMOTES);
    /*
     *	Find wiimote devices
     *
     *	Now we need to find some wiimotes.
     *	Give the function the wiimote array we created, and tell it there
     *	are MAX_WIIMOTES wiimotes we are interested in.
     *
     *	Set the timeout to be 5 seconds.
     *
     *	This will return the number of actual wiimotes that are in discovery mode.
     */
    found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
    if (!found)
    {
        printf("No wiimotes found.\n");
        return 0;
    }

    /*
     *	Connect to the wiimotes
     *
     *	Now that we found some wiimotes, connect to them.
     *	Give the function the wiimote array and the number
     *	of wiimote devices we found.
     *
     *	This will return the number of established connections to the found wiimotes.
     */
    connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
    if (connected)
    {
        printf("Connected to %i wiimotes (of %i found).\n", connected, found);
    } else
    {
        printf("Failed to connect to any wiimote.\n");
        return 0;
    }

    /*
     *	Now set the LEDs and rumble for a second so it's easy
     *	to tell which wiimotes are connected (just like the wii does).
     */
    wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
    wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_2);
    wiiuse_set_leds(wiimotes[2], WIIMOTE_LED_3);
    wiiuse_set_leds(wiimotes[3], WIIMOTE_LED_4);
    wiiuse_rumble(wiimotes[0], 1);
    wiiuse_rumble(wiimotes[1], 1);

#ifndef WIIUSE_WIN32
    usleep(200000);
#else
    Sleep(200);
#endif
    wiiuse_rumble(wiimotes[0], 0);
    wiiuse_rumble(wiimotes[1], 0);

    /*
     *	This is the main loop
     *
     *	wiiuse_poll() needs to be called with the wiimote array
     *	and the number of wiimote structures in that array
     *	(it doesn't matter if some of those wiimotes are not used
     *	or are not connected).
     *
     *	This function will set the event flag for each wiimote
     *	when the wiimote has things to report.
     */

    while (any_wiimote_connected(wiimotes, MAX_WIIMOTES))
    {
        if (wiiuse_poll(wiimotes, MAX_WIIMOTES))
        {
            /*
             *	This happens if something happened on any wiimote.
             *	So go through each one and check if anything happened.
             */
            int i = 0;
            for (; i < MAX_WIIMOTES; ++i)
            {
                switch (wiimotes[i]->event)
                {
                case WIIUSE_EVENT:
                    /* a generic event occurred */
                    handle_event(wiimotes[i]);
                    break;

                case WIIUSE_STATUS:
                    /* a status event occurred */
                    handle_ctrl_status(wiimotes[i]);
                    break;

                case WIIUSE_DISCONNECT:
                case WIIUSE_UNEXPECTED_DISCONNECT:
                    /* the wiimote disconnected */
                    handle_disconnect(wiimotes[i]);
                    break;

                case WIIUSE_READ_DATA:
                    /*
                     *	Data we requested to read was returned.
                     *	Take a look at wiimotes[i]->read_req
                     *	for the data.
                     */
                    break;

                case WIIUSE_WII_BOARD_CTRL_INSERTED:
                    printf("Balance board controller inserted.\n");
                    break;

                case WIIUSE_WII_BOARD_CTRL_REMOVED:
                    /* some expansion was removed */
                    handle_ctrl_status(wiimotes[i]);
                    printf("An expansion was removed.\n");
                    break;

                default:
                    break;
                }
            }
        }
    }

    /*
     *	Disconnect the wiimotes
     */
    wiiuse_cleanup(wiimotes, MAX_WIIMOTES);

    return 0;
}
