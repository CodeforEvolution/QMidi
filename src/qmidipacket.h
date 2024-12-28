/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIPACKET_H
#define QMIDIPACKET_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QMidiPacket : public QObject
{
	Q_OBJECT

public:
	QMidiPacket(QObject* parent = nullptr);
	~QMidiPacket();
};

QT_END_NAMESPACE


#endif	// QMIDIPACKET_H
