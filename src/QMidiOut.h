/*
 * Copyright 2012-2016 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <QMap>
#include <QString>

class QMidiEvent;
struct NativeMidiOutInstances;

class QMidiOut
{
public:
	static QMap<QString /* device id */, QString /* device name */> devices();

	QMidiOut();
	~QMidiOut();
	bool connect(QString outDeviceId);
	void disconnect();
	void sendMsg(qint32 msg);
	//! \brief sendSysex Sends a raw MIDI System Exclusive (SysEx) message.
	//! \param data The data to send.
	void sendSysEx(const QByteArray& data);

	void sendEvent(const QMidiEvent& e);
	void setInstrument(qint32 voice, qint32 instr);
	void noteOn(qint32 note, qint32 voice, qint32 velocity = 64);
	void noteOff(qint32 note, qint32 voice, qint32 velocity = 0);
	void pitchWheel(qint32 voice, qint32 value);
	void channelAftertouch(qint32 voice, qint32 value);
	void polyphonicAftertouch(qint32 note, qint32 voice, qint32 value);
	void controlChange(qint32 voice, qint32 number, qint32 value);
	void stopAll();
	void stopAll(qint32 voice);

	bool isConnected() const { return fConnected; }
	QString deviceId() const { return fDeviceId; }

private:
	QString fDeviceId;
	NativeMidiOutInstances* fMidiPtrs;
	bool fConnected;
};
