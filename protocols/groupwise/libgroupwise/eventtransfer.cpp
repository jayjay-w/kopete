//
// C++ Implementation: eventtransfer
//
// Description: 
//
//
// Author: Kopete Developers <kopete-devel@kde.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "eventtransfer.h"

EventTransfer::EventTransfer( const Q_UINT32 eventType, const QString & source, QDateTime timeStamp, const QByteArray & payload )
 : Transfer(), m_eventType( eventType ), m_source( source ), m_timeStamp( timeStamp ), m_payload( payload )
{
}


EventTransfer::~EventTransfer()
{
}

int EventTransfer::event()
{ 
	return m_eventType;
}

QString EventTransfer::source()
{
	return m_source;
}

QDateTime EventTransfer::timeStamp()
{
	return m_timeStamp;
}

QByteArray EventTransfer::payload()
{
	return m_payload;
}
