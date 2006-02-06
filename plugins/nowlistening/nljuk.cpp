/*
    nljuk.cpp

    Kopete Now Listening To plugin

    Copyright (c) 2002,2003,2004 by Will Stephenson <will@stevello.free-online.co.uk>
    Copyright (c) 2003 by Ismail Donmez <ismail.donmez@boun.edu.tr>
    Copyright (c) 2002,2003 by the Kopete developers  <kopete-devel@kde.org>
	
	Purpose: 
	This class abstracts the interface to JuK by
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
#include "nljuk.h"

NLJuk::NLJuk( DCOPClient *client ) : NLMediaPlayer()
{
	m_client = client;
	m_type = Audio;
	m_name = "JuK";
}

void NLJuk::update()
{
	m_playing = false;
	QString newTrack;

	// see if JuK is  registered with DCOP
	if (  m_client->isApplicationRegistered( "juk" ) )
	{
		// see if it's playing
		QByteArray data, replyData;
		DCOPCString replyType;
		QString result;

		if ( m_client->call( DCOPCString("juk"), DCOPCString("Player"), DCOPCString("playing()"), data, 
					replyType, replyData ) )
		{
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "bool" ) {
				reply >> m_playing;
			}
		}
		
		{
			QDataStream arg( &data,QIODevice::WriteOnly );
			arg.setVersion(QDataStream::Qt_3_1);
			arg << QString::fromLatin1("Album");
			if ( m_client->call( "juk", "Player", "trackProperty(QString)", data,
						replyType, replyData ) )
			{
				QDataStream reply( &replyData,QIODevice::ReadOnly );
				reply.setVersion(QDataStream::Qt_3_1);
	
				if ( replyType == "QString" ) {
					reply >> m_album;
				}
			}
		}
		
		{
			QDataStream arg( &data,QIODevice::WriteOnly );
			arg.setVersion(QDataStream::Qt_3_1);
			arg << QString::fromLatin1("Artist");
			if ( m_client->call( "juk", "Player", "trackProperty(QString)", data,
						replyType, replyData ) )
			{
				QDataStream reply( &replyData,QIODevice::ReadOnly );
				reply.setVersion(QDataStream::Qt_3_1);
	
				if ( replyType == "QString" ) {
					reply >> m_artist;
				}
			}
		}

		{
			QDataStream arg( &data,QIODevice::WriteOnly );
			arg.setVersion(QDataStream::Qt_3_1);
			arg << QString::fromLatin1("Title");
			if ( m_client->call( "juk", "Player", "trackProperty(QString)", data,
						replyType, replyData ) )
			{
				QDataStream reply( &replyData,QIODevice::ReadOnly );
				reply.setVersion(QDataStream::Qt_3_1);
	
				if ( replyType == "QString" ) {
					reply >> newTrack;
				}
			}
		}
		
		if ( newTrack != m_track )
		{
			m_newTrack = true;
			m_track = newTrack;
		}
		else
			m_newTrack = false;
	}
	else
		kDebug( 14307 ) << "Juk is not running!\n" << endl;
}

