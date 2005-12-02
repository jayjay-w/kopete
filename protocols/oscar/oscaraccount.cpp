/*
    oscaraccount.cpp  -  Oscar Account Class

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
    Copyright (c) 2002 by Chris TenHarmsel <tenharmsel@staticmethod.net>
    Copyright (c) 2004 by Matt Rogers <mattr@kde.org>
    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "oscaraccount.h"

#include "kopetepassword.h"
#include "kopeteprotocol.h"
#include "kopeteaway.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopeteawaydialog.h"
#include "kopetegroup.h"
#include "kopeteuiglobal.h"
#include "kopetecontactlist.h"
#include "kopetecontact.h"
#include "kopetechatsession.h"

#include <assert.h>

#include <qapplication.h>
#include <qregexp.h>
#include <q3stylesheet.h>
#include <qtimer.h>
#include <q3ptrlist.h>
//Added by qt3to4:
#include <Q3ValueList>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpassivepopup.h>

#include "client.h"
#include "connection.h"
#include "oscartypeclasses.h"
#include "oscarmessage.h"
#include "oscarutils.h"
#include "oscarclientstream.h"
#include "oscarconnector.h"
#include "ssimanager.h"
#include "oscarlistnonservercontacts.h"
#include <qtextcodec.h>

class OscarAccountPrivate
{
public:

	//The liboscar hook for the account
	Client* engine;

	quint32 ssiLastModTime;

	//contacts waiting on SSI add ack and their metacontact
	QMap<QString, Kopete::MetaContact*> addContactMap;

	//contacts waiting on their group to be added
	QMap<QString, QString> contactAddQueue;

    OscarListNonServerContacts* olnscDialog;

};

OscarAccount::OscarAccount(Kopete::Protocol *parent, const QString &accountID, const char *name, bool isICQ)
: Kopete::PasswordedAccount( parent, accountID, isICQ ? 8 : 16, name )
{
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << " accountID='" << accountID <<
		"', isICQ=" << isICQ << endl;

	d = new OscarAccountPrivate;
	d->engine = new Client( this );
    d->olnscDialog = 0L;
    QObject::connect( d->engine, SIGNAL( loggedIn() ), this, SLOT( loginActions() ) );
	QObject::connect( d->engine, SIGNAL( messageReceived( const Oscar::Message& ) ),
	                  this, SLOT( messageReceived(const Oscar::Message& ) ) );
	QObject::connect( d->engine, SIGNAL( socketError( int, const QString& ) ),
	                  this, SLOT( slotSocketError( int, const QString& ) ) );
	QObject::connect( d->engine, SIGNAL( taskError( const Oscar::SNAC&, int, bool ) ),
	                  this, SLOT( slotTaskError( const Oscar::SNAC&, int, bool ) ) );
	QObject::connect( d->engine, SIGNAL( userStartedTyping( const QString& ) ),
	                  this, SLOT( userStartedTyping( const QString& ) ) );
	QObject::connect( d->engine, SIGNAL( userStoppedTyping( const QString& ) ),
	                  this, SLOT( userStoppedTyping( const QString& ) ) );
}

OscarAccount::~OscarAccount()
{
	OscarAccount::disconnect();
	delete d;
}

Client* OscarAccount::engine()
{
	return d->engine;
}

void OscarAccount::logOff( Kopete::Account::DisconnectReason reason )
{
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "accountId='" << accountId() << "'" << endl;
	//disconnect the signals
	Kopete::ContactList* kcl = Kopete::ContactList::self();
	QObject::disconnect( kcl, SIGNAL( groupRenamed( Kopete::Group*,  const QString& ) ),
	                     this, SLOT( kopeteGroupRenamed( Kopete::Group*, const QString& ) ) );
	QObject::disconnect( kcl, SIGNAL( groupRemoved( Kopete::Group* ) ),
	                     this, SLOT( kopeteGroupRemoved( Kopete::Group* ) ) );
	QObject::disconnect( d->engine->ssiManager(), SIGNAL( contactAdded( const Oscar::SSI& ) ),
	                     this, SLOT( ssiContactAdded( const Oscar::SSI& ) ) );
	QObject::disconnect( d->engine->ssiManager(), SIGNAL( groupAdded( const Oscar::SSI& ) ),
	                     this, SLOT( ssiGroupAdded( const Oscar::SSI& ) ) );

	d->engine->close();
	myself()->setOnlineStatus( Kopete::OnlineStatus::Offline );

	foreach ( Kopete::Contact* c, contacts() )
	{
		c->setOnlineStatus(Kopete::OnlineStatus::Offline);
	}

	disconnected( reason );
}

void OscarAccount::disconnect()
{
	logOff( Kopete::Account::Manual );
}

bool OscarAccount::passwordWasWrong()
{
	return password().isWrong();
}

void OscarAccount::updateContact( Oscar::SSI item )
{
	Kopete::Contact* contact = contacts()[item.name()];
	if ( !contact )
		return;
	else
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Updating SSI Item" << endl;
		OscarContact* oc = static_cast<OscarContact*>( contact );
		oc->setSSIItem( item );
	}
}

void OscarAccount::loginActions()
{
    password().setWrong( false );
    kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "processing SSI list" << endl;
    processSSIList();

	//start a chat nav connection
	if ( !engine()->isIcq() )
	{
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "sending request for chat nav service" << endl;
		d->engine->requestServerRedirect( 0x000D );
	}

	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "sending request for icon service" << endl;
	d->engine->requestServerRedirect( 0x0010 );

}

void OscarAccount::processSSIList()
{
	//disconnect signals so we don't attempt to add things to SSI!
	Kopete::ContactList* kcl = Kopete::ContactList::self();
	QObject::disconnect( kcl, SIGNAL( groupRenamed( Kopete::Group*,  const QString& ) ),
	                     this, SLOT( kopeteGroupRenamed( Kopete::Group*, const QString& ) ) );
	QObject::disconnect( kcl, SIGNAL( groupRemoved( Kopete::Group* ) ),
	                     this, SLOT( kopeteGroupRemoved( Kopete::Group* ) ) );

	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << endl;

	SSIManager* listManager = d->engine->ssiManager();

    //first add groups
	Q3ValueList<SSI> groupList = listManager->groupList();
	Q3ValueList<SSI>::const_iterator git = groupList.constBegin();
	Q3ValueList<SSI>::const_iterator listEnd = groupList.constEnd();
	//the protocol dictates that there is at least one group that has contacts
	//so i don't have to check for an empty group list

	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Adding " << groupList.count() << " groups to contact list" << endl;
	for( ; git != listEnd; ++git )
	{ //add all the groups.
		kdDebug( OSCAR_GEN_DEBUG ) << k_funcinfo << "Adding SSI group'" << ( *git ).name()
			<< "' to the kopete contact list" << endl;
		kcl->findGroup( ( *git ).name() );
	}

	//then add contacts
	Q3ValueList<SSI> contactList = listManager->contactList();
	Q3ValueList<SSI>::const_iterator bit = contactList.constBegin();
	Q3ValueList<SSI>::const_iterator blistEnd = contactList.constEnd();
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Adding " << contactList.count() << " contacts to contact list" << endl;
	for ( ; bit != blistEnd; ++bit )
	{
		SSI groupForAdd = listManager->findGroup( ( *bit ).gid() );
		Kopete::Group* group;
		if ( groupForAdd.isValid() )
			group = kcl->findGroup( groupForAdd.name() ); //add if not present
		else
			group = kcl->findGroup( i18n( "Buddies" ) );

		kdDebug( OSCAR_GEN_DEBUG ) << k_funcinfo << "Adding contact '" << ( *bit ).name() << "' to kopete list in group " <<
			group->displayName() << endl;
		OscarContact* oc = dynamic_cast<OscarContact*>( contacts()[( *bit ).name()] );
		if ( oc )
		{
			Oscar::SSI item = ( *bit );
			oc->setSSIItem( item );
		}
		else
			addContact( ( *bit ).name(), QString::null, group, Kopete::Account::DontChangeKABC );
	}

	QObject::connect( kcl, SIGNAL( groupRenamed( Kopete::Group*,  const QString& ) ),
	                  this, SLOT( kopeteGroupRenamed( Kopete::Group*, const QString& ) ) );
	QObject::connect( kcl, SIGNAL( groupRemoved( Kopete::Group* ) ),
	                  this, SLOT( kopeteGroupRemoved( Kopete::Group* ) ) );
	QObject::connect( listManager, SIGNAL( contactAdded( const Oscar::SSI& ) ),
	                  this, SLOT( ssiContactAdded( const Oscar::SSI& ) ) );
	QObject::connect( listManager, SIGNAL( groupAdded( const Oscar::SSI& ) ),
	                  this, SLOT( ssiGroupAdded( const Oscar::SSI& ) ) );

	/** Commented for compilation purposes. 
    Q3Dict<Kopete::Contact> nonServerContacts = contacts();
    Q3DictIterator<Kopete::Contact> it( nonServerContacts );
    QStringList nonServerContactList;
    for ( ; it.current(); ++it )
    {
        OscarContact* oc = dynamic_cast<OscarContact*>( ( *it ) );
        if ( !oc )
            continue;
        kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << oc->contactId() << " contact ssi type: " << oc->ssiItem().type() << endl;
        if ( !oc->isOnServer() )
            nonServerContactList.append( ( *it )->contactId() );
    }
    kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "the following contacts are not on the server side list"
                             << nonServerContactList << endl;
    if ( !nonServerContactList.isEmpty() )
    {
        d->olnscDialog = new OscarListNonServerContacts( Kopete::UI::Global::mainWidget() );
        QObject::connect( d->olnscDialog, SIGNAL( closing() ),
                          this, SLOT( nonServerAddContactDialogClosed() ) );
        d->olnscDialog->addContacts( nonServerContactList );
        d->olnscDialog->show();
    }
	*/
}

void OscarAccount::nonServerAddContactDialogClosed()
{
    if ( !d->olnscDialog )
        return;

    if ( d->olnscDialog->result() == QDialog::Accepted )
    {
        //start adding contacts
        kdDebug(OSCAR_GEN_DEBUG) << "adding non server contacts to the contact list" << endl;
        //get the contact list. get the OscarContact object, then the group
        //check if the group is on ssi, if not, add it
        //if so, add the contact.
        QStringList offliners = d->olnscDialog->nonServerContactList();
        QStringList::iterator it, itEnd = offliners.end();
        for ( it = offliners.begin(); it != itEnd; ++it )
        {
            OscarContact* oc = dynamic_cast<OscarContact*>( contacts()[( *it )] );
            if ( !oc )
            {
                kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "no OscarContact object available for"
                                         << ( *it ) << endl;
                continue;
            }

            Kopete::MetaContact* mc = oc->metaContact();
            if ( !mc )
            {
                kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "no metacontact object available for"
                                         << ( oc->contactId() ) << endl;
                continue;
            }

            Kopete::Group* group = mc->groups().first();
            if ( !group )
            {
                kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "no metacontact object available for"
                                         << ( oc->contactId() ) << endl;
                continue;
            }

            SSIManager* listManager = d->engine->ssiManager();
            if ( !listManager->findGroup( group->displayName() ) )
            {
                kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "adding non-existant group "
                                         << group->displayName() << endl;
                d->contactAddQueue[Oscar::normalize( ( *it ) )] = group->displayName();
                d->engine->addGroup( group->displayName() );
            }
            else
            {
                d->engine->addContact( ( *it ), group->displayName() );
            }
        }


    }

    d->olnscDialog->delayedDestruct();
    d->olnscDialog = 0L;
}

void OscarAccount::slotGoOffline()
{
	OscarAccount::disconnect();
}

void OscarAccount::slotGoOnline()
{
	//do nothing
}

void OscarAccount::kopeteGroupRemoved( Kopete::Group* group )
{
	if ( isConnected() )
		d->engine->removeGroup( group->displayName() );
}

void OscarAccount::kopeteGroupAdded( Kopete::Group* group )
{
	if ( isConnected() )
		d->engine->addGroup( group->displayName() );
}

void OscarAccount::kopeteGroupRenamed( Kopete::Group* group, const QString& oldName )
{
	if ( isConnected() )
		d->engine->renameGroup( oldName, group->displayName() );
}

void OscarAccount::messageReceived( const Oscar::Message& message )
{
	//the message isn't for us somehow
	if ( Oscar::normalize( message.receiver() ) != Oscar::normalize( accountId() ) )
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "got a message but we're not the receiver: "
			<< message.text() << endl;
		return;
	}

	/* Logic behind this:
	 * If we don't have the contact yet, create it as a temporary
	 * Create the message manager
	 * Get the sanitized message back
	 * Append to the chat window
	 */
	QString sender = Oscar::normalize( message.sender() );
	if ( !contacts()[sender] )
	{
		kdDebug(OSCAR_RAW_DEBUG) << "Adding '" << sender << "' as temporary contact" << endl;
		addContact( sender, QString::null, 0,  Kopete::Account::Temporary );
	}

	OscarContact* ocSender = static_cast<OscarContact *> ( contacts()[sender] ); //should exist now

	if ( !ocSender )
	{
		kdWarning(OSCAR_RAW_DEBUG) << "Temporary contact creation failed for '"
			<< sender << "'! Discarding message: " << message.text() << endl;
		return;
	}
	else
	{
		if ( ( message.properties() & Oscar::Message::WWP ) == Oscar::Message::WWP )
			ocSender->setNickName( i18n("ICQ Web Express") );
		if ( ( message.properties() & Oscar::Message::EMail ) == Oscar::Message::EMail )
			ocSender->setNickName( i18n("ICQ Email Express") );
	}

	Kopete::ChatSession* chatSession = ocSender->manager( Kopete::Contact::CanCreate );
	chatSession->receivedTypingMsg( ocSender, false ); //person is done typing


    //decode message
    //HACK HACK HACK! Until AIM supports per contact encoding, just decode as ISO-8559-1
    QTextCodec* codec = 0L;
    if ( ocSender->hasProperty( "contactEncoding" ) )
        codec = QTextCodec::codecForMib( ocSender->property( "contactEncoding" ).value().toInt() );
    else
        codec = QTextCodec::codecForMib( 4 );

    QString realText = message.text();
    if ( message.properties() & Oscar::Message::NotDecoded )
        realText = codec->toUnicode( message.textArray() );

    //sanitize;
    QString sanitizedMsg = sanitizedMessage( realText );

	Kopete::ContactPtrList me;
	me.append( myself() );
	Kopete::Message chatMessage( message.timestamp(), ocSender, me, sanitizedMsg,
	                             Kopete::Message::Inbound, Kopete::Message::RichText );

	chatSession->appendMessage( chatMessage );
}


void OscarAccount::setServerAddress(const QString &server)
{
	configGroup()->writeEntry( QString::fromLatin1( "Server" ), server );
}

void OscarAccount::setServerPort(int port)
{
	if ( port > 0 )
		configGroup()->writeEntry( QString::fromLatin1( "Port" ), port );
	else //set to default 5190
		configGroup()->writeEntry( QString::fromLatin1( "Port" ), 5190 );
}

Connection* OscarAccount::setupConnection( const QString& server, uint port )
{
	//set up the connector
	KNetworkConnector* knc = new KNetworkConnector( 0 );
	knc->setOptHostPort( server, port );

	//set up the clientstream
	ClientStream* cs = new ClientStream( knc, 0 );

	Connection* c = new Connection( knc, cs, "AUTHORIZER" );
	c->setClient( d->engine );

	return c;
}


bool OscarAccount::createContact(const QString &contactId,
	Kopete::MetaContact *parentContact)
{
	/* We're not even online or connecting
	 * (when getting server contacts), so don't bother
	 */
	if ( !myself()->isOnline() )
	{
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Can't add contact, we are offline!" << endl;
		return false;
	}

	/* Logic for SSI additions
	If the contact is temporary, no SSI addition at all. Just create the contact and be done with it
	If the contact is not temporary, we need to do the following:
		1. Check if contact already exists in the SSI manager, if so, just create the contact
		2. If contact doesn't exist:
		2.a. create group on SSI if needed
		2.b. create contact on SSI
		2.c. create kopete contact
	 */

	Q3ValueList<TLV> dummyList;
	if ( parentContact->isTemporary() )
	{
		SSI tempItem( contactId, 0, 0, 0xFFFF, dummyList, 0 );
		return createNewContact( contactId, parentContact, tempItem );
	}

	SSI ssiItem = d->engine->ssiManager()->findContact( contactId );
	if ( ssiItem )
	{
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Have new SSI entry. Finding contact" << endl;
		if ( contacts()[ssiItem.name()] )
		{
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Found contact in list. Updating SSI item" << endl;
			OscarContact* oc = static_cast<OscarContact*>( contacts()[ssiItem.name()] );
			oc->setSSIItem( ssiItem );
			return true;
		}
		else
		{
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Didn't find contact in list, creating new contact" << endl;
			return createNewContact( contactId, parentContact, ssiItem );
		}
	}
	else
	{ //new contact, check temporary, if temporary, don't add to SSI. otherwise, add.
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "New contact '" << contactId << "' not in SSI."
			<< " Creating new contact" << endl;

		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Adding " << contactId << " to server side list" << endl;

		QString groupName;
		Kopete::GroupList kopeteGroups = parentContact->groups(); //get the group list

		if ( kopeteGroups.isEmpty() || kopeteGroups.first() == Kopete::Group::topLevel() )
		{
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Contact with NO group. " << "Adding to group 'Buddies'" << endl;
			groupName = i18n("Buddies");
		}
		else
		{
				//apparently kopeteGroups.first() can be invalid. Attempt to prevent
				//crashes in SSIData::findGroup(const QString& name)
			groupName = kopeteGroups.first() ? kopeteGroups.first()->displayName() : i18n("Buddies");

			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Contact with group." << " No. of groups = " << kopeteGroups.count() <<
				" Name of first group = " << groupName << endl;
		}

		if( groupName.isEmpty() )
		{ // emergency exit, should never occur
			kdWarning(OSCAR_GEN_DEBUG) << k_funcinfo << "Could not add contact because no groupname was given" << endl;
			return false;
		}

		if ( !d->engine->ssiManager()->findGroup( groupName ) )
		{ //group isn't on SSI
			d->contactAddQueue[Oscar::normalize( contactId )] = groupName;
			d->addContactMap[Oscar::normalize( contactId )] = parentContact;
			d->engine->addGroup( groupName );
			return true;
		}

		d->addContactMap[Oscar::normalize( contactId )] = parentContact;
		d->engine->addContact( Oscar::normalize( contactId ), groupName );
		return true;
	}
}

void OscarAccount::ssiContactAdded( const Oscar::SSI& item )
{
	if ( d->addContactMap.contains( Oscar::normalize( item.name() ) ) )
	{
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Received confirmation from server. adding " << item.name()
			<< " to the contact list" << endl;
		createNewContact( item.name(), d->addContactMap[Oscar::normalize( item.name() )], item );
	}
	else
		kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Got addition for contact we weren't waiting on" << endl;
}

void OscarAccount::ssiGroupAdded( const Oscar::SSI& item )
{
	//check the contact add queue for any contacts matching the
	//group name we just added
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "Looking for contacts to be added in group " << item.name() << endl;
	QMap<QString,QString>::iterator it;
	for ( it = d->contactAddQueue.begin(); it != d->contactAddQueue.end(); ++it )
	{
		if ( Oscar::normalize( it.data() ) == Oscar::normalize( item.name() ) )
		{
			kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "starting delayed add of contact '" << it.key() << "' to group "
				<< item.name() << endl;
			d->engine->addContact( Oscar::normalize( it.key() ), item.name() ); //already in the map
		}
	}
}

void OscarAccount::userStartedTyping( const QString & contact )
{
	Kopete::Contact * ct = contacts()[ Oscar::normalize( contact ) ];
	if ( ct && contact != accountId() )
	{
		OscarContact * oc = static_cast<OscarContact *>( ct );
		oc->startedTyping();
	}
}

void OscarAccount::userStoppedTyping( const QString & contact )
{
	Kopete::Contact * ct = contacts()[ Oscar::normalize( contact ) ];
	if ( ct && contact != accountId() )
	{
		OscarContact * oc = static_cast<OscarContact *>( ct );
		oc->stoppedTyping();
	}
}

void OscarAccount::slotSocketError( int errCode, const QString& errString )
{
	Q_UNUSED( errCode );
	KPassivePopup::message( i18n( "account has been disconnected", "%1 disconnected" ).arg( accountId() ),
	                        errString,
	                        myself()->onlineStatus().protocolIcon(),
	                        Kopete::UI::Global::mainWidget() );
	logOff( Kopete::Account::ConnectionReset );
}

void OscarAccount::slotTaskError( const Oscar::SNAC& s, int code, bool fatal )
{
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "error recieived from task" << endl;
	kdDebug(OSCAR_GEN_DEBUG) << k_funcinfo << "service: " << s.family
		<< " subtype: " << s.subtype << " code: " << code << endl;

	QString message;
	if ( s.family == 0 && s.subtype == 0 )
	{
		message = getFLAPErrorMessage( code );
		KPassivePopup::message( i18n( "account has been disconnected", "%1 disconnected" ).arg( accountId() ),
		                        message, myself()->onlineStatus().protocolIcon(),
		                        Kopete::UI::Global::mainWidget() );
		switch ( code )
		{
		case 0x0004:
		case 0x0005:
			logOff( Kopete::Account::BadPassword );
			break;
		case 0x0007:
		case 0x0008:
		case 0x0009:
		case 0x0011:
			logOff( Kopete::Account::BadUserName );
			break;
		default:
			logOff( Kopete::Account::Manual );
		}
		return;
	}
	if ( !fatal )
		message = i18n("There was an error in the protocol handling; it was not fatal, so you will not be disconnected.");
	else
		message = i18n("There was an error in the protocol handling; automatic reconnection occurring.");

	KPassivePopup::message( i18n("OSCAR Protocol error"), message, myself()->onlineStatus().protocolIcon(),
	                        Kopete::UI::Global::mainWidget() );
	if ( fatal )
		logOff( Kopete::Account::ConnectionReset );
}

QString OscarAccount::getFLAPErrorMessage( int code )
{
	bool isICQ = d->engine->isIcq();
	QString acctType = isICQ ? i18n("ICQ") : i18n("AIM");
	QString acctDescription = isICQ ? i18n("ICQ user id", "UIN") : i18n("AIM user id", "screen name");
	QString reason;
	//FLAP errors are always fatal
	//negative codes are things added by liboscar developers
	//to indicate generic errors in the task
	switch ( code )
	{
	case 0x0001:
		if ( isConnected() ) // multiple logins (on same UIN)
		{
			reason = i18n( "You have logged in more than once with the same %1," \
			               " account %2 is now disconnected.")
				.arg( acctDescription ).arg( accountId() );
		}
		else // error while logging in
		{
			reason = i18n( "Sign on failed because either your %1 or " \
			               "password are invalid. Please check your settings for account %2.")
				.arg( acctDescription ).arg( accountId() );

		}
		break;
	case 0x0002: // Service temporarily unavailable
	case 0x0014: // Reservation map error
		reason = i18n("The %1 service is temporarily unavailable. Please try again later.")
			.arg( acctType );
		break;
	case 0x0004: // Incorrect nick or password, re-enter
	case 0x0005: // Mismatch nick or password, re-enter
		reason = i18n("Could not sign on to %1 with account %2 because the " \
		              "password was incorrect.").arg( acctType ).arg( accountId() );
		break;
	case 0x0007: // non-existant ICQ#
	case 0x0008: // non-existant ICQ#
		reason = i18n("Could not sign on to %1 with nonexistent account %2.")
			.arg( acctType ).arg( accountId() );
		break;
	case 0x0009: // Expired account
		reason = i18n("Sign on to %1 failed because your account %2 expired.")
			.arg( acctType ).arg( accountId() );
		break;
	case 0x0011: // Suspended account
		reason = i18n("Sign on to %1 failed because your account %2 is " \
		              "currently suspended.").arg( acctType ).arg( accountId() );
		break;
	case 0x0015: // too many clients from same IP
	case 0x0016: // too many clients from same IP
	case 0x0017: // too many clients from same IP (reservation)
		reason = i18n("Could not sign on to %1 as there are too many clients" \
		              " from the same computer.").arg( acctType );
		break;
	case 0x0018: // rate exceeded (turboing)
		if ( isConnected() )
		{
			reason = i18n("Account %1 was blocked on the %2 server for" \
							" sending messages too quickly." \
							" Wait ten minutes and try again." \
							" If you continue to try, you will" \
							" need to wait even longer.")
				.arg( accountId() ).arg( acctType );
		}
		else
		{
			reason = i18n("Account %1 was blocked on the %2 server for" \
							" reconnecting too quickly." \
							" Wait ten minutes and try again." \
							" If you continue to try, you will" \
							" need to wait even longer.")
				.arg( accountId() ).arg( acctType) ;
		}
		break;
	case 0x001C:
		reason = i18n("The %1 server thinks the client you are using is " \
		              "too old. Please report this as a bug at http://bugs.kde.org")
			.arg( acctType );
		break;
	case 0x0022: // Account suspended because of your age (age < 13)
		reason = i18n("Account %1 was disabled on the %2 server because " \
		              "of your age (less than 13).")
			.arg( accountId() ).arg( acctType );
		break;
	default:
		if ( !isConnected() )
		{
			reason = i18n("Sign on to %1 with your account %2 failed.")
				.arg( acctType ).arg( accountId() );
		}
		break;
	}
	return reason;
}
#include "oscaraccount.moc"
//kate: tab-width 4; indent-mode csands;
