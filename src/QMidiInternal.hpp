/*
 * Copyright 2023 Jacob Secunda
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <qglobal.h>


const quint8 STATUS_CHANNEL_MASK = 0xF0;
const quint8 STATUS_VOICE_MASK = 0x0F;

enum MessageType : quint8 {
	NoteOff = 0x80,
	NoteOn = 0x90,
	PolyKeyPressure = 0xA0,
	ControlChange = 0xB0,
	ProgramChange = 0xC0,
	ChannelPressure = 0xD0,
	PitchBend = 0xE0,
	SystemExclusive = 0xF0,
	SystemExclusiveStart = 0xF0,
	SystemExclusiveEnd = 0xF7
};
