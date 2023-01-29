/*
 * Copyright 2012-2016 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "QMidiOut.h"

#include "QMidiFile.h"
#include "QMidiInternal.hpp"

// TODO: error reporting

QMidiOut::QMidiOut()
	: fMidiPtrs(Q_NULLPTR),
	  fConnected(false)
{
}
QMidiOut::~QMidiOut()
{
	if (fConnected)
		disconnect();
}

void QMidiOut::sendEvent(const QMidiEvent& e)
{
	if (e.type() == QMidiEvent::SysEx) {
		sendSysEx(e.data());
		return;
	}

	sendMsg(e.message());
}

void QMidiOut::setInstrument(qint32 voice, qint32 instr)
{
	qint32 msg = 0xC0 + voice;
	msg |= instr << 8;
	sendMsg(msg);
}

void QMidiOut::noteOn(qint32 note, qint32 voice, qint32 velocity)
{
	qint32 msg = MessageType::NoteOn + voice;
	msg |= note << 8;
	msg |= velocity << 16;
	sendMsg(msg);
}

void QMidiOut::noteOff(qint32 note, qint32 voice, qint32 velocity)
{
	qint32 msg = MessageType::NoteOff + voice;
	msg |= note << 8;
	msg |= velocity << 16;
	sendMsg(msg);
}

void QMidiOut::pitchWheel(qint32 voice, qint32 value)
{
	qint32 msg = MessageType::PitchBend + voice;
	msg |= (value & 0x7F) << 8; // fine adjustment (ignored by many synths)
	msg |= (value / 128) << 16; // coarse adjustment
	sendMsg(msg);
}

void QMidiOut::channelAftertouch(qint32 voice, qint32 value)
{
	qint32 msg = MessageType::ChannelPressure + voice;
	msg |= value << 8;
	sendMsg(msg);
}

void QMidiOut::polyphonicAftertouch(qint32 note, qint32 voice, qint32 value)
{
	qint32 msg = MessageType::PolyKeyPressure + voice;
	msg |= note << 8;
	msg |= value << 16;
	sendMsg(msg);
}

void QMidiOut::controlChange(qint32 voice, qint32 number, qint32 value)
{
	qint32 msg = MessageType::ControlChange + voice;
	msg |= number << 8;
	msg |= value << 16;
	sendMsg(msg);
}

void QMidiOut::stopAll()
{
	for (qint32 i = 0; i < 16; i++)
		stopAll(i);
}

void QMidiOut::stopAll(qint32 voice)
{
	sendMsg((MessageType::ControlChange | voice) | (0x7B << 8));
}
