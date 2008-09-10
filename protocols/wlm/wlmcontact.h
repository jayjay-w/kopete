/*
    wlmcontact.h - Kopete Wlm Protocol

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

#ifndef WLMCONTACT_H
#define WLMCONTACT_H

#include <QMap>
//Added by qt3to4:
#include <QList>
#include <kaction.h>
#include <kurl.h>
#include "kopetecontact.h"
#include "kopetemessage.h"

#include "wlmchatsession.h"

class KAction;
class KActionCollection;
namespace Kopete
{
    class Account;
}
namespace Kopete
{
    class ChatSession;
}
namespace Kopete
{
    class MetaContact;
}

/**
@author Will Stephenson
*/
class WlmContact:public
    Kopete::Contact
{
    Q_OBJECT
  public:
    WlmContact (Kopete::Account * _account, const QString & uniqueName,
                const QString & contactSerial,
                const QString & displayName, Kopete::MetaContact * parent);

    ~WlmContact ();

    virtual bool isReachable ();
    /**
	 * Serialize the contact's data into a key-value map
	 * suitable for writing to a file
	 */
    virtual void serialize (QMap < QString, QString > &serializedData,
               QMap < QString, QString > &addressBookData);
    /**
	 * Return the actions for this contact
	 */
    virtual QList <KAction *>*customContextMenuActions ();
        /**
	 * Returns a Kopete::ChatSession associated with this contact
	 */
    virtual Kopete::ChatSession * manager (CanCreateFlags canCreate = CannotCreate);

    void
    setContactSerial (QString contactSerial)
    {
        m_contactSerial = contactSerial;
    }

    public slots:
    /**
	 * Transmits an outgoing message to the server 
	 * Called when the chat window send button has been pressed
	 * (in response to the relevant Kopete::ChatSession signal)
	 */
    void sendMessage (Kopete::Message & message);
    /**
	 * Called when an incoming message arrived
	 * This displays it in the chatwindow
	 */
    void receivedMessage (const QString & message);

    QString
    getMsnObj ()
    {
        return m_msnobj;
    }

    void
    setMsnObj (QString msnobj)
    {
        m_msnobj = msnobj;
    }

    virtual void deleteContact ();

    virtual void sendFile (const KUrl & sourceURL = KUrl (),
              const QString & fileName = QString::null, uint fileSize = 0L);

    protected slots:
    /**
	 * Show the settings dialog
	 */
    void showContactSettings ();
        /**
	 * Notify the contact that its current Kopete::ChatSession was
	 * destroyed - probably by the chatwindow being closed
	 */
    void slotChatSessionDestroyed ();

  protected:
    WlmChatSession * m_msgManager;
    KActionCollection * m_actionCollection;
    KAction * m_actionPrefs;
    QString m_msnobj;
    QString m_contactSerial;
};

#endif
