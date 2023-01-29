/*
 * Copyright 2019 Georg Gadinger <nilsding@nilsding.org>
 * Distributed under the terms of the MIT license.
 */
#include "QMidiIn.h"

QMidiIn::QMidiIn(QObject *parent)
	: QObject(parent),
	fMidiPtrs(Q_NULLPTR),
	fConnected(false)
{
}

QMidiIn::~QMidiIn()
{
	if (fConnected)
		disconnect();
}
