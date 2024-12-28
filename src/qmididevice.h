/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIDEVICE_H
#define QMIDIDEVICE_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QMidiDevice : public QObject
{
	Q_OBJECT

public:
	QMidiDevice(QObject* parent = nullptr);
	~QMidiDevice();


};

QT_END_NAMESPACE


#endif	// QMIDIDEVICE_H
