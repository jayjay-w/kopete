/*
  oscarcontact.cpp  -  Oscar Protocol Plugin

  Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
  Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
*/

#include "oscarcontact.h"

#include <time.h>

#include <qapplication.h>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kdeversion.h>

#include "kopeteaccount.h"
#include "kopetechatsessionmanager.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetegroup.h"
#include "kopeteuiglobal.h"
#include <kopeteglobal.h>

#include "oscaraccount.h"
#include "client.h"
#include "ssimanager.h"
#include "oscarutils.h"

#include <assert.h>

OscarContact::OscarContact( Kopete::Account* account, const QString& name,
                            Kopete::MetaContact* parent, const QString& icon, const SSI& ssiItem )
: Kopete::Contact( account, name, parent, icon )
{
	mAccount = static_cast<OscarAccount*>(account);
	mName = name;
	mMsgManager = 0L;
	m_ssiItem = ssiItem;
	connect( this, SIGNAL( updatedSSI() ), this, SLOT( updateSSIItem() ) );
}

OscarContact::~OscarContact()
{
}

void OscarContact::serialize(QMap<QString, QString> &serializedData,
                             QMap<QString, QString> &/*addressBookData*/)
{
	serializedData["ssi_name"] = m_ssiItem.name();
	serializedData["ssi_type"] = QString::number( m_ssiItem.type() );
	serializedData["ssi_gid"] = QString::number( m_ssiItem.gid() );
	serializedData["ssi_bid"] = QString::number( m_ssiItem.bid() );
	serializedData["ssi_alias"] = m_ssiItem.alias();
	serializedData["ssi_waitingAuth"] = m_ssiItem.waitingAuth() ? QString::fromLatin1( "true" ) : QString::fromLatin1( "false" );
}

bool OscarContact::isOnServer() const
{
	return ( m_ssiItem.type() != 0xFFFF );
}

void OscarContact::setSSIItem( const Oscar::SSI& ssiItem )
{
	m_ssiItem = ssiItem;
	emit updatedSSI();
}

Oscar::SSI OscarContact::ssiItem() const
{
	return m_ssiItem;
}

Kopete::ChatSession* OscarContact::manager( CanCreateFlags canCreate )
{
	if ( !mMsgManager && canCreate )
	{
		/*kdDebug(14190) << k_funcinfo <<
			"Creating new ChatSession for contact '" << displayName() << "'" << endl;*/

		QPtrList<Kopete::Contact> theContact;
		theContact.append(this);

		mMsgManager = Kopete::ChatSessionManager::self()->create(account()->myself(), theContact, protocol());

		// This is for when the user types a message and presses send
		connect(mMsgManager, SIGNAL( messageSent( Kopete::Message&, Kopete::ChatSession * ) ),
		        this, SLOT( slotSendMsg( Kopete::Message&, Kopete::ChatSession * ) ) );

		// For when the message manager is destroyed
		connect(mMsgManager, SIGNAL( destroyed() ),
		        this, SLOT( chatSessionDestroyed() ) );

		connect(mMsgManager, SIGNAL( myselfTyping( bool ) ),
		        this, SLOT( slotTyping( bool ) ) );
	}
	return mMsgManager;
}

void OscarContact::deleteContact()
{
	mAccount->engine()->removeContact( contactId() );
	deleteLater();
}

void OscarContact::chatSessionDestroyed()
{
	mMsgManager = 0L;
}

// Called when the metacontact owning this contact has changed groups
void OscarContact::sync(unsigned int flags)
{
	/* 
	 * If the contact is waiting for auth, we do nothing
	 * If the contact has changed groups, then we update the server
	 *   adding the group if it doesn't exist, changing the ssi item
	 *   contained in the client and updating the contact's ssi item
	 * Otherwise, we don't do much
	 */
	if ( flags & Kopete::Contact::MovedBetweenGroup == Kopete::Contact::MovedBetweenGroup )
	{

		if ( m_ssiItem.waitingAuth() )
		{
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Contact still requires auth. Doing nothing" << endl;
			return;
		}
		
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Moving a contact between groups" << endl;
		SSIManager* ssiManager = mAccount->engine()->ssiManager();
		SSI oldGroup = ssiManager->findGroup( m_ssiItem.gid() );
		Kopete::Group* newGroup = metaContact()->groups().first();
		if ( newGroup->displayName() == oldGroup.name() )
			return; //we didn't really move
		
		if ( !ssiManager->findGroup( newGroup->displayName() ) )
		{ //we don't have the group on the server
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "the group '" << newGroup->displayName() << "' is not on the server"
				<< "adding it" << endl;
			mAccount->engine()->addGroup( newGroup->displayName() );
		}
		
		SSI newSSIGroup = ssiManager->findGroup( newGroup->displayName() );
		if ( !newSSIGroup )
		{
			kdWarning(OSCAR_GEN_DEBUG) << k_funcinfo << newSSIGroup.name() << " not found on SSI list after addition!" << endl;
			return;
		}
		
		mAccount->engine()->changeContactGroup( contactId(), newGroup->displayName() );
		SSI newItem( m_ssiItem.name(), newSSIGroup.gid(), m_ssiItem.bid(), m_ssiItem.type(),
		             m_ssiItem.tlvList(), m_ssiItem.tlvListLength() );
		setSSIItem( newItem );
	}
	return;
}

void OscarContact::userInfoUpdated( const QString& contact, const UserDetails& details  )
{
	Q_UNUSED( contact );
	setProperty( Kopete::Global::Properties::self()->onlineSince(), details.onlineSinceTime() );
	setIdleTime( details.idleTime() );
	m_warningLevel = details.warningLevel();
	m_details = details;
	
	QStringList capList;
	// Append client name and version in case we found one
	if ( details.userClass() & 0x0080 /* WIRELESS */ )
		capList << i18n( "Mobile AIM Client" );
	else
	{
		if ( !details.clientName().isEmpty() )
		{
			capList << i18n( "Translators: client name and version",
			                "%1").arg( details.clientName() );
		}
	}
	
	// and now for some general informative capabilities
	if ( details.hasCap( CAP_BUDDYICON ) )
		capList << i18n( "Buddy icons" );
	if ( details.hasCap( CAP_UTF8 ) )
		capList << i18n( "UTF-8" );
	if ( details.hasCap( CAP_RTFMSGS ) )
		capList << i18n( "Rich text messages" );
	if ( details.hasCap( CAP_CHAT ) )
		capList << i18n( "Group chat" );
	if ( details.hasCap( CAP_VOICE ) )
		capList << i18n( "Voice chat" );
	if ( details.hasCap( CAP_IMIMAGE ) )
		capList << i18n( "DirectIM/IMImage" );
	if ( details.hasCap( CAP_SENDBUDDYLIST ) )
		capList << i18n( "Send buddy list" );
	if ( details.hasCap( CAP_SENDFILE ) )
		capList << i18n( "File transfers" );
	if ( details.hasCap( CAP_GAMES ) || details.hasCap( CAP_GAMES2 ) )
		capList << i18n( "Games" );
	if ( details.hasCap( CAP_TRILLIAN ) )
		capList << i18n( "Trillian user" );
	
	m_clientFeatures = capList.join( ", " );
	emit featuresUpdated();
}

void OscarContact::startedTyping()
{
	if ( mMsgManager )
		mMsgManager->receivedTypingMsg( this, true );
}

void OscarContact::stoppedTyping()
{
	if ( mMsgManager )
		mMsgManager->receivedTypingMsg( this, false );
}

void OscarContact::slotTyping( bool typing )
{
	if ( this != account()->myself() )
		account()->engine()->sendTyping( contactId(), typing );
}

#include "oscarcontact.moc"
//kate: tab-width 4; indent-mode csands;
