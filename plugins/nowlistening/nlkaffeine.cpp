/*
    nlkaffeine.cpp

    Kopete Now Listening To plugin

    Copyright (c) 2004 by Will Stephenson <will@stevello.free-online.co.uk>
    Kopete
	Copyright (c) 2002,2003,2004 by the Kopete developers  <kopete-devel@kde.org>
	
	Purpose: 
	This class abstracts the interface to Kaffeine by
	implementing NLMediaPlayer

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <QString>

#include "nlmediaplayer.h"
#include "nlkaffeine.h"

NLKaffeine::NLKaffeine( DCOPClient *client ) : NLMediaPlayer()
{
	m_client = client;
	m_type = Video;
	m_name = "Kaffeine";
}

void NLKaffeine::update()
{
	m_playing = false;
	m_newTrack = false;
	QString newTrack;
	bool error = true; // Asume we have a error first. 
	DCOPCString kaffeineIface("Kaffeine"), kaffeineGetTrack("getTitle()");

	// see if kaffeine is  registered with DCOP
	if ( m_client->isApplicationRegistered( DCOPCString("kaffeine") ) )
	{
		// see if it's playing
		QByteArray data, replyData;
		DCOPCString replyType;
		QString result;
		if ( !m_client->call( DCOPCString("kaffeine"), kaffeineIface, DCOPCString("isPlaying()"), data,
					replyType, replyData ) )
		{
			kDebug ( 14307 ) << k_funcinfo << " Trying DCOP interface of Kaffeine >= 0.5" << endl;
			// Trying with the new Kaffeine DCOP interface (>=0.5)
			kaffeineIface = "KaffeineIface";
			kaffeineGetTrack = "title()";
			if( !m_client->call( DCOPCString("kaffeine"), kaffeineIface, DCOPCString("isPlaying()"), data, replyType, replyData ) )
			{
				kDebug( 14307 ) << k_funcinfo << " DCOP error on Kaffeine." << endl;
			}
			else
			{
				error = false;
			}
		}
		else
		{
			error = false;
		}
			
		// If we didn't get any DCOP error, check if Kaffeine is playing.
		if(!error)
		{
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "bool" ) {
					reply >> m_playing;
					kDebug( 14307 ) << "checked if Kaffeine is playing!" << endl;
			}
		}

		if ( m_client->call( DCOPCString("kaffeine"), kaffeineIface, kaffeineGetTrack, data,
					replyType, replyData ) )
		{
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);

			if ( replyType == "QString" ) {
				reply >> newTrack;
			}
		}
		if( newTrack != m_track )
		{
			m_newTrack = true;
			m_track = newTrack;
		}
	}
	else
		kDebug ( 14307 ) << "Kaffeine is not running!\n" << endl;
}

