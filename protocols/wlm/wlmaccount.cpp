/*
    wlmaccount.cpp - Kopete Wlm Protocol

    Copyright (c) 2008      by Tiago Salem Herrmann <tiagosh@gmail.com>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "wlmaccount.h"

#include <QTextCodec>
#include <QFile>
#include <QImage>
#include <QDomDocument>

#include <kaction.h>
#include <kactionmenu.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <KCodecs>
#include <KStandardDirs>

#include "kopetechatsessionmanager.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetegroup.h"
#include "kopetepassword.h"
#include "kopeteuiglobal.h"
#include "kopetepicture.h"
#include "kopeteutils.h"
#include "kopetetransfermanager.h"
#include "kopeteidentity.h"
#include "kopeteavatarmanager.h"
#include "contactaddednotifydialog.h"

#include "wlmcontact.h"
#include "wlmprotocol.h"
#include "wlmchatsession.h"

WlmAccount::WlmAccount (WlmProtocol * parent, const QString & accountID,
                        const char *name):
Kopete::PasswordedAccount (parent, accountID.lower (), name),
m_server (NULL),
m_transferManager (NULL),
m_chatManager (NULL),
clientid (0)
{
    // Init the myself contact
    setMyself (new
               WlmContact (this, accountId (), QString::null,
                           accountId (),
                           Kopete::ContactList::self ()->myself ()));
    myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
    clientid += MSN::MSNC7;
    clientid += MSN::MSNC6;
    clientid += MSN::MSNC5;
    clientid += MSN::MSNC4;
    clientid += MSN::MSNC3;
    clientid += MSN::MSNC2;
    clientid += MSN::MSNC1;
    clientid += MSN::SupportWinks;
    clientid += MSN::VoiceClips;
    clientid += MSN::InkGifSupport;
    clientid += MSN::SIPInvitations;
    clientid += MSN::SupportMultiPacketMessaging;

}

WlmAccount::~WlmAccount ()
{
    disconnect ();
}

void
WlmAccount::fillActionMenu (KActionMenu * actionMenu)
{
    Kopete::Account::fillActionMenu (actionMenu);

    actionMenu->addSeparator ();

    KAction *action;

    action =
        new KAction (KIcon ("testbed_showvideo"),
                     i18n ("Show my own video..."), actionMenu);
    //, "actionShowVideo");
    QObject::connect (action, SIGNAL (triggered (bool)), this,
                      SLOT (slotShowVideo ()));
    actionMenu->addAction (action);
    action->setEnabled (isConnected ());
}

bool
WlmAccount::createContact (const QString & contactId,
                           Kopete::MetaContact * parentContact)
{
    WlmContact *newContact =
        new WlmContact (this, contactId, QString::null,
                        parentContact->displayName (), parentContact);
    return newContact != 0L;
}

void
WlmAccount::setPersonalMessage (const QString & reason)
{
    kdDebug (14210) << k_funcinfo << endl;
    myself ()->setProperty (WlmProtocol::protocol ()->personalMessage,
                            reason);
    if (isConnected ())
    {
        MSN::personalInfo pInfo;
        QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("utf8"));
        if (reason.isEmpty ())
            pInfo.PSM = "";
        else
            pInfo.PSM = reason.toAscii ().data ();
//      pInfo.mediaType="Music";
        pInfo.mediaIsEnabled = 0;
//      pInfo.mediaFormat="{0} - {1}";
//      pInfo.mediaLines.push_back("Artist");
//      pInfo.mediaLines.push_back("Song");
        m_server->cb.mainConnection->setPersonalStatus (pInfo);
    }
}

void
WlmAccount::setOnlineStatus (const Kopete::OnlineStatus & status,
                             const Kopete::StatusMessage & reason)
{
    kdDebug (14210) << k_funcinfo << endl;

    setPersonalMessage (reason.message ());

    temporaryStatus = status;

    if (status == WlmProtocol::protocol ()->wlmConnecting &&
        myself ()->onlineStatus () == WlmProtocol::protocol ()->wlmOffline)
        slotGoOnline ();
    else if (status == WlmProtocol::protocol ()->wlmOnline)
        slotGoOnline ();
    else if (status == WlmProtocol::protocol ()->wlmOffline)
        slotGoOffline ();
    else if (status == WlmProtocol::protocol ()->wlmInvisible)
        slotGoInvisible ();
    else if (status.status () == Kopete::OnlineStatus::Away)
        slotGoAway (status);
}

void
WlmAccount::setStatusMessage (const Kopete::StatusMessage & statusMessage)
{
    setPersonalMessage (statusMessage.message ());
}

void
WlmAccount::connectWithPassword (const QString & pass)
{
    kdDebug (14210) << k_funcinfo << endl;
    if (myself ()->onlineStatus () != WlmProtocol::protocol ()->wlmOffline)
        return;

    if (pass.isEmpty ())
    {
        password ().setWrong (true);
        password ().setWrong (false);
        return;
    }

    password ().setWrong (false);

    QString id = accountId ();
    QString pass1 = pass;

    enableInitialList ();

    m_server = new WlmServer (this, id, pass1);
    m_server->WlmConnect ();

    m_transferManager = new WlmTransferManager (this);

    m_chatManager = new WlmChatManager (this);

    QObject::connect (&m_server->cb, SIGNAL (connectionCompleted ()),
                      this, SLOT (connectionCompleted ()));
    QObject::connect (&m_server->cb, SIGNAL (connectionFailed ()),
                      this, SLOT (connectionFailed ()));
    QObject::connect (&m_server->cb,
                      SIGNAL (gotDisplayName (const QString &)), this,
                      SLOT (gotDisplayName (const QString &)));
    QObject::connect (&m_server->cb,
                      SIGNAL (receivedOIMList
                              (std::vector < MSN::eachOIM > &)), this,
                      SLOT (receivedOIMList
                            (std::vector < MSN::eachOIM > &)));
    QObject::connect (&m_server->cb,
                      SIGNAL (receivedOIM (const QString &, const QString &)),
                      this,
                      SLOT (receivedOIM (const QString &, const QString &)));
    QObject::connect (&m_server->cb,
                      SIGNAL (receivedOIM (const QString &, const QString &)),
                      this,
                      SLOT (receivedOIM (const QString &, const QString &)));
    QObject::connect (&m_server->cb,
                      SIGNAL (NotificationServerConnectionTerminated
                              (MSN::NotificationServerConnection *)), this,
                      SLOT (NotificationServerConnectionTerminated
                            (MSN::NotificationServerConnection *)));
    QObject::connect (&m_server->cb, SIGNAL (wrongPassword ()), this,
                      SLOT (wrongPassword ()));

    myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmConnecting);
}

void
WlmAccount::gotNewContact (const MSN::ContactList & list,
                           const QString & contact,
                           const QString & friendlyname)
{
    if (list == MSN::LST_RL)
    {
        Kopete::UI::ContactAddedNotifyDialog * dialog =
            new Kopete::UI::ContactAddedNotifyDialog (contact, friendlyname,
                           this,Kopete::UI::ContactAddedNotifyDialog::InfoButton | 
			   Kopete::UI::ContactAddedNotifyDialog::AddGroupBox | 
			   Kopete::UI::ContactAddedNotifyDialog::AuthorizeCheckBox);
        QObject::connect (dialog, SIGNAL (applyClicked (const QString &)),
                          this,SLOT (slotContactAddedNotifyDialogClosed(const QString &)));
        dialog->show ();
    }
}

void
WlmAccount::slotContactAddedNotifyDialogClosed (const QString & contactId)
{
    const Kopete::UI::ContactAddedNotifyDialog * dialog =
        dynamic_cast <
        const Kopete::UI::ContactAddedNotifyDialog * >(sender ());

    if (!dialog)
        return;

    if (dialog->added ())
    {
        // TODO: find a way to get the friendly name received in gotNewContact()
        m_server->cb.mainConnection->addToAddressBook (contactId.toAscii().data(),
                                                       contactId.toAscii().data());
    }
}

void
WlmAccount::wrongPassword ()
{
    kdDebug (14210) << k_funcinfo << endl;
    password ().setWrong (true);
}

void
WlmAccount::scheduleConnect ()
{
    connect (temporaryStatus);
}

void
WlmAccount::gotDisplayPicture (const QString & contactId,
                               const QString & filename)
{
    kdDebug (14210) << k_funcinfo << endl;
    Kopete::Contact * contact = contacts ()[contactId];
    if (contact)
    {
        if (!QFile (filename).exists () || !QFile (filename).size ())
        {
            QFile (filename).remove ();
            contact->removeProperty (Kopete::Global::Properties::self ()->
                                     photo ());
            dynamic_cast < WlmContact * >(contact)->setMsnObj ("0");
            return;
        }
        // remove old picture
        if (contact->
            hasProperty (Kopete::Global::Properties::self()->photo ().key ()))
        {
            QString file =
                contact->property (
			Kopete::Global::Properties::self ()->photo()).value ().toString ();
            contact->removeProperty (Kopete::Global::Properties::self ()->photo ());
            if (QFile (file).exists () && file != filename)
                QFile (file).remove ();
        }
        contact->setProperty (Kopete::Global::Properties::self ()->photo (),
                              filename);
    }
}

void
WlmAccount::gotDisplayName (const QString & displayName)
{
    kdDebug (14210) << k_funcinfo << endl;
    myself ()->setProperty (Kopete::Global::Properties::self ()->nickName (),
                            displayName);
}

void
WlmAccount::gotContactPersonalInfo (const MSN::Passport & fromPassport,
                                    const MSN::personalInfo & pInfo)
{
    kdDebug (14210) << k_funcinfo << endl;
    Kopete::Contact * contact = contacts ()[fromPassport.c_str ()];
    if (contact)
    {
        // TODO - handle the other fields of pInfo
        contact->setProperty (WlmProtocol::protocol ()->personalMessage,
                              QString (pInfo.PSM.c_str ()));
        QString type (pInfo.mediaType.c_str ());
        if (pInfo.mediaIsEnabled && type == "Music")
        {
            QString song_line (pInfo.mediaFormat.c_str ());
            int num = pInfo.mediaLines.size ();
            for (int i = 0; i < num; i++)
            {
                song_line.replace ("{" + QString::number (i) + "}",
                                   pInfo.mediaLines[i].c_str ());
            }
            contact->setProperty (WlmProtocol::protocol ()->currentSong,
                                  song_line.toAscii ().data ());
        }
        else
        {
            contact->setProperty (WlmProtocol::protocol ()->currentSong,
                                  QVariant (QString::null));
        }
    }
}

void
WlmAccount::contactChangedStatus (const MSN::Passport & buddy,
                                  const std::string & friendlyname,
                                  const MSN::BuddyStatus & state,
                                  const unsigned int &clientID,
                                  const std::string & msnobject)
{
    kdDebug (14210) << k_funcinfo << endl;
    Kopete::Contact * contact = contacts ()[buddy.c_str ()];
    if (contact)
    {
        contact->setProperty (Kopete::Global::Properties::self ()->
                              nickName (), QString (friendlyname.c_str ()));

        if (state == MSN::STATUS_AWAY)
            contact->setOnlineStatus (WlmProtocol::protocol ()->wlmAway);
        else if (state == MSN::STATUS_AVAILABLE)
            contact->setOnlineStatus (WlmProtocol::protocol ()->wlmOnline);
        else if (state == MSN::STATUS_INVISIBLE)
            contact->setOnlineStatus (WlmProtocol::protocol ()->wlmInvisible);
        else if (state == MSN::STATUS_BUSY)
            contact->setOnlineStatus (WlmProtocol::protocol ()->wlmBusy);
        else if (state == MSN::STATUS_OUTTOLUNCH)
            contact->setOnlineStatus (WlmProtocol::protocol ()->
                                      wlmOutToLunch);
        else if (state == MSN::STATUS_ONTHEPHONE)
            contact->setOnlineStatus (WlmProtocol::protocol ()->
                                      wlmOnThePhone);
        else if (state == MSN::STATUS_BERIGHTBACK)
            contact->setOnlineStatus (WlmProtocol::protocol ()->
                                      wlmBeRightBack);
        else if (state == MSN::STATUS_IDLE)
            contact->setOnlineStatus (WlmProtocol::protocol ()->wlmIdle);

        QString a = QString (msnobject.c_str ());
        dynamic_cast <WlmContact *>(contact)->setMsnObj (msnobject.c_str ());

        if (a.isEmpty () || a == "0")   // no picture
        {
            contact->removeProperty (Kopete::Global::Properties::self ()->
                                     photo ());
            return;
        }

        QDomDocument xmlobj;
        xmlobj.setContent (a);

        // track display pictures by SHA1D field
        QString SHA1D = xmlobj.documentElement ().attribute ("SHA1D");

        if (SHA1D.isEmpty ())
            return;

        QString newlocation =
            KGlobal::dirs ()->locateLocal ("appdata",
                                           "wlmpictures/" +
                                           QString (SHA1D.replace ("/", "_")));

        if (QFile (newlocation).exists () && QFile (newlocation).size ())
        {
            gotDisplayPicture (contact->contactId (), newlocation);
            return;
        }

        // do not request all pictures at once when you are just connected
        if (isInitialList ())
            return;

        if ((myself ()->onlineStatus () !=
                WlmProtocol::protocol ()->wlmOffline)
               && (myself ()->onlineStatus () !=
                   WlmProtocol::protocol ()->wlmInvisible)
               && (myself ()->onlineStatus () !=
                   WlmProtocol::protocol ()->wlmUnknown))
        {
            chatManager ()->requestDisplayPicture (buddy.c_str ());
        }
    }
}

void
WlmAccount::contactDisconnected (const MSN::Passport & buddy)
{
    kdDebug (14210) << k_funcinfo << endl;
    Kopete::Contact * contact = contacts ()[buddy.c_str ()];
    if (contact)
    {
        contact->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
    }
}

void
WlmAccount::groupListReceivedFromServer (std::map < std::string,
                                         MSN::Group > &list)
{
    kdDebug (14210) << k_funcinfo << endl;
    // add server groups on local list
    std::map < std::string, MSN::Group >::iterator it;
    for (it = list.begin (); it != list.end (); ++it)   // groups from server
    {
        MSN::Group * g = &(*it).second;
        Kopete::Group * b =
            Kopete::ContactList::self ()->findGroup (g->name.c_str ());
        QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("utf8"));
        if (!b)
            Kopete::ContactList::self ()->addGroup (
                      new Kopete::Group (QString (g->name.c_str ()).toAscii ().data ()));
    }
/*
	// remove local groups which are not on server
	QPtrList<Kopete::Group>::Iterator it1 =  Kopete::ContactList::self()->groups().begin();
	Kopete::Group *group;
	for ( group = Kopete::ContactList::self()->groups().first(); group; group = Kopete::ContactList::self()->groups().next() )
	{
		bool ok=false;
		std::map<std::string, MSN::Group>::iterator it;
		for ( it = list.begin(); it != list.end(); ++it ) // groups from server
		{
			MSN::Group *g = &(*it).second;
			if(Kopete::ContactList::self()->findGroup(QString(g->name.c_str()).toAscii().data()))
				ok=true;
		}
		if(!ok)
		{
			if(!group->members().count())
				Kopete::ContactList::self()->removeGroup(group);
		}
	}
*/
}

void
WlmAccount::addressBookReceivedFromServer (std::map < std::string,
                                           MSN::Buddy * >&list)
{
    kdDebug (14210) << k_funcinfo << endl;
    // local contacts which dont exist on server should be deleted
    std::map < std::string, MSN::Buddy * >::iterator it;
    for (it = list.begin (); it != list.end (); ++it)
    {
        Kopete::MetaContact * metacontact = 0L;
        MSN::Buddy * b = (*it).second;
        Kopete::Contact * contact = contacts ()[(*it).first.c_str ()];
        if (!contact)
        {
            if (!b->friendlyName.length ())
                b->friendlyName = b->userName;

            QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("utf8"));
            std::list < MSN::Group * >::iterator i = b->groups.begin ();
            bool ok = false;

            // no groups, add to top level
            if (!b->groups.size ())
            {
                // only add users in forward list
                if (b->lists & MSN::LST_AB)
                    metacontact =
                        addContact (b->userName.c_str (),
                                    QString (b->friendlyName.c_str ()).
                                    ascii (), 0L,
                                    Kopete::Account::DontChangeKABC);

                if (metacontact)
                {
                    Kopete::Contact * c = contacts ()[(*it).first.c_str ()];
                    if (c)
                    {
                        WlmContact *contact = dynamic_cast <WlmContact *>(c);
                        if (contact)
                        {
                            contact->setContactSerial (b->properties["contactId"].c_str ());
                            kdDebug (14210) << "ContactID: " << b->
                                properties["contactId"].c_str () << endl;
                        }
                        metacontact->setDisplayNameSourceContact (c);
                        metacontact->setDisplayNameSource (Kopete::MetaContact::SourceContact);
                    }
                }
                continue;
            }

            for (; i != b->groups.end (); ++i)
            {
                Kopete::Group * g =
                    Kopete::ContactList::self ()->
                    findGroup (QString ((*i)->name.c_str ()).ascii ());
                if (g)
                    metacontact =
                        addContact (b->userName.c_str (),
                                    QString (b->friendlyName.c_str ()).
                                    toAscii ().data (), g,
                                    Kopete::Account::DontChangeKABC);
                else
                    metacontact =
                        addContact (b->userName.c_str (),
                                    QString (b->friendlyName.c_str ()).
                                    toAscii ().data (),
                                    Kopete::Group::topLevel (),
                                    Kopete::Account::DontChangeKABC);

                if (metacontact)
                {
                    Kopete::Contact * c = contacts ()[(*it).first.c_str ()];
                    if (c)
                    {
                        WlmContact *contact = dynamic_cast <WlmContact *>(c);
                        if (contact)
                        {
                            contact->setContactSerial (b->
                                                       properties
                                                       ["contactId"].
                                                       c_str ());
                            kdDebug (14210) << "ContactID: " << b->
                                properties["contactId"].c_str () << endl;
                        }
                        metacontact->setDisplayNameSourceContact (c);
                        metacontact->setDisplayNameSource (Kopete::MetaContact::SourceContact);
                    }
                }
            }
        }
    }
}

void
WlmAccount::slotGlobalIdentityChanged (Kopete::PropertyContainer *,
                                       const QString & key, const QVariant &,
                                       const QVariant & newValue)
{
    kdDebug (14210) << k_funcinfo << endl;
    if (key == Kopete::Global::Properties::self ()->photo ().key ())
    {
        m_pictureFilename = newValue.toString ();
        // TODO - Set no photo on server
        if (m_pictureFilename.isEmpty ())
        {
            myself()->removeProperty(Kopete::Global::Properties::self()->photo ());
            if(m_server && isConnected ())
            {
                m_server->cb.mainConnection->change_DisplayPicture ("");
                setOnlineStatus (myself ()->onlineStatus ());
            }
            return;
        }

        QImage contactPhoto = QImage( m_pictureFilename );
        Kopete::AvatarManager::AvatarEntry entry;
        entry.name = myself ()->contactId();
        entry.image = contactPhoto;
        entry.category = Kopete::AvatarManager::Contact;
        entry.contact = myself();
        entry = Kopete::AvatarManager::self()->add(entry);

        kdDebug (14140) << k_funcinfo << m_pictureFilename << endl;
        if(!entry.path.isNull())
        {
            if(m_server)
               m_server->cb.mainConnection->
                   change_DisplayPicture (entry.path.toLatin1 ().data ());
               myself()->setProperty(Kopete::Global::Properties::self()->photo (), entry.path);
        }
        setOnlineStatus (myself ()->onlineStatus ());
    }
    else if (key == Kopete::Global::Properties::self ()->nickName ().key ())
    {
        QString oldNick =
            myself ()->property (Kopete::Global::Properties::self ()->
                                 nickName ()).value ().toString ();
        QString newNick = newValue.toString ();

        if (newNick != oldNick)
        {
            QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("utf8"));
            if(m_server && isConnected())
                m_server->cb.mainConnection->setFriendlyName (newNick.toAscii ().
                                                          data ());
        }
    }
}

void
WlmAccount::changedStatus (MSN::BuddyStatus & state)
{
    kdDebug (14210) << k_funcinfo << endl;
    if (state == MSN::STATUS_AWAY)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmAway);
    else if (state == MSN::STATUS_AVAILABLE)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOnline);
    else if (state == MSN::STATUS_INVISIBLE)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmInvisible);
    else if (state == MSN::STATUS_BUSY)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmBusy);
    else if (state == MSN::STATUS_OUTTOLUNCH)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOutToLunch);
    else if (state == MSN::STATUS_ONTHEPHONE)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOnThePhone);
    else if (state == MSN::STATUS_BERIGHTBACK)
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmBeRightBack);
}

void
WlmAccount::connectionFailed ()
{
    kdDebug (14210) << k_funcinfo << endl;
    myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
    Kopete::Utils::notifyCannotConnect (this);
}

void
WlmAccount::connectionCompleted ()
{
    kdDebug (14210) << k_funcinfo << endl;
    if (identity ()->
        hasProperty (Kopete::Global::Properties::self ()->photo ().key()))
    {
        m_server->cb.mainConnection->change_DisplayPicture (identity ()->
                                                            customIcon ().
                                                            toLatin1 ().
                                                            data ());
        QImage contactPhoto = QImage( identity ()->customIcon() );
        Kopete::AvatarManager::AvatarEntry entry;
        entry.name = myself ()->contactId();
        entry.image = contactPhoto;
        entry.category = Kopete::AvatarManager::Contact;
        entry.contact = myself();
        entry = Kopete::AvatarManager::self()->add(entry);
        if(!entry.path.isNull())
            myself()->setProperty(Kopete::Global::Properties::self()->photo (), entry.path);
    }

    password ().setWrong (false);

    QObject::connect (&m_server->cb,
                      SIGNAL (changedStatus (MSN::BuddyStatus &)), this,
                      SLOT (changedStatus (MSN::BuddyStatus &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (contactChangedStatus
                              (const MSN::Passport &, const std::string &,
                               const MSN::BuddyStatus &, const unsigned int &,
                               const std::string &)), this,
                      SLOT (contactChangedStatus
                            (const MSN::Passport &, const std::string &,
                             const MSN::BuddyStatus &, const unsigned int &,
                             const std::string &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (contactDisconnected (const MSN::Passport &)),
                      this,
                      SLOT (contactDisconnected (const MSN::Passport &)));

    QObject::connect (identity (),
                      SIGNAL (propertyChanged
                              (Kopete::PropertyContainer *, const QString &,
                               const QVariant &, const QVariant &)),
                      SLOT (slotGlobalIdentityChanged
                            (Kopete::PropertyContainer *, const QString &,
                             const QVariant &, const QVariant &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (gotDisplayPicture
                              (const QString &, const QString &)),
                      SLOT (gotDisplayPicture
                            (const QString &, const QString &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (gotContactPersonalInfo
                              (const MSN::Passport &,
                               const MSN::personalInfo &)),
                      SLOT (gotContactPersonalInfo
                            (const MSN::Passport &,
                             const MSN::personalInfo &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (gotNewContact
                              (const MSN::ContactList &, const QString &,
                               const QString &)),
                      SLOT (gotNewContact
                            (const MSN::ContactList &, const QString &,
                             const QString &)));

    QObject::connect (&m_server->cb,
                      SIGNAL (gotAddedContactToAddressBook
                              (const bool &, const std::string &,
                               const std::string &, const std::string &)),
                      SLOT (gotAddedContactToAddressBook
                            (const bool &, const std::string &,
                             const std::string &, const std::string &)));

    MSN::BuddyStatus state = MSN::STATUS_AVAILABLE;

    if (temporaryStatus == WlmProtocol::protocol ()->wlmOnline)
        state = MSN::STATUS_AVAILABLE;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmAway)
        state = MSN::STATUS_AWAY;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmInvisible)
        state = MSN::STATUS_INVISIBLE;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmBusy)
        state = MSN::STATUS_BUSY;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmOutToLunch)
        state = MSN::STATUS_OUTTOLUNCH;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmOnThePhone)
        state = MSN::STATUS_ONTHEPHONE;
    else if (temporaryStatus == WlmProtocol::protocol ()->wlmBeRightBack)
        state = MSN::STATUS_BERIGHTBACK;

    m_server->cb.mainConnection->setState (state, clientid);
    // this prevents our client from downloading display pictures
    // when is just connected.
    QTimer::singleShot (10 * 1000, this, SLOT (disableInitialList ()));
    QString current_msg =
        myself ()->property (WlmProtocol::protocol ()->personalMessage).
        value ().toString ();
    setPersonalMessage (current_msg);
}

void
WlmAccount::gotAddedContactToAddressBook (const bool & added,
                                          const std::string & passport,
                                          const std::string & displayName,
                                          const std::string & guid)
{
    if (added)
    {
        addContact (QString (passport.c_str ()),
                    QString (displayName.c_str ()).toAscii ().data (),
                    Kopete::Group::topLevel (),
                    Kopete::Account::DontChangeKABC);
    }
    else
    {
        // TODO: Raise an error
    }
}

void
WlmAccount::NotificationServerConnectionTerminated (MSN::
                                                    NotificationServerConnection
                                                    * conn)
{
    kdDebug (14210) << k_funcinfo << endl;

    if (myself ()->onlineStatus () == WlmProtocol::protocol ()->wlmConnecting
        && !password ().isWrong ())
    {
        connectionFailed ();
        return;
    }
    if (password ().isWrong ())
    {
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
        QTimer::singleShot (2 * 1000, this, SLOT (scheduleConnect ()));
        return;
    }
    if (isConnected ())
    {
        myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
    }
}

void
WlmAccount::disconnect ()
{
    kdDebug (14210) << k_funcinfo << endl;
    if (m_server)
        m_server->WlmDisconnect ();

    myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);

    QObject::disconnect (Kopete::ContactList::self (), 0, 0, 0);
    QObject::disconnect (Kopete::TransferManager::transferManager (), 0, 0, 0);

    if (m_transferManager)
    {
        delete m_transferManager;
        m_transferManager = NULL;
    }
    if (m_chatManager)
    {
        delete m_chatManager;
        m_chatManager = NULL;
    }
    if (m_server)
    {
        QObject::disconnect (&m_server->cb, 0, 0, 0);
        delete m_server;
        m_server = NULL;
    }
}

WlmServer *
WlmAccount::server ()
{
    return m_server;
}

void
WlmAccount::slotGoOnline ()
{
    kdDebug (14210) << k_funcinfo << endl;

    if (!isConnected ())
        connect (WlmProtocol::protocol ()->wlmOnline);
    else
        m_server->cb.mainConnection->setState (MSN::STATUS_AVAILABLE,
                                               clientid);
}

void
WlmAccount::slotGoInvisible ()
{
    kdDebug (14210) << k_funcinfo << endl;

    if (!isConnected ())
        connect (WlmProtocol::protocol ()->wlmInvisible);
    else
        m_server->cb.mainConnection->setState (MSN::STATUS_INVISIBLE,
                                               clientid);
}

void
WlmAccount::slotGoAway (const Kopete::OnlineStatus & status)
{
    kdDebug (14210) << k_funcinfo << endl;

    if (!isConnected ())
        connect (status);
    else
    {
        if (status == WlmProtocol::protocol ()->wlmAway)
            m_server->cb.mainConnection->setState (MSN::STATUS_AWAY,
                                                   clientid);
        else if (status == WlmProtocol::protocol ()->wlmOutToLunch)
            m_server->cb.mainConnection->setState (MSN::STATUS_OUTTOLUNCH,
                                                   clientid);
        else if (status == WlmProtocol::protocol ()->wlmBusy)
            m_server->cb.mainConnection->setState (MSN::STATUS_BUSY,
                                                   clientid);
        else if (status == WlmProtocol::protocol ()->wlmOnThePhone)
            m_server->cb.mainConnection->setState (MSN::STATUS_ONTHEPHONE,
                                                   clientid);
        else if (status == WlmProtocol::protocol ()->wlmBeRightBack)
            m_server->cb.mainConnection->setState (MSN::STATUS_BERIGHTBACK,
                                                   clientid);
    }
}

void
WlmAccount::slotGoOffline ()
{
    kdDebug (14210) << k_funcinfo << endl;

    if (isConnected ()
        || myself ()->onlineStatus ().status () ==
        Kopete::OnlineStatus::Connecting)
        disconnect ();
    myself ()->setOnlineStatus (WlmProtocol::protocol ()->wlmOffline);
}

void
WlmAccount::slotShowVideo ()
{
    kdDebug (14210) << k_funcinfo << endl;

    if (isConnected ())
        WlmWebcamDialog *wlmWebcamDialog = new WlmWebcamDialog (0, 0);
}

void
WlmAccount::updateContactStatus ()
{
}

void
WlmAccount::receivedOIMList (std::vector < MSN::eachOIM > &oimlist)
{
    kdDebug (14210) << k_funcinfo << endl;
    std::vector < MSN::eachOIM >::iterator i = oimlist.begin ();
    for (; i != oimlist.end (); i++)
    {
        m_oimList[(*i).id.c_str ()] = (*i).from.c_str ();
        m_server->cb.mainConnection->get_oim ((*i).id, true);
    }
}

void
WlmAccount::receivedOIM (const QString & id, const QString & message)
{
    kdDebug (14210) << k_funcinfo << endl;
    QString contactId = m_oimList[id];
    Kopete::Contact * contact = contacts ()[contactId];

    Kopete::Message msg = Kopete::Message (contact, myself ());
    msg.setPlainBody (message);
    msg.setDirection (Kopete::Message::Inbound);

    if (contact)
        contact->manager (Kopete::Contact::CanCreate)->appendMessage (msg);

    m_oimList.remove (id);
    m_server->cb.mainConnection->delete_oim (id.toLatin1 ().data ());
}

#include "wlmaccount.moc"
