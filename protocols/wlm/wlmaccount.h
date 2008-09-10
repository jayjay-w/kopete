/*
    wlmaccount.h - Kopete Wlm Protocol

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

#ifndef WLMACCOUNT_H
#define WLMACCOUNT_H

#include <kopeteaccount.h>
#include "kopetechatsessionmanager.h"
#include "kopetepasswordedaccount.h"
#include "wlmwebcamdialog.h"
#include "wlmserver.h"
#include "wlmchatsession.h"
#include "wlmtransfermanager.h"
#include "wlmchatmanager.h"
#include "kopetetransfermanager.h"

class KActionMenu;
namespace Kopete
{
    class Contact;
}
namespace Kopete
{
    class MetaContact;
}

class WlmContact;
class WlmProtocol;
class WlmServer;
class WlmTransferManager;
class WlmChatManager;

/**
 * This represents an account connected to the wlm
 * @author Will Stephenson
*/
class WlmAccount:public
    Kopete::PasswordedAccount
{
    Q_OBJECT
  public:
    WlmAccount (WlmProtocol * parent, const QString & accountID,
                const char *name = 0);
     ~
    WlmAccount ();
        /**
	 * Construct the context menu used for the status bar icon
	 */
    virtual void
    fillActionMenu (KActionMenu * actionMenu);

        /**
	 * Creates a protocol specific Kopete::Contact subclass and adds it to the supplie
	 * Kopete::MetaContact
	 */
    virtual bool
    createContact (const QString & contactId,
                   Kopete::MetaContact * parentContact);
        /**
	 * Called when Kopete status is changed globally
	 */
    virtual void
    setOnlineStatus (const Kopete::OnlineStatus & status,
                     const Kopete::StatusMessage & reason =
                     Kopete::StatusMessage ());
    virtual void
    setStatusMessage (const Kopete::StatusMessage & statusMessage);
        /**
	 * 'Connect' to the wlm server.  Only sets myself() online.
	 */
//      virtual void connect( const Kopete::OnlineStatus& initialStatus = Kopete::OnlineStatus::OnlineStatus() );
        /**
	 * Disconnect from the server.  Only sets myself() offline.
	 */
    virtual void
    disconnect ();
        /**
	 * Return a reference to the server stub
	 */
    virtual void
    connectWithPassword (const QString & password);

    WlmServer *
    server ();

    WlmChatManager *
    chatManager ()
    {
        return m_chatManager;
    }
    WlmTransferManager *
    transferManager ()
    {
        return m_transferManager;
    }

    public
        slots:
        /**
	 * Called by the server when it has a message for us.
	 * This identifies the sending Kopete::Contact and passes it a Kopete::Message
	 */
        void
    contactChangedStatus (const MSN::Passport & buddy,
                          const std::string & friendlyname,
                          const MSN::BuddyStatus & status,
                          const unsigned int &clientID,
                          const std::string & msnobject);

    void
    contactDisconnected (const MSN::Passport & buddy);

    void
    connectionCompleted ();

    void
    connectionFailed ();

    void
    changedStatus (MSN::BuddyStatus & state);

    void
    slotGlobalIdentityChanged (Kopete::PropertyContainer *,
                               const QString & key, const QVariant &,
                               const QVariant & newValue);

    void
    setPersonalMessage (const QString & reason);

    void
    addressBookReceivedFromServer (std::map < std::string,
                                   MSN::Buddy * >&list);

    void
    groupListReceivedFromServer (std::map < std::string, MSN::Group > &list);

    void
    gotDisplayName (const QString & displayName);

    void
    gotDisplayPicture (const QString & contactId, const QString & filename);

    void
    gotNewContact (const MSN::ContactList & list, const QString & contact,
                   const QString & friendlyname);

    void
    slotContactAddedNotifyDialogClosed (const QString &);

    void
    receivedOIMList (std::vector < MSN::eachOIM > &oimlist);

    void
    receivedOIM (const QString & id, const QString & message);

    void
    gotContactPersonalInfo (const MSN::Passport & fromPassport,
                            const MSN::personalInfo & pInfo);

    void
    NotificationServerConnectionTerminated (MSN::
                                            NotificationServerConnection *
                                            conn);

    void
    wrongPassword ();

    void
    scheduleConnect ();

    void
    gotAddedContactToAddressBook (const bool & added,
                                  const std::string & passport,
                                  const std::string & displayName,
                                  const std::string & guid);

  protected:
        /**
	 * This simulates contacts going on and offline in sync with the account's status changes
	 */
    void
    updateContactStatus ();

    WlmServer *
        m_server;

    WlmTransferManager *
        m_transferManager;

    WlmChatManager *
        m_chatManager;

    protected
        slots:
        /**
	 * Change the account's status.  Called by KActions and internally.
	 */
        void
    slotGoOnline ();
        /**
	 * Change the account's status.  Called by KActions and internally.
	 */
    void
    slotGoAway (const Kopete::OnlineStatus & status);
        /**
	 * Change the account's status.  Called by KActions and internally.
	 */
    void
    slotGoOffline ();

    void
    slotGoInvisible ();
        /**
	 * Show webcam.  Called by KActions and internally.
	 */
    void
    slotShowVideo ();

    void
    disableInitialList ()
    {
        m_initialList = false;
    }

    void
    enableInitialList ()
    {
        m_initialList = true;
    }

    bool
    isInitialList ()
    {
        return m_initialList;
    }

  private:
    Kopete::OnlineStatus temporaryStatus;

    QString
        m_pictureFilename;

    QString
        m_PersonalMessage;

    bool
        m_initialList;

    uint
        clientid;

    QMap < QString, QString > m_oimList;

};

#endif
