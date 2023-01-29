/*
 * Copyright 2003-2012 by David G. Slomin
 * Copyright 2012-2015 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <QString>
#include <QMap>
#include <QList>

class QMidiEvent
{
public:
	enum EventType {
		Invalid = -1,
		NoteOn,
		NoteOff,
		KeyPressure,
		ChannelPressure,
		ControlChange,
		ProgramChange,
		PitchWheel,
		Meta,
		SysEx
	};
	enum MetaNumbers {
		/* These types match the MIDI values for them.
		 * DON'T CHANGE OR YOU WON'T BE ABLE TO READ/WRITE FILES! */
		TrackName = 0x03,
		Tempo = 0x51,
		TimeSignature = 0x58,
		Lyric = 0x5,
		Marker = 0x6
	};

	QMidiEvent();
	~QMidiEvent();

	inline EventType type() const { return fType; }
	inline void setType(EventType newType) { fType = newType; }

	inline qint32 tick() { return fTick; }
	inline void setTick(qint32 tick) { fTick = tick; }
	/* you MUST run the QMidiFile's sort() function after changing ticks! */
	/* otherwise, it will not play or write the file properly! */

	inline qint32 track() { return fTrackNumber; }
	inline void setTrack(qint32 trackNumber) { fTrackNumber = trackNumber; }

	inline qint32 voice() { return fVoice; }
	inline void setVoice(qint32 voice) { fVoice = voice; }

	inline qint32 note() { return fNote; }
	inline void setNote(qint32 note) { fNote = note; }

	inline qint32 velocity() { return fVelocity; }
	inline void setVelocity(qint32 velocity) { fVelocity = velocity; }

	inline qint32 amount() { return fAmount; }
	inline void setAmount(qint32 amount) { fAmount = amount; }

	inline qint32 number() { return fNumber; }
	inline void setNumber(qint32 number) { fNumber = number; }

	inline qint32 value() { return fValue; }
	inline void setValue(qint32 value) { fValue = value; }

	float tempo();

	inline qint32 numerator() { return fNumerator; }
	inline void setNumerator(qint32 numerator) { fNumerator = numerator; }

	inline qint32 denominator() { return fDenominator; }
	inline void setDenominator(qint32 denominator) { fDenominator = denominator; }

	inline QByteArray data() const { return fData; }
	inline void setData(QByteArray data) { fData = data; }

	quint32 message() const;
	void setMessage(quint32 data);

	inline bool isNoteEvent() { return ((fType == NoteOn) || (fType == NoteOff)); }

private:
	qint32 fVoice;
	qint32 fNote;
	qint32 fVelocity;
	qint32 fAmount;	// KeyPressure, ChannelPressure
	qint32 fNumber;	// ControlChange, ProgramChange, Meta
	qint32 fValue;		// PitchWheel, ControlChange
	qint32 fNumerator; // TimeSignature
	qint32 fDenominator; // TimeSignature
	QByteArray fData; // Meta, SysEx

	qint32 fTick;
	EventType fType;
	qint32 fTrackNumber;
};

class QMidiFile
{
public:
	enum DivisionType {
		/* These types match the MIDI values for them.
		 * DON'T CHANGE OR YOU WON'T BE ABLE TO READ/WRITE FILES! */
		Invalid = -1,
		PPQ = 0,
		SMPTE24 = -24,
		SMPTE25 = -25,
		SMPTE30DROP = -29,
		SMPTE30 = -30
	};

	QMidiFile();
	~QMidiFile();

	void clear();
	bool load(QString filename);
	bool save(QString filename);

	QMidiFile* oneTrackPerVoice();

	void sort();

	inline void setFileFormat(qint32 fileFormat) { fFileFormat = fileFormat; }
	inline qint32 fileFormat() { return fFileFormat; }

	inline void setResolution(qint32 resolution) { fResolution = resolution; }
	inline qint32 resolution() { return fResolution; }

	inline void setDivisionType(DivisionType type) { fDivType = type; }
	inline DivisionType divisionType() { return fDivType; }

	void addEvent(qint32 tick, QMidiEvent* e);
	void removeEvent(QMidiEvent* e);

	qint32 createTrack();
	void removeTrack(qint32 track);
	qint32 trackEndTick(qint32 track);
	inline QList<qint32> tracks() { return fTracks; }

	QMidiEvent* createNoteOnEvent(qint32 track, qint32 tick, qint32 voice, qint32 note, qint32 velocity);
	QMidiEvent* createNoteOffEvent(qint32 track, qint32 tick, qint32 voice, qint32 note, qint32 velocity = 64);
	/* velocity on NoteOff events is how fast to stop the note (127=fastest) */

	QMidiEvent* createNote(qint32 track, qint32 start_tick, qint32 end_tick, qint32 voice, qint32 note,
						   qint32 start_velocity, qint32 end_velocity);
	/* returns the start event */

	QMidiEvent* createKeyPressureEvent(qint32 track, qint32 tick, qint32 voice, qint32 note, qint32 amount);
	QMidiEvent* createChannelPressureEvent(qint32 track, qint32 tick, qint32 voice, qint32 amount);
	QMidiEvent* createControlChangeEvent(qint32 track, qint32 tick, qint32 voice, qint32 number, qint32 value);
	QMidiEvent* createProgramChangeEvent(qint32 track, qint32 tick, qint32 voice, qint32 number);
	QMidiEvent* createPitchWheelEvent(qint32 track, qint32 tick, qint32 voice, qint32 value);
	QMidiEvent* createSysexEvent(qint32 track, qint32 tick, QByteArray data);
	QMidiEvent* createMetaEvent(qint32 track, qint32 tick, qint32 number, QByteArray data);
	QMidiEvent* createTempoEvent(qint32 track, qint32 tick, float tempo); /* tempo is in BPM */
	QMidiEvent* createTimeSignatureEvent(qint32 track, qint32 tick, qint32 numerator, qint32 denominator);
	QMidiEvent* createLyricEvent(qint32 track, qint32 tick, QByteArray text);
	QMidiEvent* createMarkerEvent(qint32 track, qint32 tick, QByteArray text);
	QMidiEvent* createVoiceEvent(qint32 track, qint32 tick, quint32 data);

	inline QList<QMidiEvent*> events() { return QList<QMidiEvent*>(fEvents); }
	QList<QMidiEvent*> events(qint32 voice);
	QList<QMidiEvent*> eventsForTrack(qint32 track);

	float timeFromTick(qint32 tick); /* time is in seconds */
	qint32 tickFromTime(float time);
	float beatFromTick(qint32 tick);
	qint32 tickFromBeat(float beat);

private:
	QList<QMidiEvent*> fEvents;
	QList<QMidiEvent*> fTempoEvents;
	QList<qint32> fTracks;
	DivisionType fDivType;
	qint32 fResolution;
	qint32 fFileFormat;

	bool fDisableSort;
};
