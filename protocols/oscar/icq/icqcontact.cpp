/*
  icqontact.cpp  -  Oscar Protocol Plugin

  Copyright (c) 2003 by Stefan Gehn  <metz AT gehn.net>
  Copyright (c) 2003 by Olivier Goffart
  Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
  */

#include "icqcontact.h"

#include "icqprotocol.h"
#include "icqaccount.h"

#include <qapplication.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kopetemessagemanagerfactory.h"

#include "icquserinfo.h"
#include "icqreadaway.h"

static const unsigned int SUPPORTED_INFO_ITEMS = 7;


ICQContact::ICQContact(const QString name, const QString displayName,
	ICQAccount *acc, KopeteMetaContact *parent)
	: OscarContact(name, displayName, acc, parent)
{
	mProtocol = static_cast<ICQProtocol *>(protocol());

	mInvisible = false;
	setOnlineStatus(mProtocol->statusOffline);

	infoDialog = 0L;
	awayMessageDialog = 0L;

	userinfoRequestSequence=0;
	userinfoReplyCount = 0;

	generalInfo.uin=0;
	generalInfo.countryCode=0;
	generalInfo.timezoneCode=0;
	generalInfo.publishEmail=false;
	generalInfo.showOnWeb=false;
	workInfo.occupation=0;

	// Buddy Changed
	QObject::connect(
		acc->engine(), SIGNAL(gotContactChange(const UserInfo &)),
		this, SLOT(slotContactChanged(const UserInfo &)));

	QObject::connect(
		acc->engine(), SIGNAL(gotICQGeneralUserInfo(const int, const ICQGeneralUserInfo &)),
		this, SLOT(slotUpdGeneralInfo(const int, const ICQGeneralUserInfo &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQWorkUserInfo(const int, const ICQWorkUserInfo &)),
		this, SLOT(slotUpdWorkInfo(const int, const ICQWorkUserInfo &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQMoreUserInfo(const int, const ICQMoreUserInfo &)),
		this, SLOT(slotUpdMoreUserInfo(const int, const ICQMoreUserInfo &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQAboutUserInfo(const int, const QString &)),
		this, SLOT(slotUpdAboutUserInfo(const int, const QString &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQEmailUserInfo(const int, const ICQMailList &)),
		this, SLOT(slotUpdEmailUserInfo(const int, const ICQMailList &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQInfoItemList(const int, const ICQInfoItemList &)),
		this, SLOT(slotUpdInterestUserInfo(const int, const ICQInfoItemList &)));
	QObject::connect(
		acc->engine(), SIGNAL(gotICQInfoItemList(const int, const ICQInfoItemList &, const ICQInfoItemList & )),
		this, SLOT(slotUpdBackgroundUserInfo(const int, const ICQInfoItemList &, const ICQInfoItemList & ) ) );

/*	if(name == displayName && account()->isConnected())
	{
		kdDebug(14200) << k_funcinfo << "ICQ Contact with no nickname, grabbing userinfo" << endl;
		requestUserInfo();
	}*/

	actionReadAwayMessage = 0L;
}

ICQContact::~ICQContact()
{
}

void ICQContact::setOwnDisplayName(const QString &s)
{
	kdDebug(14200) << k_funcinfo << "Called." << endl;
	if(this == account()->myself())
		setDisplayName(s);
}

void ICQContact::slotContactChanged(const UserInfo &u)
{
	if (u.sn != contactName())
		return;

	/*kdDebug(14190) << k_funcinfo << "Called for '"
		<< displayName() << "', contactName()=" << contactName() << endl;*/

	unsigned int newStatus = 0;
	mInvisible = (u.icqextstatus & ICQ_STATUS_IS_INVIS);
	if (u.icqextstatus & ICQ_STATUS_IS_FFC)			newStatus = OSCAR_FFC;
	else if (u.icqextstatus & ICQ_STATUS_IS_DND)	newStatus = OSCAR_DND;
	else if (u.icqextstatus & ICQ_STATUS_IS_OCC)	newStatus = OSCAR_OCC;
	else if (u.icqextstatus & ICQ_STATUS_IS_NA)		newStatus = OSCAR_NA;
	else if (u.icqextstatus & ICQ_STATUS_IS_AWAY)	newStatus = OSCAR_AWAY;
	else											newStatus = OSCAR_ONLINE;

	if((this != account()->myself()) &&
	   (newStatus != onlineStatus().internalStatus()) &&
	   (newStatus != OSCAR_ONLINE) &&
	   (account()->myself()->onlineStatus().status() != KopeteOnlineStatus::Connecting))
	{
		// TODO: Add queues for away message requests
		mAccount->engine()->requestAwayMessage(this);
	}
	else
	{
		removeProperty("awayMessage"); // unset awayMessage
	}

	setStatus(newStatus);

	slotUpdateBuddy();
}

void ICQContact::slotOffgoingBuddy(QString sender)
{
	if(sender != contactName())
		return;

	setOnlineStatus(mProtocol->statusOffline);
	slotUpdateBuddy();
}

#if 0
void ICQContact::gotIM(OscarSocket::OscarMessageType /*type*/, const QString &message)
{
	// Build a KopeteMessage and set the body as Rich Text
	KopeteContactPtrList tmpList;
	tmpList.append(account()->myself());
	KopeteMessage msg(this, tmpList, message, KopeteMessage::Inbound,
		KopeteMessage::RichText);
	manager(true)->appendMessage(msg);
}
#endif

void ICQContact::slotSendMsg(KopeteMessage& message, KopeteMessageManager *)
{
	if (message.plainBody().isEmpty()) // no text, do nothing
		return;

	// Check to see if we're even online
	if(!account()->isConnected())
	{
		KMessageBox::sorry(qApp->mainWidget(),
			i18n("<qt>You must be logged on to ICQ before you can "
				"send a message to a user.</qt>"),
			i18n("Not Signed On"));
		return;
	}

	// FIXME: We don't do HTML in ICQ
	// we might be able to do that in AIM and we might also convert
	// HTML to RTF for ICQ type-2 messages  [mETz]
	static_cast<OscarAccount*>(account())->engine()->sendIM(
		message.plainBody(), this, false);

	// Show the message we just sent in the chat window
	manager()->appendMessage(message);
	manager()->messageSucceeded();
}

bool ICQContact::isReachable()
{
	return true;
}

QPtrList<KAction> *ICQContact::customContextMenuActions()
{
	QPtrList<KAction> *actionCollection = new QPtrList<KAction>();

	QString awTxt;
	QString awIcn;
	unsigned int status = onlineStatus().internalStatus();
	if (status >= 15)
		status -= 15; // get rid of invis addon
	switch(status)
	{
		case OSCAR_FFC:
			awTxt = i18n("Read 'Free For Chat' &Message");
			awIcn = "icq_ffc";
			break;
		case OSCAR_DND:
			awTxt = i18n("Read 'Do Not Disturb' &Message");
			awIcn = "icq_dnd";
			break;
		case OSCAR_NA:
			awTxt = i18n("Read 'Not Available' &Message");
			awIcn = "icq_na";
			break;
		case OSCAR_OCC:
			awTxt = i18n("Read 'Occupied' &Message");
			awIcn = "icq_occ";
			break;
		default:
			awTxt = i18n("Read 'Away' &Message");
			awIcn = "icq_away";
			break;
	}

	if(actionReadAwayMessage==0)
	{
		actionReadAwayMessage = new KAction(awTxt, awIcn, 0,
			this, SLOT(slotReadAwayMessage()), this, "actionReadAwayMessage");
		actionRequestAuth = new KAction(i18n("&Request Authorization"), "mail_reply", 0,
			this, SLOT(slotRequestAuth()), this, "actionRequestAuth");
		actionSendAuth = new KAction(i18n("&Send Authorization"), "mail_forward", 0,
			this, SLOT(slotSendAuth()), this, "actionSendAuth");
	}

	actionRequestAuth->setEnabled(waitAuth() && account()->isConnected());
	actionSendAuth->setEnabled(account()->isConnected());
	actionReadAwayMessage->setEnabled(status != OSCAR_OFFLINE && status != OSCAR_ONLINE);

	actionCollection->append(actionRequestAuth);
	actionCollection->append(actionSendAuth);
	actionCollection->append(actionReadAwayMessage);

	return actionCollection;
}

void ICQContact::setStatus(const unsigned int newStatus)
{
	if((onlineStatus().internalStatus() == newStatus) && !mInvisible)
		return;

	switch(newStatus)
	{
		case OSCAR_FFC:
			setOnlineStatus(mProtocol->statusFFC);
			break;
		case OSCAR_OFFLINE:
			mInvisible = false;
			setOnlineStatus(mProtocol->statusOffline);
			break;
		case OSCAR_AWAY:
			setOnlineStatus(mProtocol->statusAway);
			break;
		case OSCAR_DND:
			setOnlineStatus(mProtocol->statusDND);
			break;
		case OSCAR_NA:
			setOnlineStatus(mProtocol->statusNA);
			break;
		case OSCAR_OCC:
			setOnlineStatus(mProtocol->statusOCC);
			break;
		case OSCAR_CONNECTING:
			setOnlineStatus(mProtocol->statusConnecting); // ONLY for myself() contact
			break;
		default: // emergency choose, also OSCAR_ONLINE
			setOnlineStatus(mProtocol->statusOnline);
	}
}

void ICQContact::setOnlineStatus(const KopeteOnlineStatus& status)
{
	if(mInvisible)
	{
		kdDebug(14200) << k_funcinfo << "'" << displayName() << "' is invisible!" << endl;
		KopeteContact::setOnlineStatus(
			KopeteOnlineStatus(
				status.status() , (status.weight()==0) ? 0 : (status.weight() -1),
				protocol(),
				status.internalStatus()+15,
				QString::fromLatin1("icq_invisible"),
				status.caption(),
				i18n("%1|Invisible").arg(status.description())
				)
			);
	}
	else
	{
		KopeteContact::setOnlineStatus(status);
	}
/*
	kdDebug(14200) << k_funcinfo << "'" << displayName() << "' is now " <<
		onlineStatus().description() << endl;
*/
}


void ICQContact::slotUserInfo()
{
	if (!infoDialog)
	{
		infoDialog = new ICQUserInfo(this, 0L, "infoDialog");
		if(!infoDialog)
			return;
		QObject::connect(infoDialog, SIGNAL(closing()), this, SLOT(slotCloseUserInfoDialog()));
		infoDialog->show();
	}
	else
	{
		infoDialog->raise();
	}
}

void ICQContact::slotCloseUserInfoDialog()
{
	infoDialog->delayedDestruct();
	infoDialog = 0L;
}


void ICQContact::slotReadAwayMessage()
{
	kdDebug(14200) << k_funcinfo << "account='" << account()->accountId() <<
		"', contact='" << displayName() << "'" << endl;

	if (!awayMessageDialog)
	{
		awayMessageDialog = new ICQReadAway(this, 0L, "awayMessageDialog");
		if(!awayMessageDialog)
			return;
		QObject::connect(awayMessageDialog, SIGNAL(closing()), this, SLOT(slotCloseAwayMessageDialog()));
		awayMessageDialog->show();
	}
	else
	{
		awayMessageDialog->raise();
	}
}

void ICQContact::slotCloseAwayMessageDialog()
{
	awayMessageDialog->delayedDestruct();
	awayMessageDialog = 0L;
}


void ICQContact::requestUserInfo()
{
	//kdDebug(14200) << k_funcinfo << "called" << endl;
	userinfoReplyCount = 0;
	userinfoRequestSequence =
		account()->engine()->sendReqInfo(contactName().toULong());
}

void ICQContact::slotUpdGeneralInfo(const int seq, const ICQGeneralUserInfo &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	generalInfo = inf;

	if(!generalInfo.firstName.isEmpty())
		setProperty("firstName", i18n("First Name"), generalInfo.firstName);
	else
		removeProperty("firstName");

	if(!generalInfo.lastName.isEmpty())
		setProperty("lastName", i18n("Last Name"), generalInfo.lastName);
	else
		removeProperty("lastName");

	if(!generalInfo.eMail.isEmpty())
		setProperty("emailAddress", i18n("eMail"), generalInfo.eMail);
	else
		removeProperty("emailAddress");

	if(!generalInfo.phoneNumber.isEmpty())
		setProperty("privPhoneNum", i18n("Private Phone Number"), generalInfo.phoneNumber);
	else
		removeProperty("privPhoneNum");

	if(!generalInfo.faxNumber.isEmpty())
		setProperty("privFaxNum", i18n("Private Fax Number"), generalInfo.faxNumber);
	else
		removeProperty("privFaxNum");

	if(!generalInfo.cellularNumber.isEmpty())
		setProperty("privMobileNum", i18n("Private Mobile Number"), generalInfo.cellularNumber);
	else
		removeProperty("privMobileNum");

	if(contactName() == displayName() && !generalInfo.nickName.isEmpty())
	{
		kdDebug(14200) << k_funcinfo << "setting new displayname for former UIN-only Contact" << endl;
		setDisplayName(generalInfo.nickName);
	}

	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdWorkInfo(const int seq, const ICQWorkUserInfo &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	workInfo = inf;

	if(!workInfo.phone.isEmpty())
		setProperty("workPhoneNum", i18n("Work Phone Number"), workInfo.phone);
	else
		removeProperty("workPhoneNum");

	if(!workInfo.fax.isEmpty())
		setProperty("workFaxNum", i18n("Work Fax Number"), workInfo.fax);
	else
		removeProperty("workFaxNum");

	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdMoreUserInfo(const int seq, const ICQMoreUserInfo &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	moreInfo = inf;
	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdAboutUserInfo(const int seq, const QString &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	aboutInfo = inf;
	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdEmailUserInfo(const int seq, const ICQMailList &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	emailInfo = inf;
	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdInterestUserInfo(const int seq, const ICQInfoItemList &inf)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	interestInfo = inf;
	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

void ICQContact::slotUpdBackgroundUserInfo(const int seq, const ICQInfoItemList &curr, const ICQInfoItemList &past)
{
	// compare reply's sequence with the one we sent with our last request
	if(seq != userinfoRequestSequence)
		return;
	currentBackground = curr;
	pastBackground = past;
	userinfoReplyCount++;
	if (userinfoReplyCount >= SUPPORTED_INFO_ITEMS)
		emit updatedUserInfo();
}

#include "icqcontact.moc"
// vim: set noet ts=4 sts=4 sw=4:
