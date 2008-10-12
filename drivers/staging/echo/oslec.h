/*
 *  OSLEC - A line echo canceller.  This code is being developed
 *          against and partially complies with G168. Using code from SpanDSP
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *         and David Rowe <david_at_rowetel_dot_com>
 *
 * Copyright (C) 2001 Steve Underwood and 2007-2008 David Rowe
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __OSLEC_H
#define __OSLEC_H

/* TODO: document interface */

/* Mask bits for the adaption mode */
#define ECHO_CAN_USE_ADAPTION	0x01
#define ECHO_CAN_USE_NLP	0x02
#define ECHO_CAN_USE_CNG	0x04
#define ECHO_CAN_USE_CLIP	0x08
#define ECHO_CAN_USE_TX_HPF	0x10
#define ECHO_CAN_USE_RX_HPF	0x20
#define ECHO_CAN_DISABLE	0x40

/*!
    G.168 echo canceller descriptor. This defines the working state for a line
    echo canceller.
*/
typedef struct echo_can_state echo_can_state_t;

/*! Create a voice echo canceller context.
    \param len The length of the canceller, in samples.
    \return The new canceller context, or NULL if the canceller could not be created.
*/
echo_can_state_t *echo_can_create(int len, int adaption_mode);

/*! Free a voice echo canceller context.
    \param ec The echo canceller context.
*/
void echo_can_free(echo_can_state_t *ec);

/*! Flush (reinitialise) a voice echo canceller context.
    \param ec The echo canceller context.
*/
void echo_can_flush(echo_can_state_t *ec);

/*! Set the adaption mode of a voice echo canceller context.
    \param ec The echo canceller context.
    \param adapt The mode.
*/
void echo_can_adaption_mode(echo_can_state_t *ec, int adaption_mode);

void echo_can_snapshot(echo_can_state_t *ec);

/*! Process a sample through a voice echo canceller.
    \param ec The echo canceller context.
    \param tx The transmitted audio sample.
    \param rx The received audio sample.
    \return The clean (echo cancelled) received sample.
*/
int16_t echo_can_update(echo_can_state_t *ec, int16_t tx, int16_t rx);

/*! Process to high pass filter the tx signal.
    \param ec The echo canceller context.
    \param tx The transmitted auio sample.
    \return The HP filtered transmit sample, send this to your D/A.
*/
int16_t echo_can_hpf_tx(echo_can_state_t *ec, int16_t tx);

#endif	/* __OSLEC_H */
