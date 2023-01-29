/*
 * Copyright 2023 Jacob Secunda <secundaja@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "QMidiOut.h"
#include "QMidiIn.h"

#include <Availability.h>
#include <CoreAudio/HostTime.h>
#include <CoreMIDI/CoreMIDI.h>

#include <QCoreApplication>

#pragma mark - globals

static dispatch_once_t sJustThisOneTime = 0;
static MIDIClientRef sMidiClient = 0;

static void _initQMidiClient(void* context)
{
    auto result = static_cast<OSStatus*>(context);

    QString clientName = QCoreApplication::applicationName();
    *result = MIDIClientCreate(clientName.toCFString(), Q_NULLPTR, Q_NULLPTR,
                              &sMidiClient);
}


#pragma mark - QMidiOut

struct NativeMidiOutInstances {
	MIDIPortRef outputPort;
	MIDIEndpointRef destinationId;
};


static void _MIDISysexCompleted(MIDISysexSendRequest *request)
{
    delete request;
}


// TODO: error reporting
QMap<QString, QString> QMidiOut::devices()
{
	QMap<QString, QString> ret;

	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
	quint32 destinations = MIDIGetNumberOfDestinations();
	for (quint32 destIndex = 0; destIndex <= destinations; destIndex++) {
		MIDIEndpointRef destRef = MIDIGetDestination(destIndex);
		if (destRef != 0) {
			CFStringRef nameRef;

  			MIDIObjectGetStringProperty(destRef,
				kMIDIPropertyDisplayName, &nameRef);

			ret.insert(QString::number(destIndex),
				QString::fromCFString(nameRef));

			CFRelease(nameRef);
		}
	}

	return ret;
}


bool QMidiOut::connect(QString outDeviceId)
{
    OSStatus result = noErr;

	dispatch_once_f(&sJustThisOneTime, &result, _initQMidiClient);
    if (result != noErr)
        return false;

	if (fConnected)
		disconnect();

    fMidiPtrs = new(std::nothrow) NativeMidiOutInstances;
    Q_CHECK_PTR(fMidiPtrs);

    fMidiPtrs->destinationId = MIDIGetDestination(outDeviceId.toUInt());
    if (fMidiPtrs->destinationId == 0)
        return false;

    QString portName;
    CFStringRef destName;

    result = MIDIObjectGetStringProperty(fMidiPtrs->destinationId, kMIDIPropertyDisplayName, &destName);
    if (result == noErr) {
        portName = QString::fromCFString(destName);
        CFRelease(destName);
    }

    if (portName.isEmpty())
        portName = "Device " + outDeviceId;

	result = MIDIOutputPortCreate(sMidiClient, portName.toCFString(), &fMidiPtrs->outputPort);
	if (result != noErr) {
        MIDIEndpointDispose(fMidiPtrs->destinationId);
        return false;
    }

	fDeviceId = outDeviceId;
	fConnected = true;
	return true;
}


void QMidiOut::disconnect()
{
	if (!fConnected)
		return;

	if (fMidiPtrs->destinationId != 0) {
		MIDIEndpointDispose(fMidiPtrs->destinationId);
		fMidiPtrs->destinationId = 0;
	}

	if (fMidiPtrs->outputPort != 0) {
		MIDIPortDispose(fMidiPtrs->outputPort);
		fMidiPtrs->outputPort = 0;
	}

	fConnected = false;

	delete fMidiPtrs;
	fMidiPtrs = Q_NULLPTR;
}


void QMidiOut::sendMsg(qint32 msg)
{
	if (!fConnected)
		return;

	MIDIPacketList packetList;
	MIDIPacket *packet = MIDIPacketListInit(&packetList);

	MIDITimeStamp timeStamp = AudioGetCurrentHostTime();
	packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
		timeStamp, sizeof(msg), (Byte*)&msg);
    if (packet == Q_NULLPTR)
        return;

	MIDISend(fMidiPtrs->outputPort, fMidiPtrs->destinationId, &packetList);
}


void QMidiOut::sendSysEx(const QByteArray &data)
{
	if (!fConnected)
		return;

	MIDISysexSendRequest* request = new(std::nothrow) MIDISysexSendRequest;
    Q_CHECK_PTR(request);

	request->bytesToSend = data.length();
	request->complete = false;
	request->completionProc = _MIDISysexCompleted;
	request->completionRefCon = Q_NULLPTR;
	request->data = reinterpret_cast<const Byte *>(data.constData());
	request->destination = fMidiPtrs->destinationId;

	MIDISendSysex(request);
}


# pragma mark - QMidiIn

struct NativeMidiInInstances {
	MIDIPortRef inputPort;
	MIDIEndpointRef sourceId;
};

static void QMidiInReadProc(const MIDIPacketList *list, void *readProc,
	void *srcConn)
{
	Q_UNUSED(srcConn)
	QMidiIn *midiIn = static_cast<QMidiIn *>(readProc);
	MIDIPacket *packet = const_cast<MIDIPacket *>(list->packet);

	for (quint32 index = 0; index < list->numPackets; index++) {
        if (packet == Q_NULLPTR)
            return;

		quint16 byteCount = packet->length;

		// Check that MIDIPacket has data in 3-byte groups
		if (byteCount != 0 && (byteCount % 3) == 0) {
			// We need to break apart the data into 3-byte messages
			for (int i = 0; i < byteCount; i += 3) {
				// Make sure that it's a normal MIDI message. SysEx etc.
				// are not supported at the moment.
				if ((packet->data[i] < 0xF0) && (packet->data[i] & 0x80)) {
					const quint32 msg =   (packet->data[i])
										| (packet->data[i + 1] << 8)
										| (packet->data[i + 2] << 16);
					emit midiIn->midiEvent(msg, packet->timeStamp);
				}
			}
    	}

		packet = MIDIPacketNext(packet);
	}
}

QMap<QString, QString> QMidiIn::devices()
{
	QMap<QString, QString> deviceMap;

	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
	quint32 sources = MIDIGetNumberOfSources();
	for (quint32 sourceIndex = 0; sourceIndex <= sources; sourceIndex++) {
		MIDIEndpointRef sourceRef = MIDIGetSource(sourceIndex);
		if (sourceRef != 0) {
            CFStringRef nameRef = Q_NULLPTR;
            MIDIObjectGetStringProperty(sourceRef,kMIDIPropertyDisplayName, &nameRef);

            deviceMap.insert(QString::number(sourceIndex),
                             QString::fromCFString(nameRef));

            CFRelease(nameRef);
		}
	}

	return deviceMap;
}


bool QMidiIn::connect(QString inDeviceId)
{
    OSStatus result = noErr;
	dispatch_once_f(&sJustThisOneTime, &result, _initQMidiClient);
    if (result != noErr)
        return false;

	if (fConnected)
		disconnect();

	fMidiPtrs = new(std::nothrow) NativeMidiInInstances;
    Q_CHECK_PTR(fMidiPtrs);

    CFStringRef portNameRef = QString("Input Port " + inDeviceId).toCFString();

    if (__builtin_available(macOS 11.0, *)) {
        MIDIReceiveBlock receiveBlock = ^ (const MIDIEventList* eventList, void* srcConnRefCon) {
            // We only support MIDI 1.0 protocol messages for now.
            // TODO: Add support for handling MIDI 2.0 protocol messages.
            if (eventList->protocol != kMIDIProtocol_1_0)
                return;

            auto packet = const_cast<MIDIEventPacket *>(eventList->packet);
            auto midiIn = static_cast<QMidiIn *>(srcConnRefCon);

            // Let's read out the Universal Midi Packets!
            for (quint32 index = 0; index < eventList->numPackets; index++) {
                if (packet == Q_NULLPTR)
                    return;

                for (quint32 wordIndex = 0; wordIndex < packet->wordCount; wordIndex++) {
                    quint32 word = packet->words[wordIndex];
                    quint8 messageType = word >> 0xCF;
                    switch (messageType) {
                        case kMIDIMessageTypeChannelVoice1:
                        {

                        }
                        default:
                            break;

                    }


                }


                packet = MIDIEventPacketNext(packet);
            }
        };

        result = MIDIInputPortCreateWithProtocol(sMidiClient, portNameRef,
                                                 kMIDIProtocol_1_0, &fMidiPtrs->inputPort,
                                                 receiveBlock);
    } else {
        MIDIReadBlock readBlock = ^ (const MIDIPacketList* packetList, void* srcConnRefCon) {
            auto packet = const_cast<MIDIPacket *>(packetList->packet);
            auto midiIn = static_cast<QMidiIn *>(srcConnRefCon);

            for (quint32 index = 0; index < packetList->numPackets; index++) {
                if (packet == Q_NULLPTR)
                    return;

                quint16 byteCount = packet->length;

                // Check that MIDIPacket has data in 3-byte groups
                if (byteCount != 0 && (byteCount % 3) == 0) {
                    // We need to break apart the data into 3-byte messages
                    for (int i = 0; i < byteCount; i += 3) {
                        // Make sure that it's a normal MIDI message.
                        // TODO: SysEx, etc. are not supported at the moment.
                        if ((packet->data[i] < 0xF0) && (packet->data[i] & 0x80)) {
                            const quint32 msg =   (packet->data[i])
                                                  | (packet->data[i + 1] << 8)
                                                  | (packet->data[i + 2] << 16);
                            emit midiIn->midiEvent(msg, packet->timeStamp);
                        }
                    }
                }

                packet = MIDIPacketNext(packet);
            }
        };

        result = MIDIInputPortCreateWithBlock(sMidiClient, portNameRef, &fMidiPtrs->inputPort,
                                              readBlock);
    }

    CFRelease(portNameRef);

	if (result != noErr)
		return false;

	fMidiPtrs->sourceId = MIDIGetSource(inDeviceId.toUInt());
	if (fMidiPtrs->sourceId == 0) {
		MIDIPortDispose(fMidiPtrs->inputPort);
		return false;
	}

	fDeviceId = inDeviceId;
	fConnected = true;
	return true;
}


void QMidiIn::disconnect()
{
	if (!fConnected)
		return;

	MIDIPortDisconnectSource(fMidiPtrs->inputPort, fMidiPtrs->sourceId);

	if (fMidiPtrs->inputPort != 0) {
		MIDIPortDispose(fMidiPtrs->inputPort);
		fMidiPtrs->inputPort = 0;
	}

	fConnected = false;

	delete fMidiPtrs;
	fMidiPtrs = Q_NULLPTR;
}


void QMidiIn::start()
{
	if (!fConnected)
		return;

	MIDIPortConnectSource(fMidiPtrs->inputPort, fMidiPtrs->sourceId,this);
}


void QMidiIn::stop()
{
	if (!fConnected)
		return;

	MIDIPortDisconnectSource(fMidiPtrs->inputPort, fMidiPtrs->sourceId);
}
