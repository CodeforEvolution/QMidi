/*
 * Copyright 2012-2016 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "QMidiOut.h"
#include "QMidiIn.h"
#include "QMidiInternal.hpp"
#include "OS/QMidi_ALSA.h"

#include <QByteArray>
#include <QStringList>
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <alsa/seq_midi_event.h>

// # pragma mark - QMidiOut

struct NativeMidiOutInstances {
	snd_seq_t* midiOutPtr;
};

// TODO: error reporting

static QMap<QString, QString> buildDevicesMap(bool forInput)
{
	qint32 streams = forInput ? SND_SEQ_OPEN_INPUT : SND_SEQ_OPEN_OUTPUT;
	quint32 cap = forInput ? (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE)
		: (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ);

	QMap<QString, QString> ret;

	snd_seq_client_info_t* cinfo = Q_NULLPTR;
	snd_seq_port_info_t* pinfo = Q_NULLPTR;
	qint32 client = -1;
	qint32 err = -1;
	snd_seq_t* handle = Q_NULLPTR;

	err = snd_seq_open(&handle, "hw", streams, 0);
	if (err < 0) {
		/* Use snd_strerror(errno) to get the error here. */
		return ret;
	}

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	while (snd_seq_query_next_client(handle, cinfo) >= 0) {
		client = snd_seq_client_info_get_client(cinfo);
		snd_seq_port_info_alloca(&pinfo);
		snd_seq_port_info_set_client(pinfo, client);

		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(handle, pinfo) >= 0) {
			if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap) {
				QString port = QString::number(snd_seq_port_info_get_client(pinfo));
				port += ":" + QString::number(snd_seq_port_info_get_port(pinfo));
				QString name = snd_seq_client_info_get_name(cinfo);
				ret.insert(port, name);
			}
		}
	}

	snd_seq_close(handle);

	return ret;
}

QMap<QString, QString> QMidiOut::devices()
{
	return buildDevicesMap(false);
}

bool QMidiOut::connect(QString outDeviceId)
{
	if (fConnected)
		disconnect();

	fMidiPtrs = new (std::nothrow) NativeMidiOutInstances;
	Q_CHECK_PTR(fMidiPtrs);

	int err = snd_seq_open(&fMidiPtrs->midiOutPtr, "default", SND_SEQ_OPEN_OUTPUT, 0);
	if (err < 0) {
		delete fMidiPtrs;
		return false;
	}
	snd_seq_set_client_name(fMidiPtrs->midiOutPtr, "QMidi");

	snd_seq_create_simple_port(fMidiPtrs->midiOutPtr, "Output Port", SND_SEQ_PORT_CAP_READ,
		SND_SEQ_PORT_TYPE_MIDI_GENERIC);

	QStringList l = outDeviceId.split(":");
	int client = l.at(0).toInt();
	int port = l.at(1).toInt();
	snd_seq_connect_to(fMidiPtrs->midiOutPtr, 0, client, port);

	fDeviceId = outDeviceId;
	fConnected = true;
	return true;
}

void QMidiOut::disconnect()
{
	if (!fConnected)
		return;

	QStringList l = fDeviceId.split(":");
	int client = l.at(0).toInt();
	int port = l.at(1).toInt();

	snd_seq_disconnect_from(fMidiPtrs->midiOutPtr, 0, client, port);
	fConnected = false;

	snd_seq_close(fMidiPtrs->midiOutPtr);
	delete fMidiPtrs;
	fMidiPtrs = Q_NULLPTR;
}

void QMidiOut::sendMsg(qint32 msg)
{
	if (!fConnected)
		return;

	char buf[3];
	buf[0] = msg & 0xFF;
	buf[1] = (msg >> 8) & 0xFF;
	buf[2] = (msg >> 16) & 0xFF;

	snd_seq_event_t ev;
	snd_midi_event_t* mev = Q_NULLPTR;

	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, 0);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);

	snd_midi_event_new(3, &mev);
	snd_midi_event_resize_buffer(mev, 3);
	snd_midi_event_encode(mev, (unsigned char*)&buf, 3, &ev);

	snd_seq_event_output(fMidiPtrs->midiOutPtr, &ev);
	snd_seq_drain_output(fMidiPtrs->midiOutPtr);

	snd_midi_event_free(mev);
}

void QMidiOut::sendSysEx(const QByteArray &data)
{
	if (!fConnected)
		return;

	snd_seq_event_t ev;
	snd_midi_event_t* mev = Q_NULLPTR;

	snd_seq_ev_set_source(&ev, 0);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);

	snd_midi_event_new(data.size(), &mev);
	snd_midi_event_resize_buffer(mev, data.size());
	snd_midi_event_encode(mev, (unsigned char*)data.constData(), data.size(), &ev);

	snd_seq_event_output(fMidiPtrs->midiOutPtr, &ev);
	snd_seq_drain_output(fMidiPtrs->midiOutPtr);

	snd_midi_event_free(mev);
}

// # pragma mark - QMidiIn

struct NativeMidiInInstances {
	//! \brief midiIn is a reference to the MIDI input device
	snd_seq_t* midiIn;

	//! \brief receiveThread is a reference to the MIDI input receive thread.
	QMidiInternal::MidiInReceiveThread* receiveThread;
};

QMap<QString, QString> QMidiIn::devices()
{
	return buildDevicesMap(true);
}

bool QMidiIn::connect(QString inDeviceId)
{
	if (fConnected)
		disconnect();

	fMidiPtrs = new (std::nothrow) NativeMidiInInstances;
	Q_CHECK_PTR(fMidiPtrs);

	int err = snd_seq_open(&fMidiPtrs->midiIn, "default", SND_SEQ_OPEN_INPUT, 0);
	if (err < 0) {
		delete fMidiPtrs;
		fMidiPtrs = Q_NULLPTR;
		return false;
	}
	snd_seq_set_client_name(fMidiPtrs->midiIn, "QMidi");
	snd_seq_create_simple_port(fMidiPtrs->midiIn, "Input Port", SND_SEQ_PORT_CAP_WRITE,
		SND_SEQ_PORT_TYPE_MIDI_GENERIC);

	// connect the device to our previously created port
	QStringList l = inDeviceId.split(":");
	int client = l.at(0).toInt();
	int port = l.at(1).toInt();
	snd_seq_connect_from(fMidiPtrs->midiIn, 0, client, port);

	fDeviceId = inDeviceId;
	fConnected = true;
	return true;
}

void QMidiIn::disconnect()
{
	if (!fConnected)
		return;

	QStringList l = fDeviceId.split(":");
	int client = l.at(0).toInt();
	int port = l.at(1).toInt();

	snd_seq_disconnect_to(fMidiPtrs->midiIn, 0, client, port);
	fConnected = false;

	snd_seq_close(fMidiPtrs->midiIn);
	delete fMidiPtrs;
	fMidiPtrs = Q_NULLPTR;
}

void QMidiIn::start()
{
	if (!fConnected)
		return;

	fMidiPtrs->receiveThread = new (std::nothrow) QMidiInternal::MidiInReceiveThread(this, fMidiPtrs);
	Q_CHECK_PTR(fMidiPtrs->receiveThread);

	fMidiPtrs->receiveThread->start();
}

void QMidiIn::stop()
{
	if (!fConnected)
		return;

	fMidiPtrs->receiveThread->requestInterruption();
	fMidiPtrs->receiveThread->wait();
	fMidiPtrs->receiveThread->deleteLater();
	fMidiPtrs->receiveThread = Q_NULLPTR;
}

QMidiInternal::MidiInReceiveThread::MidiInReceiveThread(QMidiIn* qMidiIn, NativeMidiInInstances* fMidiPtrs, QObject* parent)
	: QThread(parent), fMidiIn(qMidiIn), fMidiPtrs(fMidiPtrs)
{}

void QMidiInternal::MidiInReceiveThread::run()
{
	snd_seq_event_t* ev = Q_NULLPTR;
	int data = 0;
	int value = 0;

	while (!isInterruptionRequested() && fMidiIn->isConnected()) {
		snd_seq_event_input(fMidiPtrs->midiIn, &ev);

		switch (ev->type) {
		case SND_SEQ_EVENT_SYSEX:
		{
			QByteArray ba = QByteArray(reinterpret_cast<const char*>(ev->data.ext.ptr), ev->data.ext.len);
			emit(fMidiIn->midiSysExEvent(ba));
			continue;
		}
		case SND_SEQ_EVENT_NOTEOFF:
			data = MessageType::NoteOff
					| (ev->data.note.channel & 0x0F)
					| ((ev->data.note.note & 0x7F) << 8)
					| ((ev->data.note.velocity & 0x7F) << 16);
			break;
		case SND_SEQ_EVENT_NOTEON:
			data = MessageType::NoteOn
					| (ev->data.note.channel & 0x0F)
					| ((ev->data.note.note & 0x7F) << 8)
					| ((ev->data.note.velocity & 0x7F) << 16);
			break;
		case SND_SEQ_EVENT_KEYPRESS:
			data = MessageType::PolyKeyPressure
					| (ev->data.note.channel & 0x0F)
					| ((ev->data.note.note & 0x7F) << 8)
					| ((ev->data.note.velocity & 0x7F) << 16);
			break;
		case SND_SEQ_EVENT_CONTROLLER:
			data = MessageType::ControlChange
					| (ev->data.control.channel & 0x0F)
					| ((ev->data.control.param & 0x7F) << 8)
					| ((ev->data.control.value & 0x7F) << 16);
			break;
		case SND_SEQ_EVENT_PGMCHANGE:
			data = MessageType::ProgramChange
					| (ev->data.control.channel & 0x0F)
					| ((ev->data.control.value & 0x7F) << 8);
			break;
		case SND_SEQ_EVENT_CHANPRESS:
			data = MessageType::ChannelPressure
					| (ev->data.control.channel & 0x0F)
					| ((ev->data.control.value & 0x7F) << 8);
			break;
		case SND_SEQ_EVENT_PITCHBEND:
			value = ev->data.control.value + 8192;
			data = MessageType::PitchBend
					| (ev->data.note.channel & 0x0F)
					| ((value & 0x7F) << 8)
					| (((value >> 7) & 0x7F) << 16);
			break;
		default:
			continue;
		}

		emit(fMidiIn->midiEvent(static_cast<quint32>(data), ev->time.tick));
	}
}
