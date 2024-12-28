/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIINPUT_H
#define QMIDIINPUT_H

#include <QtCore/qobject.h>

#include <QMidi/qmidiendpoint.h>

QT_BEGIN_NAMESPACE

class QMidiInput : public QMidiEndpoint
{
	Q_OBJECT

public:
	QMidiInput(QObject* parent = nullptr);
	~QMidiInput();


};

QT_END_NAMESPACE


#endif	// QMIDIINPUT_H
