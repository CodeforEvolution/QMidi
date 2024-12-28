/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIROSTER_H
#define QMIDIROSTER_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QMidiRoster : public QObject
{
    Q_OBJECT
	Q_PROPERTY(QList<QMidiDevice> midiDevices READ midiDevices NOTIFY midiDevicesChanged)
    Q_PROPERTY(QList<QMidiEndpoint> midiInputs READ midiInputs NOTIFY midiInputsChanged)
	Q_PROPERTY(QList<QMidiEndpoint> midiOutputs READ midiOutputs NOTIFY midiOutputsChanged)

public:
	QMidiRoster(QObject *parent = nullptr);
    ~QMidiRoster();

	static QList<QMidiDevice> midiDevices();
	static QList<QMidiEndpoint> midiInputs();
	static QList<QMidiEndpoint> midiOutputs();

	static QMidiEndpoint defaultMidiInput();
	static QMidiEndpoint defaultMidiOutput();

Q_SIGNALS:
	void midiDevicesChanged;
	void midiInputsChanged;
	void midiOutputsChanged;

protected:
	void connectNotify(const QMetaMethod &signal) override;
};

QT_END_NAMESPACE


#endif  // QMIDIROSTER_H
