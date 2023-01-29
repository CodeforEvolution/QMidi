/*
 * Copyright 2003-2012 by David G. Slomin
 * Copyright 2012-2015 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "QMidiFile.h"

#include <QFile>
#include <cstdlib>

#include "QMidiInternal.hpp"


QMidiEvent::QMidiEvent()
{
	fTrackNumber = -1;
	fType = Invalid;
	fVoice = -1;
	fNote = -1;
	fVelocity = -1;
	fAmount = -1;
	fNumber = -1;
	fValue = -1;
	fNumerator = -1;
	fDenominator = -1;
	fData = "";
	fTick = -1;
}
QMidiEvent::~QMidiEvent()
{
}

quint32 QMidiEvent::message() const
{
	union {
		quint8 data_as_bytes[4];
		quint32 data_as_uint32;
	} u;

	switch (fType) {
	case NoteOff:
		u.data_as_bytes[0] = MessageType::NoteOff | fVoice;
		u.data_as_bytes[1] = fNote;
		u.data_as_bytes[2] = fVelocity;
		u.data_as_bytes[3] = 0;
		break;

	case NoteOn:
		u.data_as_bytes[0] = MessageType::NoteOn | fVoice;
		u.data_as_bytes[1] = fNote;
		u.data_as_bytes[2] = fVelocity;
		u.data_as_bytes[3] = 0;
		break;

	case KeyPressure:
		u.data_as_bytes[0] = MessageType::PolyKeyPressure | fVoice;
		u.data_as_bytes[1] = fNote;
		u.data_as_bytes[2] = fAmount;
		u.data_as_bytes[3] = 0;
		break;

	case ControlChange:
		u.data_as_bytes[0] = MessageType::ControlChange | fVoice;
		u.data_as_bytes[1] = fNumber;
		u.data_as_bytes[2] = fValue;
		u.data_as_bytes[3] = 0;
		break;

	case ProgramChange:
		u.data_as_bytes[0] = MessageType::ProgramChange | fVoice;
		u.data_as_bytes[1] = fNumber;
		u.data_as_bytes[2] = 0;
		u.data_as_bytes[3] = 0;
		break;

	case ChannelPressure:
		u.data_as_bytes[0] = MessageType::ChannelPressure | fVoice;
		u.data_as_bytes[1] = fAmount;
		u.data_as_bytes[2] = 0;
		u.data_as_bytes[3] = 0;
		break;

	case PitchWheel:
		u.data_as_bytes[0] = MessageType::PitchBend | fVoice;
		u.data_as_bytes[2] = fValue >> 7;
		u.data_as_bytes[1] = fValue;
		u.data_as_bytes[3] = 0;
		break;

	default:
		return 0;
		break;
	}
	return u.data_as_uint32;
}

void QMidiEvent::setMessage(quint32 data)
{
	union {
		quint32 data_as_uint32;
		quint8 data_as_bytes[4];
	} u;

	u.data_as_uint32 = data;

	switch (u.data_as_bytes[0] & STATUS_CHANNEL_MASK) {
	case MessageType::NoteOff: {
		setType(NoteOff);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setNote(u.data_as_bytes[1]);
		setVelocity(u.data_as_bytes[2]);
		return;
	}
	case MessageType::NoteOn: {
		setType(NoteOn);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setNote(u.data_as_bytes[1]);
		setVelocity(u.data_as_bytes[2]);
		return;
	}
	case MessageType::PolyKeyPressure: {
		setType(KeyPressure);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setNote(u.data_as_bytes[1]);
		setAmount(u.data_as_bytes[2]);
		return;
	}
	case MessageType::ControlChange: {
		setType(ControlChange);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setNumber(u.data_as_bytes[1]);
		setValue(u.data_as_bytes[2]);
		return;
	}
	case MessageType::ProgramChange: {
		setType(ProgramChange);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setNumber(u.data_as_bytes[1]);
		return;
	}
	case MessageType::ChannelPressure: {
		setType(ChannelPressure);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setAmount(u.data_as_bytes[1]);
		return;
	}
	case MessageType::PitchBend: {
		setType(PitchWheel);
		setVoice(u.data_as_bytes[0] & STATUS_VOICE_MASK);
		setValue((u.data_as_bytes[2] << 7) | u.data_as_bytes[1]);
		return;
	}
	}
}

float QMidiEvent::tempo()
{
	quint8* buffer = Q_NULLPTR;
	qint32 midi_tempo = 0;

	if ((fType != Meta) || (fNumber != Tempo)) {
		return -1;
	}

	buffer = (quint8*)fData.constData();
	midi_tempo = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
	return (float)(60000000.0 / midi_tempo);
}

/* End of QMidiEvent functions, on to QMidiFile */

QMidiFile::QMidiFile()
	: fDisableSort(false)
{
	clear();
}
QMidiFile::~QMidiFile()
{
	clear();
}

void QMidiFile::clear()
{
	for (QMidiEvent* e : fEvents)
		delete e;
	fEvents.clear();
	fTempoEvents.clear();
	fTracks.clear();
	fDivType = PPQ;
	fResolution = 0;
	fFileFormat = 1;
}

QMidiFile* QMidiFile::oneTrackPerVoice()
{
	if (fFileFormat != 0) {
		return 0;
	}

	QMidiFile* ret = new (std::nothrow) QMidiFile();
	Q_CHECK_PTR(ret);

	ret->setDivisionType(fDivType);
	ret->setResolution(fResolution);
	ret->setFileFormat(1);

	QMap<qint32 /*voice*/, qint32 /*track*/> tracks;
	ret->createTrack(); /* Track 0 */
	ret->fDisableSort = true;
	for (QMidiEvent* event : fEvents) {
		QMidiEvent* e = new (std::nothrow) QMidiEvent();
		Q_CHECK_PTR(e);

		*e = *event; /* copy data buffer */
		if ((e->type() == QMidiEvent::Meta) && (e->number() == QMidiEvent::TrackName)) {
			e->setTrack(1);
			ret->addEvent(e->tick(), e);
			continue;
		} else if (e->type() == QMidiEvent::Meta) {
			e->setTrack(0);
			ret->addEvent(e->tick(), e);
			continue;
		}
		if (!tracks.contains(e->voice())) {
			tracks.insert(e->voice(), ret->createTrack());
		}
		e->setTrack(tracks.value(e->voice()));
		ret->addEvent(e->tick(), e);
	}
	ret->fDisableSort = false;
	ret->sort();
	return ret;
}

bool isGreaterThan(QMidiEvent* e1, QMidiEvent* e2)
{
	qint32 e1t = e1->tick();
	qint32 e2t = e2->tick();
	return (e1t < e2t);
}
void QMidiFile::sort()
{
	if (fDisableSort) {
		return;
	}
	std::stable_sort(fEvents.begin(), fEvents.end(), isGreaterThan);
	std::stable_sort(fTempoEvents.begin(), fTempoEvents.end(), isGreaterThan);
}

void QMidiFile::addEvent(qint32 tick, QMidiEvent* e)
{
	e->setTick(tick);
	fEvents.append(e);
	if ((e->track() == 0) && (e->type() == QMidiEvent::Meta) && (e->number() == QMidiEvent::Tempo)) {
		fTempoEvents.append(e);
	}
	sort();
}
void QMidiFile::removeEvent(QMidiEvent* e)
{
	fEvents.removeOne(e);
	if ((e->track() == 0) && (e->type() == QMidiEvent::Meta) && (e->number() == QMidiEvent::Tempo)) {
		fTempoEvents.removeOne(e);
	}
}

QList<QMidiEvent*> QMidiFile::eventsForTrack(qint32 track)
{
	QList<QMidiEvent*> ret;
	for (QMidiEvent* e : fEvents) {
		if (e->track() == track) {
			ret.append(e);
		}
	}
	return ret;
}

QList<QMidiEvent*> QMidiFile::events(qint32 voice)
{
	QList<QMidiEvent*> ret;
	for (QMidiEvent* e : fEvents) {
		if (e->voice() == voice) {
			ret.append(e);
		}
	}
	return ret;
}

qint32 QMidiFile::createTrack()
{
	qint32 t = fTracks.count();
	fTracks.append(t);
	return t;
}

void QMidiFile::removeTrack(qint32 track)
{
	if (fTracks.contains(track)) {
		fTracks.removeOne(track);
	}
}

qint32 QMidiFile::trackEndTick(qint32 track)
{
	for (qint32 i = fEvents.size() - 1; i >= 0; i--) {
		QMidiEvent* e = fEvents.at(i);
		if (e->track() == track) {
			return e->tick();
		}
	}
	return 0;
}

QMidiEvent* QMidiFile::createNote(qint32 track, qint32 start_tick, qint32 end_tick, qint32 voice,
								  qint32 note, qint32 start_velocity, qint32 end_velocity)
{
	createNoteOffEvent(track, end_tick, voice, note, end_velocity);
	return createNoteOnEvent(track, start_tick, voice, note, start_velocity);
}

QMidiEvent* QMidiFile::createNoteOffEvent(qint32 track, qint32 tick, qint32 voice, qint32 note, qint32 velocity)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::NoteOff);
	e->setTrack(track);
	e->setVoice(voice);
	e->setNote(note);
	e->setVelocity(velocity);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createNoteOnEvent(qint32 track, qint32 tick, qint32 voice, qint32 note, qint32 velocity)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::NoteOn);
	e->setTrack(track);
	e->setVoice(voice);
	e->setNote(note);
	e->setVelocity(velocity);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createKeyPressureEvent(qint32 track, qint32 tick, qint32 voice, qint32 note,
											  qint32 amount)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::KeyPressure);
	e->setTrack(track);
	e->setVoice(voice);
	e->setNote(note);
	e->setAmount(amount);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createChannelPressureEvent(qint32 track, qint32 tick, qint32 voice, qint32 amount)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::ChannelPressure);
	e->setTrack(track);
	e->setVoice(voice);
	e->setAmount(amount);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createControlChangeEvent(qint32 track, qint32 tick, qint32 voice, qint32 number,
												qint32 value)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::ControlChange);
	e->setTrack(track);
	e->setVoice(voice);
	e->setNumber(number);
	e->setValue(value);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createProgramChangeEvent(qint32 track, qint32 tick, qint32 voice, qint32 number)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::ProgramChange);
	e->setTrack(track);
	e->setVoice(voice);
	e->setNumber(number);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createPitchWheelEvent(qint32 track, qint32 tick, qint32 voice, qint32 value)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::PitchWheel);
	e->setTrack(track);
	e->setVoice(voice);
	e->setValue(value);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createSysexEvent(qint32 track, qint32 tick, QByteArray data)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::SysEx);
	e->setTrack(track);
	e->setData(data);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createMetaEvent(qint32 track, qint32 tick, qint32 number, QByteArray data)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::Meta);
	e->setTrack(track);
	e->setNumber(number);
	e->setData(data);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createTempoEvent(qint32 track, qint32 tick, float tempo)
{
	qint64 midi_tempo = 60000000L / tempo;
	QByteArray buffer(3, 0);
	buffer[0] = (midi_tempo >> 16) & 0xFF;
	buffer[1] = (midi_tempo >> 8) & 0xFF;
	buffer[2] = midi_tempo & 0xFF;
	return createMetaEvent(track, tick, QMidiEvent::Tempo, buffer);
}
QMidiEvent* QMidiFile::createTimeSignatureEvent(qint32 track, qint32 tick, qint32 numerator,
												qint32 denominator)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::Meta);
	e->setNumber(QMidiEvent::TimeSignature);
	e->setTrack(track);
	e->setNumerator(numerator);
	e->setDenominator(denominator);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createLyricEvent(qint32 track, qint32 tick, QByteArray text)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::Meta);
	e->setNumber(QMidiEvent::Lyric);
	e->setTrack(track);
	e->setData(text);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createMarkerEvent(qint32 track, qint32 tick, QByteArray text)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setType(QMidiEvent::Meta);
	e->setNumber(QMidiEvent::Marker);
	e->setTrack(track);
	e->setData(text);
	addEvent(tick, e);
	return e;
}
QMidiEvent* QMidiFile::createVoiceEvent(qint32 track, qint32 tick, quint32 data)
{
	QMidiEvent* e = new (std::nothrow) QMidiEvent();
	Q_CHECK_PTR(e);

	e->setTrack(track);
	e->setMessage(data);
	addEvent(tick, e);
	return e;
}

float QMidiFile::timeFromTick(qint32 tick)
{
	switch (fDivType) {
	case PPQ: {
		float tempo_event_time = 0.0;
		qint32 tempo_event_tick = 0;
		float tempo = 120.0;

		for (QMidiEvent* e : fTempoEvents) {
			if (e->tick() >= tick) {
				break;
			}
			tempo_event_time +=
				(((float)(e->tick() - tempo_event_tick)) / fResolution / (tempo / 60));
			tempo_event_tick = e->tick();
			tempo = e->tempo();
		}

		float time =
			tempo_event_time + (((float)(tick - tempo_event_tick)) / fResolution / (tempo / 60));
		return time;
	}
	case SMPTE24:
		return (float)(tick) / (fResolution * 24.0);
	case SMPTE25:
		return (float)(tick) / (fResolution * 25.0);
	case SMPTE30DROP:
		return (float)(tick) / (fResolution * 29.97);
	case SMPTE30:
		return (float)(tick) / (fResolution * 30.0);
	default:
		return -1;
	}
}

qint32 QMidiFile::tickFromTime(float time)
{
	switch (fDivType) {
	case PPQ: {
		float tempo_event_time = 0.0;
		qint32 tempo_event_tick = 0;
		float tempo = 120.0;

		for (QMidiEvent* e : fTempoEvents) {
			float next_tempo_event_time =
				tempo_event_time +
				(((float)(e->tick() - tempo_event_tick)) / fResolution / (tempo / 60));
			if (next_tempo_event_time >= time) break;
			tempo_event_time = next_tempo_event_time;
			tempo_event_tick = e->tick();
			tempo = e->tempo();
		}

		return tempo_event_tick + (qint32)((time - tempo_event_time) * (tempo / 60) * fResolution);
	}
	case SMPTE24:
		return (qint32)(time * fResolution * 24.0);
	case SMPTE25:
		return (qint32)(time * fResolution * 25.0);
	case SMPTE30DROP:
		return (qint32)(time * fResolution * 29.97);
	case SMPTE30:
		return (qint32)(time * fResolution * 30.0);
	default:
		return -1;
	}
}

float QMidiFile::beatFromTick(qint32 tick)
{
	switch (fDivType) {
	case PPQ:
		return (float)(tick) / fResolution;
	case SMPTE24:
		return (float)(tick) / 24.0;
	case SMPTE25:
		return (float)(tick) / 25.0;
	case SMPTE30DROP:
		return (float)(tick) / 29.97;
	case SMPTE30:
		return (float)(tick) / 30.0;
	default:
		return -1.0;
	}
}

qint32 QMidiFile::tickFromBeat(float beat)
{
	switch (fDivType) {
	case PPQ:
		return (qint32)(beat * fResolution);
	case SMPTE24:
		return (qint32)(beat * 24.0);
	case SMPTE25:
		return (qint32)(beat * 25.0);
	case SMPTE30DROP:
		return (qint32)(beat * 29.97);
	case SMPTE30:
		return (qint32)(beat * 30);
	default:
		return -1;
	}
}

/*
 * Helpers
 */

qint16 interpret_int16(qint8* buffer)
{
	return ((qint16)(buffer[0]) << 8) | (qint16)(buffer[1]);
}
quint16 interpret_uint16(qint8* buffer)
{
	return ((quint16)(buffer[0]) << 8) | (quint16)(buffer[1]);
}
quint16 read_uint16(QFile* in)
{
	qint8 buffer[2];
	in->read((char*)buffer, 2);
	return interpret_uint16(buffer);
}
void write_uint16(QFile* out, quint16 value)
{
	qint8 buffer[2];
	buffer[0] = (qint8)((value >> 8) & 0xFF);
	buffer[1] = (qint8)(value & 0xFF);
	out->write((char*)buffer, 2);
}

quint32 read_uint32(QFile* in)
{
	qint8 buffer[4];
	in->read((char*)&buffer, 4);
	return ((quint32)(buffer[0]) << 24) | ((quint32)(buffer[1]) << 16) |
		   ((quint32)(buffer[2]) << 8) | (quint32)(buffer[3]);
}
void write_uint32(QFile* out, quint32 value)
{
	qint8 buffer[4];
	buffer[0] = (qint8)(value >> 24);
	buffer[1] = (qint8)((value >> 16) & 0xFF);
	buffer[2] = (qint8)((value >> 8) & 0xFF);
	buffer[3] = (qint8)(value & 0xFF);
	out->write((char*)buffer, 4);
}

quint32 read_variable_length_quantity(QFile* in)
{
	qint8 b;
	quint32 value = 0;

	do {
		in->getChar((char*)&b);
		value = (value << 7) | (b & 0x7F);
	} while ((b & 0x80) == 0x80 && !in->atEnd());

	return value;
}
void write_variable_length_quantity(QFile* out, quint32 value)
{
	qint8 buffer[4];
	qint8 offset = 3;

	forever {
		buffer[offset] = (qint8)(value & 0x7F);
		if (offset < 3) buffer[offset] |= 0x80;
		value >>= 7;
		if ((value == 0) || (offset == 0)) {
			break;
		}
		offset--;
	}

	out->write((char*)buffer + offset, 4 - offset);
}

bool QMidiFile::load(QString filename)
{
	clear();

	QFile in(filename);
	if (!in.exists() || !in.open(QFile::ReadOnly)) {
		return false;
	}

	fDisableSort = true;
	uchar chunk_id[4];
	qint8 division_type_and_resolution[2];
	qint64 chunk_size = 0, chunk_start = 0;
	quint16 file_format = 0, number_of_tracks = 0, number_of_tracks_read = 0;

	in.read((char*)chunk_id, 4);
	chunk_size = read_uint32(&in);
	chunk_start = in.pos();

	/* check for the RMID variation on SMF */

	if (memcmp(chunk_id, "RIFF", 4) == 0) {
		in.read((char*)chunk_id, 4);
		/* technically this one is a type id rather than a chunk id */

		if (memcmp(chunk_id, "RMID", 4) != 0) {
			in.close();
			fDisableSort = false;
			return false;
		}

		in.read((char*)chunk_id, 4);
		chunk_size = read_uint32(&in);

		if (memcmp(chunk_id, "data", 4) != 0) {
			in.close();
			fDisableSort = false;
			return false;
		}

		in.read((char*)chunk_id, 4);
		chunk_size = read_uint32(&in);
		chunk_start = in.pos();
	}

	if (memcmp(chunk_id, "MThd", 4) != 0) {
		in.close();
		fDisableSort = false;
		return false;
	}

	file_format = read_uint16(&in);
	number_of_tracks = read_uint16(&in);
	in.read((char*)division_type_and_resolution, 2);

	switch (division_type_and_resolution[0]) {
	case SMPTE24: {
		fFileFormat = file_format;
		fDivType = SMPTE24;
		fResolution = division_type_and_resolution[1];
		break;
	}
	case SMPTE25: {
		fFileFormat = file_format;
		fDivType = SMPTE25;
		fResolution = division_type_and_resolution[1];
		break;
	}
	case SMPTE30DROP: {
		fFileFormat = file_format;
		fDivType = SMPTE30DROP;
		fResolution = division_type_and_resolution[1];
		break;
	}
	case SMPTE30: {
		fFileFormat = file_format;
		fDivType = SMPTE30;
		fResolution = division_type_and_resolution[1];
		break;
	}
	default: {
		fFileFormat = file_format;
		fDivType = PPQ;
		fResolution = interpret_uint16(division_type_and_resolution);
		break;
	}
	}

	/* forwards compatibility:  skip over any extra header data */
	in.seek(chunk_start + chunk_size);

	while (number_of_tracks_read < number_of_tracks) {
		in.read((char*)chunk_id, 4);
		chunk_size = read_uint32(&in);
		chunk_start = in.pos();

		if (memcmp(chunk_id, "MTrk", 4) == 0) {
			qint32 track = createTrack();
			qint32 tick = 0, previous_tick = 0;
			qint64 previous_pos = 0;
			quint8 status, running_status = 0;
			bool at_end_of_track = false;

			while ((in.pos() < chunk_start + chunk_size) && !at_end_of_track) {
				tick = read_variable_length_quantity(&in) + previous_tick;
				previous_tick = tick;

				in.getChar((char*)&status);

				if ((status & 0x80) == 0x00) {
					status = running_status;
					in.seek(in.pos() - 1);
				} else {
					running_status = status;
				}

				if (in.pos() == previous_pos) {
					in.close();
					fDisableSort = false;
					sort();
					return false;
				}
				previous_pos = in.pos();

				switch (status & STATUS_CHANNEL_MASK) {
				case MessageType::NoteOff: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char note;
					in.getChar(&note);
					char velocity;
					in.getChar(&velocity);
					createNoteOffEvent(track, tick, voice, note, velocity);
					break;
				}
				case MessageType::NoteOn: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char note;
					in.getChar(&note);
					char velocity;
					in.getChar(&velocity);
					if (velocity != 0) {
						createNoteOnEvent(track, tick, voice, note, velocity);
					} else {
						createNoteOffEvent(track, tick, voice, note);
					}
					break;
				}
				case MessageType::PolyKeyPressure: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char note;
					in.getChar(&note);
					char amount;
					in.getChar(&amount);
					createKeyPressureEvent(track, tick, voice, note, amount);
					break;
				}
				case MessageType::ControlChange: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char number;
					in.getChar(&number);
					char value;
					in.getChar(&value);
					createControlChangeEvent(track, tick, voice, number, value);
					break;
				}
				case MessageType::ProgramChange: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char number;
					in.getChar(&number);
					createProgramChangeEvent(track, tick, voice, number);
					break;
				}
				case MessageType::ChannelPressure: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char amount;
					in.getChar(&amount);
					createChannelPressureEvent(track, tick, voice, amount);
					break;
				}
				case MessageType::PitchBend: {
					qint32 voice = status & STATUS_VOICE_MASK;
					char value;
					in.getChar(&value);
					char value2;
					in.getChar(&value2);

					qint16 pitch;
					pitch = ((value2 & 0x7F) << 7) | (value & 0x7F); // Unpack 14-bit value

					createPitchWheelEvent(track, tick, voice, pitch);
					break;
				}
				case MessageType::SystemExclusive: {
					switch (status) {
					case SystemExclusiveStart:
					case SystemExclusiveEnd: {
						qint32 data_length = read_variable_length_quantity(&in) + 1;
						QByteArray data(1, 0);
						data[0] = status;
						data += in.read(data_length - 1);

						createSysexEvent(track, tick, data);
						break;
					}
					case 0xFF: {
						char number;
						in.getChar(&number);
						qint32 data_length = read_variable_length_quantity(&in);
						QByteArray data = in.read(data_length);

						if (number == 0x2F) {
							at_end_of_track = true;
						} else {
							createMetaEvent(track, tick, number, data);
						}
						break;
					}
					}

					break;
				}
				}
			}

			number_of_tracks_read++;
		} else {
			in.close();
			fDisableSort = false;
			sort();
			return false;
		}

		/* forwards compatibility: skip over any unrecognized chunks, or extra
		 * data at the end of tracks. */
		in.seek(chunk_start + chunk_size);
	}

	in.close();
	fDisableSort = false;
	sort();
	return true;
}

bool QMidiFile::save(QString filename)
{
	if (filename.isEmpty()) {
		return false;
	}

	QFile out(filename);

	if (out.exists()) {
		out.remove();
	}

	if (!out.open(QFile::WriteOnly)) {
		return false;
	}

	out.write("MThd", 4);
	write_uint32(&out, 6);
	write_uint16(&out, (quint16)(fFileFormat));
	write_uint16(&out, (quint16)(fTracks.count()));

	switch (fDivType) {
	case PPQ:
		write_uint16(&out, (quint16)(fResolution));
		break;
	default:
		out.putChar(fDivType);
		out.putChar(fResolution);
		break;
	}

	quint32 track_size_offset, track_start_offset, track_end_offset;
	quint32 tick, previous_tick;
	for (qint32 curTrack : fTracks) {
		track_size_offset = track_start_offset = track_end_offset = 0;
		tick = previous_tick = 0;

		out.write("MTrk", 4);

		track_size_offset = (quint32)out.pos();
		write_uint32(&out, 0);

		track_start_offset = (quint32)out.pos();

		QList<QMidiEvent*> eventsForTrk = eventsForTrack(curTrack);
		for (QMidiEvent* e : eventsForTrk) {
			tick = e->tick();
			write_variable_length_quantity(&out, tick - previous_tick);

			switch (e->type()) {
			case QMidiEvent::NoteOff:
				out.putChar(MessageType::NoteOff
					| (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->note() & 0x7F);
				out.putChar(e->velocity() & 0x7F);
				break;

			case QMidiEvent::NoteOn:
				out.putChar(MessageType::NoteOn | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->note() & 0x7F);
				out.putChar(e->velocity() & 0x7F);
				break;

			case QMidiEvent::KeyPressure:
				out.putChar(MessageType::PolyKeyPressure | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->note() & 0x7F);
				out.putChar(e->amount() & 0x7F);
				break;

			case QMidiEvent::ControlChange:
				out.putChar(MessageType::ControlChange | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->number() & 0x7F);
				out.putChar(e->value() & 0x7F);
				break;

			case QMidiEvent::ProgramChange:
				out.putChar(MessageType::ProgramChange | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->number() & 0x7F);
				break;

			case QMidiEvent::ChannelPressure:
				out.putChar(MessageType::ChannelPressure | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(e->value() & 0x7F);
				break;

			case QMidiEvent::PitchWheel: {
				const qint32 value = e->value();
				out.putChar(MessageType::PitchBend | (e->voice() & STATUS_VOICE_MASK));
				out.putChar(value & 0x7F);
				out.putChar((value >> 7) & 0x7F);
				break;
			}
			case QMidiEvent::SysEx: {
				const qint32 data_length = e->data().length();
				const qint8* data = (qint8*)e->data().constData();
				out.putChar(data[0]);
				write_variable_length_quantity(&out, data_length - 1);
				out.write((char*)data + 1, data_length - 1);
				break;
			}
			case QMidiEvent::Meta: {
				const qint32 data_length = e->data().length();
				const qint8* data = (qint8*)e->data().constData();
				out.putChar(0xFF);
				out.putChar(e->number() & 0x7F);
				write_variable_length_quantity(&out, data_length);
				out.write((char*)data, data_length);
				break;
			}
			default:
				break;
			}

			previous_tick = tick;
		}

		write_variable_length_quantity(&out, trackEndTick(curTrack) - previous_tick);
		out.write("\xFF\x2F\x00", 3);

		track_end_offset = (quint32)out.pos();

		out.seek(track_size_offset);
		write_uint32(&out, track_end_offset - track_start_offset);

		out.seek(track_end_offset);
	}

	out.close();
	return true;
}
