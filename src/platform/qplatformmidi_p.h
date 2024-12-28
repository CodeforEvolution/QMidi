/*
 * Copyright 2024 Jacob Secunda <CodeforEvolution>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef QPLATFORMMIDI_H
#define QPLATFORMMIDI_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QPlatformMidi
{
	explicit QPlatformMidi(QMidiDevice *frontendDevice) : device(frontendDevice) { }
	virtual ~QPlatformMidi() = default;

};

QT_END_NAMESPACE

#endif	// QPLATFORMMIDI_H
