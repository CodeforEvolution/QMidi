/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIOUTPUT_H
#define QMIDIOUTPUT_H

#include <QtCore/qobject.h>

#include <QMidi/qmidiendpoint.h>

QT_BEGIN_NAMESPACE

class QMidiOutput : public QMidiEndpoint
{
	Q_OBJECT

public:
	QMidiOutput(QObject* parent = nullptr);
	~QMidiOutput();


};

QT_END_NAMESPACE


#endif	// QMIDIOUTPUT_H
