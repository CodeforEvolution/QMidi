/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QMIDIENDPOINT_H
#define QMIDIENDPOINT_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QMidiEndpoint : public QObject
{
	Q_OBJECT

public:
	QMidiEndpoint(QObject* parent = nullptr);
	~QMidiEndpoint();


};

QT_END_NAMESPACE


#endif	// QMIDIENDPOINT_H
