/*
  oscarsocket.cpp  -  Oscar Protocol Implementation

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
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

#include "oscarsocket.h"
#include "rateclass.h"
#include "oscardebug.h"

#include <stdlib.h>
#include <netinet/in.h> // for htonl()

#include "oscaraccount.h"

#include <qobject.h>
#include <qtextcodec.h>
#include <qtimer.h>

#include <kdebug.h>
//#include <kextsock.h>

// ---------------------------------------------------------------------------------------

OscarSocket::OscarSocket(const QString &connName, const QByteArray &cookie,
	OscarAccount *account, QObject *parent, const char *name, bool isicq)
	: OscarConnection(connName, Server, cookie, parent, name)
{
//	kdDebug(14150) << k_funcinfo << "connName=" << connName <<
//		QString::fromLatin1( isicq?" ICQ":" AIM" ) << endl;

	mIsICQ=isicq;
	toicqsrv_seq=1;
	type2SequenceNum=0xFFFF;
	flapSequenceNum=rand() & 0x7FFF; // value taken from libicq
	mPwEncryptionKey=0L;
	mCookie=0L;
	loginStatus=0;
	gotAllRights=0;
	keepaliveTime=30;
	keepaliveTimer=0L;
	rateClasses.setAutoDelete(TRUE);
	isLoggedIn=false;
	idle=false;
	//mDirectConnnectionCookie=rand();
	mAccount=account;
	//mDirectIMMgr=0L;
	//mFileTransferMgr=0L;
	//bSomethingOutgoing=false;

	QObject::connect(this, SIGNAL(socketClosed(const QString &)),
		this, SLOT(slotConnectionClosed()));
	QObject::connect(this, SIGNAL(moreToRead()),
		this, SLOT(slotRead()));
	/*
	QObject::connect(this, SIGNAL(serverReady()), this, SLOT(OnServerReady()));
	QObject::connect(mSocket, SIGNAL(bytesWritten(int)),
		this, SLOT(slotBytesWritten(int)));
	*/
}

OscarSocket::~OscarSocket()
{
	kdDebug(14150) << k_funcinfo << "Called" << endl;

	if(socketStatus() == OscarConnection::Connecting ||
		socketStatus() == OscarConnection::Connected )
	{
		kdDebug(14150) << k_funcinfo <<
			"ERROR: WE ARE NOT DISCONNECTED YET" << endl;

		if(mIsICQ)
			stopKeepalive();

		QObject::disconnect(mSocket, 0, 0, 0);
		mSocket->reset();
	}

	//delete mDirectIMMgr;
	//delete mFileTransferMgr;
	delete[] mCookie;
	delete[] mPwEncryptionKey;

	for (RateClass *tmp=rateClasses.first(); tmp; tmp=rateClasses.next())
		QObject::disconnect(tmp, SIGNAL(dataReady(Buffer &)), this, SLOT(writeData(Buffer &)));
	rateClasses.clear();
}

DWORD OscarSocket::setIPv4Address(const QString &address)
{
	kdDebug(14150) << "Setting IP address to: " << address << endl;
	QString a=address.simplifyWhiteSpace();

	QStringList ipv4Addr=QStringList::split(".", a, FALSE);
	if (ipv4Addr.count() == 4)
	{
		unsigned long newAddr=0;
		int i=0;
		bool ok=true;
		while (ok && i < 4)
		{
			unsigned long value=ipv4Addr[i].toUInt(&ok);
			if (value > 255)
				ok=false;
			if (ok)
				newAddr=newAddr * 256 + value;
			i++;
		}
		if (ok)
			return newAddr;
	}
	return 0;
}

SSIData& OscarSocket::ssiData()
{
	return mSSIData;
}

void OscarSocket::slotConnected()
{
	kdDebug(14150) << k_funcinfo <<
		"Connected to '" << peerHost() <<
		"', port '" << peerPort() << "'" << endl;

#if 0
	QString h=mSocket->localAddress()->nodeName();
	mDirectIMMgr=new OncomingSocket(this, h, DirectIM);
	mFileTransferMgr=new OncomingSocket(this, h, SendFile, SENDFILE_PORT);
#endif
	//emit connectionChanged(1, QString("Connected to %2, port %1").arg(peerPort()).arg(peerName()));
}

void OscarSocket::slotConnectionClosed()
{
	kdDebug(14150) << k_funcinfo << "Connection for account '" <<
		mAccount->accountId() << "' closed." << endl;

	/*
	kdDebug(14150) << k_funcinfo << "Socket Status: " <<
			mSocket->strError(mSocket->socketStatus(), mSocket->systemError()) << endl;
	*/

	if(mSocket->bytesAvailable() > 0)
	{
		kdDebug(14150) << k_funcinfo <<
			mSocket->bytesAvailable() << " bytes were left to read" << endl;
	}

	if(mIsICQ)
		stopKeepalive();

	rateClasses.clear();
	loginStatus=0;
	idle=false;
	gotAllRights=0;
	isLoggedIn=false;

	mSocket->reset();
	//kdDebug(14150) << k_funcinfo << "Socket state is " << state() << endl;

	QObject::disconnect(this, SIGNAL(connAckReceived()), 0, 0);
	QObject::disconnect(mSocket, SIGNAL(socketConnected(const QString &)), 0, 0);

	/*if(mDirectIMMgr)
	{
		delete mDirectIMMgr;
		mDirectIMMgr=0L;
	}

	if(mFileTransferMgr)
	{
		delete mFileTransferMgr;
		mFileTransferMgr=0L;
	}*/

	kdDebug(14150) << k_funcinfo << "emitting statusChanged(OSCAR_OFFLINE)" << endl;
	emit statusChanged(OSCAR_OFFLINE);
}

/*void OscarSocket::slotBytesWritten(int n)
{
	if (n > 0 && !bSomethingOutgoing)
	{
		//kdDebug(14150) << k_funcinfo << "Setting bSomethingOutgoing = true" << endl;
		bSomethingOutgoing = true;
	}
}*/

void OscarSocket::slotRead()
{
	//kdDebug(14150) << k_funcinfo << "-----------------------" << endl;

	//int waitCount=0;
	char *buf=0L;
	Buffer inbuf;
	FLAP fl;
	int bytesread=0;

	/*kdDebug(14150) << k_funcinfo <<
		mSocket->bytesAvailable() << " bytes to read" << endl;*/

	// a FLAP is min 6 bytes, we cannot read more out of
	// a buffer without these 6 initial bytes
	if(mSocket->bytesAvailable() < 6)
	{
		/*kdDebug(14150) << k_funcinfo <<
		"less than 6 bytes available, waiting for m data" << endl;*/
		mSocket->waitForMore(200);
		return;
	}

	fl = getFLAP();

	if (fl.error || fl.length == 0)
	{
		//kdDebug(14150) << k_funcinfo << "Error reading FLAP" << endl;
		return;
	}

	buf = new char[fl.length];

	bytesread = mSocket->readBlock(buf, fl.length);
	if(bytesread != fl.length)
	{
		kdDebug(14150) << k_funcinfo << "WARNING, couldn't read all of" \
			" that packet, this will probably break things!!!" << endl;
	}

	inbuf.setBuf(buf, bytesread);

#ifdef OSCAR_PACKETLOG
	kdDebug(14150) << "=== INPUT ===" << inbuf.toString();
#endif

	//kdDebug(14150) << "FLAP channel is " << fl.channel << endl;

	switch(fl.channel)
	{
		case 0x01: //new connection negotiation channel
		{
			DWORD flapversion=inbuf.getDWord();
			if (flapversion == 0x00000001)
			{
				emit connAckReceived();
			}
			else
			{
				kdDebug(14150) << k_funcinfo <<
					"!Could not read FLAP version on channel 0x01" << endl;
				break;
			}
			break;
		} // END 0x01

		case 0x02: //snac data channel
		{
			SNAC s;
			s=inbuf.getSnacHeader();

			if (s.family != 3 && s.subtype != 11) // avoid the spam of all the online notifies
			{
				kdDebug(14150) << k_funcinfo << "SNAC(" << s.family << "," <<
					s.subtype << "), id=" << s.id << endl;
			}

			switch(s.family)
			{
				case OSCAR_FAM_1: // Service Controls
				{
					switch(s.subtype)
					{
						case 0x0001:
							kdDebug(14150) << k_funcinfo << "SNAC Family 1 error" << endl;
							parseError(s.family, s.id, inbuf);
							break;
						case 0x0003: //server ready
							parseServerReady(inbuf);
							break;
						case 0x0005: //redirect
							kdDebug(14150) << k_funcinfo <<
								"TODO: Parse redirect!" << endl;
							//parseRedirect(inbuf);
							break;
						case 0x0007: //rate info request response, SRV_RATES
							parseRateInfoResponse(inbuf);
							break;
						case 0x000f: //my user info
							parseMyUserInfo(inbuf);
							break;
						case 0x000a: //rate change
							parseRateChange(inbuf);
							break;
						case 0x0010: //warning notification
							parseWarningNotify(inbuf);
							break;
						case 0x0013: //message of the day
							parseMessageOfTheDay(inbuf);
							break;
						case 0x0018: //server versions
							parseServerVersions(inbuf);
							break;
						case 0x001f: //requests a memory segment, part of aim.exe  EVIL AOL!!!
							parseMemRequest(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC(" <<
								s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END OSCAR_FAM_1

				case OSCAR_FAM_2: // Location service
				{
					switch(s.subtype)
					{
						case 0x0001:
							kdDebug(14150) << k_funcinfo << "SNAC Family 2 error" << endl;
							parseError(s.family, s.id, inbuf);
							break;
						case 0x0003: //locate rights
							parseLocateRights(inbuf);
							break;
						case 0x0006: //user info (AIM)
							parseUserLocationInfo(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC(" <<
								s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END 0x0002

				case OSCAR_FAM_3: // Contact services
				{
					switch(s.subtype)
					{
						case 0x0001: // contact list error (SRV_CONTACTERR)
							kdDebug(14150) << k_funcinfo << "SNAC Family 3 error" << endl;
							kdDebug(14150) << k_funcinfo <<
								"RECV SRV_CONTACTERR, UNHANDLED!!!" << endl;
							break;
						case 0x0003: //buddy list rights
							parseBuddyRights(inbuf);
							break;
						case 0x000a: // server refused adding contact to list (SRV_REFUSED)
							kdDebug(14150) << k_funcinfo <<
								"RECV SRV_REFUSED, UNHANDLED!!!" << endl;
							break;
						case 0x000b: //contact changed status, (SRV_USERONLINE)
							parseUserOnline(inbuf);
							break;
						case 0x000c: //contact went offline
							parseUserOffline(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC(" <<
								s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END 0x0003

				case OSCAR_FAM_4: //msg services
				{
					switch(s.subtype)
					{
						case 0x0001: //msg error
							kdDebug(14150) << k_funcinfo << "SNAC Family 4 (ICBM) error" << endl;
							parseError(s.family, s.id, inbuf);
							break;
						case 0x0005: //msg rights SNAC(4,5)
							parseMsgRights(inbuf);
							break;
						case 0x0007: //incoming IM SNAC(4,7)
							parseIM(inbuf);
							break;
						case 0x000a: //missed messages SNAC(4,10)
							parseMissedMessage(inbuf);
							break;
						case 0x000b: // message ack SNAC(4,11)
							parseMsgAck(inbuf);
							break;
						case 0x000c: // server ack for type-2 message SNAC(4,12)
							parseSrvMsgAck(inbuf);
							break;
						case 0x0014: // Mini-Typing notification
							parseMiniTypeNotify(inbuf);
							break;
						default: //invalid subtype
							kdDebug(14150) << k_funcinfo << "Unknown SNAC("
								<< s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END OSCAR_FAM_4

				case OSCAR_FAM_9: // BOS
				{
					switch(s.subtype)
					{
						case 0x0001: //BOS error
							kdDebug(14150) << k_funcinfo << "SNAC Family 9 (BOS) error" << endl;
							parseError(s.family, s.id, inbuf);
							break;
						case 0x0003: //bos rights incoming
							parseBOSRights(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC("
								<< s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END OSCAR_FAM_9

				case OSCAR_FAM_19: // Contact list management
				{
					switch(s.subtype)
					{
						case 0x0003: //ssi rights
							parseSSIRights(inbuf);
							break;
						case 0x0006: //buddy list
							parseSSIData(inbuf);
							break;
						case 0x000e: //server ack for adding/changing roster items
							parseSSIAck(inbuf, s.id);
							break;
						case 0x000f: //ack when contactlist timestamp/length matches those values sent
							parseSSIOk(inbuf);
							break;
						case 0x001b: // auth reply
							parseAuthReply(inbuf);
							break;
						case 0x001c: // "added by" message
							kdDebug(14150) << k_funcinfo << "IGNORE 'added by' message" << endl;
							break;
						default: //invalid subtype
							kdDebug(14150) << k_funcinfo << "Unknown SNAC("
								<< s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END OSCAR_FAM_19

				case OSCAR_FAM_21: // ICQ 0x15 packets
				{
					switch(s.subtype)
					{
						case 0x0001:
							parseError(s.family, s.id, inbuf);
							break;
						case 0x0003:
							parseSRV_FROMICQSRV(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC("
								<< s.family << ",|" << s.subtype << "|)" << endl;
					}
					break;
				} // END OSCAR_FAM_21

				case OSCAR_FAM_23: //authorization family, TODO: also for icq registration
				{
					switch(s.subtype)
					{
						case 0x0001: //registration refused!
							emit protocolError(i18n("Registration refused. Check your user name and password and try again"), 0);
							break;
						case 0x0003: //authorization response (and hash) is being sent
							parseAuthResponse(inbuf);
							break;
						case 0x0007: //encryption key is being sent
							parsePasswordKey(inbuf);
							break;
						default:
							kdDebug(14150) << k_funcinfo << "Unknown SNAC(" << s.family << ",|" << s.subtype << "|)" << endl;
					};
					break;
				} // END OSCAR_FAM_23

				default:
					kdDebug(14150) << k_funcinfo << "Unknown SNAC(|" << s.family << "|," << s.subtype << ")" << endl;

			}; // END switch (s.family)
			break;
		} // END channel 0x02

		case 0x03: //FLAP error channel
		{
			kdDebug(14150) << "FLAP error channel, UNHANDLED" << endl;
			break;
		} // END channel 0x03

		case 0x04: //close connection negotiation channel
		{
			parseConnectionClosed(inbuf);
			break;
		}

		case 0x05:
		{
			kdDebug(14150) << k_funcinfo << "RECV KEEPALIVE" << endl;
			break;
		}

		default: //oh, crap, something is really wrong
		{
			kdDebug(14150) << k_funcinfo << "Unknown channel " << fl.channel << endl;
		}

	} // END switch(fl.channel)

	inbuf.clear(); // separate buf from inbuf again
	delete [] buf;

	// another flap is waiting in read buffer
	if (mSocket->bytesAvailable() > 0)
		slotRead();
}

void OscarSocket::sendLoginRequest()
{
	kdDebug(14150) << k_funcinfo << "Called" << endl;
	Buffer outbuf;
	outbuf.addSnac(OSCAR_FAM_23,0x0006,0x0000,0x00000000);
	outbuf.addTLV(0x0001, getSN().length(), getSN().latin1());
	sendBuf(outbuf,0x02);
//	emit connectionChanged(2,QString("Requesting login for " + getSN() + "..."));
}

void OscarSocket::putFlapVer(Buffer &outbuf)
{
	outbuf.addDWord(0x00000001);
}

void OscarSocket::OnConnAckReceived()
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;
	if(mIsICQ)
	{
		kdDebug(14150) << k_funcinfo << "ICQ-LOGIN, sending ICQ login" << endl;
		sendLoginICQ();
	}
	else
	{
		kdDebug(14150) << k_funcinfo << "AIM-LOGIN, Sending flap version to server" << endl;
		Buffer outbuf;
		putFlapVer(outbuf);
		sendBuf(outbuf,0x01);
		sendLoginRequest();
	}
}

void OscarSocket::sendBuf(Buffer &outbuf, BYTE chan)
{

	//For now, we use 0 as the sequence number because rate
	//limiting can cause packets from different classes to be
	//sent out in different order
	outbuf.addFlap(chan, 0);

	//Read SNAC family/type from buffer if able
	SNAC s = outbuf.readSnacHeader();

	//if the snac was read without a problem, find its rate class
	if ( !s.error )
	{
		//Pointer to proper rate class
		RateClass *rc = 0L;

		//Determine rate class
		for ( RateClass *tmp=rateClasses.first(); tmp; tmp = rateClasses.next() )
		{
			if ( tmp->isMember(s) )
			{
				kdDebug(14150) << k_funcinfo << "Rate class (id=" << tmp->id() <<
					") found: SNAC(" << s.family << "," << s.subtype << ")" << endl;
				rc = tmp;
				break;
			}
		}

		if ( rc )
			rc->enqueue(outbuf);
		else
			writeData(outbuf);
	}
	else
	{
		writeData(outbuf);
	}
}

/* Writes the next packet in the queue onto the wire */
void OscarSocket::writeData(Buffer &outbuf)
{
	//Update packet sequence number
	outbuf.changeSeqNum(flapSequenceNum);
	flapSequenceNum++;

	if(socketStatus() != OscarConnection::Connected)
	{
		kdDebug(14150) << k_funcinfo <<
			"Socket is NOT open, can't write to it right now" << endl;
		return;
	}

	kdDebug(14150) << k_funcinfo << "Writing data" << endl;
#ifdef OSCAR_PACKETLOG
	kdDebug(14150) << "--- OUTPUT ---" << outbuf.toString() << endl;
#endif

	//actually put the data onto the wire
	if(mSocket->writeBlock(outbuf.buffer(), outbuf.length()) == -1)
	{
		kdDebug(14150) << k_funcinfo << "writeBlock() call failed!" << endl;
		/*
		kdDebug(14150) << k_funcinfo <<
		mSocket->strError(mSocket->socketStatus(), mSocket->systemError()) << endl;
		*/
	}

	if ( sender() && sender()->isA("RateClass") )
		((RateClass *)sender())->dequeue();
}

// Logs in the user!
void OscarSocket::doLogin(
	const QString &host, int port,
	const QString &name, const QString &password,
	const QString &userProfile, const unsigned long initialStatus,
	const QString &/*awayMessage*/)
{
	QString realHost = host;

	if (isLoggedIn)
	{
		kdDebug(14150) << k_funcinfo << "We're already connected, aborting!" << endl;
		return;
	}
	if(realHost.isEmpty())
	{
		kdDebug(14150) << k_funcinfo << " Tried to connect without a hostname!" << endl;
		kdDebug(14150) << k_funcinfo << "Using login.oscar.aol.com" << endl;
		realHost = QString::fromLatin1("login.oscar.aol.com");
	}
	if(port < 1)
	{
		kdDebug(14150) << k_funcinfo << " Tried to connect to port < 1! Using port 5190" << endl;
		port = 5190;
	}
	if(password.isEmpty())
	{
		kdDebug(14150) << k_funcinfo << " Tried to connect without a password!" << endl;
		return;
	}

	kdDebug(14150) << k_funcinfo << "Connecting to '" << host << "', port=" << port << endl;

	QObject::disconnect(this, SIGNAL(socketConnected(const QString &)),
		this, SLOT(OnBosConnect()));
	QObject::disconnect(this, SIGNAL(connAckReceived()),
		this, SLOT(OnBosConnAckReceived()));

	QObject::connect(this, SIGNAL(socketConnected(const QString &)),
		this, SLOT(slotConnected()));
	QObject::connect(this, SIGNAL(connAckReceived()),
		this, SLOT(OnConnAckReceived()));

	setSN(name);
	loginPassword=password;
	loginProfile=userProfile;
	loginStatus=initialStatus;

	/*kdDebug(14150) << k_funcinfo <<
		"emitting statusChanged(OSCAR_CONNECTING)" << endl;*/
	emit statusChanged(OSCAR_CONNECTING);

	connectTo(realHost, QString::number(port));
}

void OscarSocket::parsePasswordKey(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "Got the key" << endl;

	WORD keylen = inbuf.getWord();
	delete[] mPwEncryptionKey;
	mPwEncryptionKey = inbuf.getBlock(keylen);
	sendLoginAIM();
}

void OscarSocket::connectToBos()
{
	kdDebug(14150) << k_funcinfo <<
		"Cookie received! Connecting to BOS server..." << endl;

//	emit connectionChanged(4,"Connecting to server...");
	QObject::disconnect(this, SIGNAL(connAckReceived()),
		this, SLOT(OnConnAckReceived()));
	QObject::disconnect(this, SIGNAL(socketConnected(const QString &)),
		this, SLOT(slotConnected()));

	// Disconnect from socketClosed signal so we do not set our
	// account status (and thus our icon) from connecting to offline
	QObject::disconnect(this, SIGNAL(socketClosed(const QString &)),
		this, SLOT(slotConnectionClosed()));

	//mSocket->close();
	mSocket->reset();

	QObject::connect(this, SIGNAL(connAckReceived()),
		this, SLOT(OnBosConnAckReceived()));
	QObject::connect(this, SIGNAL(socketConnected(const QString &)),
		this, SLOT(OnBosConnect()));

	// Reconnect socketClosed status to track if we (were) disconnected
	// from the main BOS server
	QObject::connect(this, SIGNAL(socketClosed(const QString &)),
		this, SLOT(slotConnectionClosed()));

	connectTo(bosServer, QString::number(bosPort));
}

void OscarSocket::OnBosConnAckReceived()
{
	kdDebug(14150) << k_funcinfo <<
		"BOS server ack'ed us! Sending auth cookie" << endl;

	sendCookie();
//	emit connectionChanged(5,"Connected to server, authorizing...");
}

void OscarSocket::sendCookie()
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_COOKIE)" << endl;
	Buffer outbuf;
	putFlapVer(outbuf);
	outbuf.addTLV(0x0006, mCookieLength, mCookie);
	sendBuf(outbuf, 0x01);
}

// SNAC(01,06)
// http://iserverd.khstu.ru/oscar/snac_01_06.html
void OscarSocket::sendRateInfoRequest(void)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_RATESREQUEST)" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0006,0x0000,0x00000006);
	sendBuf(outbuf,0x02);
}

// SNAC(01,07)
// http://iserverd.khstu.ru/oscar/snac_01_07.html
void OscarSocket::parseRateInfoResponse(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo <<
		"RECV (SRV_RATES), Parsing Rate Info Response" << endl;

	RateClass *rc = 0;
	//SnacPair *snacPair = 0;
	unsigned int i = 0;
	WORD numclasses = inbuf.getWord();

	/*kdDebug(14150) << k_funcinfo <<
	"Number of Rate Classes=" << numclasses << endl;*/

	for (i=0; i<numclasses; i++)
	{
		WORD classid;
		DWORD windowsize, clear, alert, limit, disconnect,
			current, max, lastTime;
		BYTE currentState;

		rc = new RateClass;

		classid = inbuf.getWord();
		//kdDebug(14150) << k_funcinfo << "Rate classId=" << rc->classid << endl;
		windowsize = inbuf.getDWord();
		clear = inbuf.getDWord();
		alert = inbuf.getDWord();
		limit = inbuf.getDWord();
		disconnect = inbuf.getDWord();
		current = inbuf.getDWord();
		max = inbuf.getDWord();
		// TODO: From http://iserverd.khstu.ru/oscar/snac_01_07.html
		// Last time  -  not present in protocol version 2
		// Current state  -  not present in protocol version 2
		// i.e. find out when protocol version 2 is used (if it
		// isn't obsolete already)
		lastTime = inbuf.getDWord();
		currentState = inbuf.getByte();

		rc->setRateInfo(classid, windowsize, clear, alert, limit,
			disconnect, current, max, lastTime, currentState);
		QObject::connect(rc, SIGNAL(dataReady(Buffer &)),
			this, SLOT(writeData(Buffer &)));
		rateClasses.append(rc);
	}

#ifdef OSCAR_PACKETLOG
	kdDebug(14150) << "REMAINING BUFFER:" << inbuf.toString() << endl;
#endif

	//now here come the members of each class
	for (i=0; i<numclasses; i++)
	{
		WORD classid = inbuf.getWord();
		WORD count = inbuf.getWord();

//		kdDebug(14150) << k_funcinfo << "Classid: " << classid <<
//			", Count: " << count << endl;

		RateClass *tmp;
		rc = 0;
		// find the class we're talking about
		for (tmp=rateClasses.first(); tmp; tmp=rateClasses.next())
		{
			if (tmp->id() == classid)
			{
				rc = tmp;
				break;
			}
		}

		// do not go into next for() loop if we didnt find a RateClass anyway
		if (rc == 0)
		{
			kdDebug(14150) << k_funcinfo << "WARNING: Could not find RateClass" <<
				"to store SNAC group/type pairs into. Missing class id = " <<
				classid << endl;
			continue;
		}

		for (WORD j=0; j<count; j++)
		{
			/*
			snacPair = new SnacPair;
			snacPair->group = inbuf.getWord();
			snacPair->type = inbuf.getWord();
			if (rc)
				rc->addMember(s);
			*/

			WORD group = inbuf.getWord();
			WORD type = inbuf.getWord();
			rc->addMember(group, type);
		}
	}

	if(inbuf.length() != 0)
	{
		kdDebug(14150) << k_funcinfo <<
			"ERROR: Did not parse all Rates successfully!" << endl;
	}

	sendRateAck();
}

void OscarSocket::sendRateAck()
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_ACKRATES)" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0008,0x0000,0x00000000);
	for (RateClass *rc = rateClasses.first(); rc; rc = rateClasses.next())
	{
		/*kdDebug(14150) << k_funcinfo << "adding classid " << rc->id() <<
			 " to the rate acknowledgement" << endl;*/
		outbuf.addWord(rc->id());
	}
	sendBuf(outbuf,0x02);

	//request our user info after sending the rate ack
	requestInfo();
}

void OscarSocket::OnBosConnect()
{
	kdDebug(14150) << k_funcinfo << "Connected to " << peerHost() <<
		", port " << peerPort() << endl;
}

void OscarSocket::sendPrivacyFlags()
{
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0014,0x0000,0x00000000);
	//bit 1: let others see idle time
	//bit 2: let other see member since
	//TODO: Make this configurable
	outbuf.addDWord(0x00000003);
	sendBuf(outbuf,0x02);
}

void OscarSocket::requestMyUserInfo()
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQINFO)" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x000e,0x0000,0x00000000);
	sendBuf(outbuf,0x02);
}

void OscarSocket::parseMyUserInfo(Buffer &inbuf)
{
	if (gotAllRights > 7)
	{
		kdDebug(14150) << k_funcinfo "RECV (SRV_REPLYINFO) Parsing OWN user info" << endl;
		UserInfo u;
		parseUserInfo(inbuf, u);
		emit gotContactChange(u); // gotMyUserInfo(u);
		return;
	}

	kdDebug(14150) << k_funcinfo <<
		"RECV (SRV_REPLYINFO) Ignoring OWN user info, gotAllRights=" <<
		gotAllRights << endl;

	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}

void OscarSocket::parseAuthResponse(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;

	QPtrList<TLV> lst = inbuf.getTLVList();
	lst.setAutoDelete(TRUE);
	TLV *sn = findTLV(lst,0x0001);  //screen name
	TLV *url = findTLV(lst,0x0004);  //error url
	TLV *bosip = findTLV(lst,0x0005); //bos server address
	TLV *cook = findTLV(lst,0x0006); //authorization cookie
	//TLV *email = findTLV(lst,0x0007); //the email address attached to the account
	//TLV *regstatus = findTLV(lst,0x0013); //whether the email address is available to others
	TLV *err = findTLV(lst,0x0008); //whether an error occurred
	TLV *passChangeUrl = findTLV(lst,0x0054); //Password change URL, TODO: use for AIM accounts

	if (passChangeUrl)
	{
		kdDebug(14150) << k_funcinfo << "password change url='"  << passChangeUrl->data << "'" << endl;
		delete [] passChangeUrl->data;
	}

	delete[] mCookie;

	if (err)
	{
		QString errorString;
		int errorCode = (err->data[0] << 8)|err->data[1];

		switch(errorCode)
		{
			case 1: { errorString = i18n("Sign on failed because the screen name you provided is not registered on the AIM network. Please visit http://aim.aol.com to create a screen name for use on the AIM network."); break;  }
			case 5: { errorString = i18n("Sign on failed because the password supplied for this screen name is invalid. Please check your password and try again."); break; }
			case 0x11: { errorString = i18n("Sign on failed because your account is currently suspended."); break; }
			case 0x14: { errorString = i18n("The AOL Instant Messenger service is temporarily unavailable. Please try again later."); break; }
			case 0x18: { errorString = i18n("You have been connecting and disconnecting too frequently. Wait ten minutes and try again. If you continue to try, you will need to wait even longer."); break; }
			case 0x1c: { errorString = i18n("The client you are using is too old. Please upgrade."); break; }
			default: { errorString = i18n("Authentication failed."); break; }
		}

		emit protocolError(errorString, errorCode);
		delete[] err->data;
	}

	if (bosip)
	{
		QString ip = bosip->data;
		int index;
		index = ip.find(':');
		bosServer = ip.left(index);
		ip.remove(0,index+1); //get rid of the colon and everything before it
		bosPort = ip.toInt();

		kdDebug(14150) << "server is " << bosServer <<
			", ip.right(index) is " << ip <<
			", bosPort is " << bosPort << endl;

		delete[] bosip->data;
	}

	if (cook)
	{
		mCookie=cook->data;
		mCookieLength=cook->length;
		connectToBos();
	}

	if (sn)
		delete [] sn->data;

	if (url)
		delete [] url->data;

	lst.clear();
}

/** finds a tlv of type typ in the list */
TLV * OscarSocket::findTLV(QPtrList<TLV> &l, WORD typ)
{
	TLV *t;
	for(t=l.first();t;t=l.next())
	{
		if (t->type == typ)
			return t;
	}
	return NULL;
}

/** tells the server that the client is ready to receive commands & stuff */
void OscarSocket::sendClientReady(void)
{
	kdDebug(14150) << "SEND (CLI_READY) sending client ready, end of login procedure." << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0002,0x0000,0x00000000);

	for (RateClass *rc=rateClasses.first();rc;rc=rateClasses.next())
	{
		/*kdDebug(14150) << "adding family '" << rc->classid <<
			"' to CLI_READY packet" << endl;*/

		outbuf.addWord(rc->id());

		if (rc->id() == 0x0001)
		{
			outbuf.addWord(0x0003);
		}
		else if (rc->id() == 0x0013)
		{
			if(mIsICQ)
				outbuf.addWord(0x0002);
			else
				outbuf.addWord(0x0003); // AIM supports Family 19 at version 3 already
		}
		else
		{
			outbuf.addWord(0x0001);
		}

		if(mIsICQ)
		{
			if(rc->id() == 0x0002)
				outbuf.addWord(0x0101);
			else
				outbuf.addWord(0x0110);
			outbuf.addWord(0x047b);
		}
		else // AIM
		{
			if (rc->id() == 0x0008 ||
				rc->id() == 0x000b ||
				rc->id() == 0x000c)
			{
				outbuf.addWord(0x0104);
				outbuf.addWord(0x0001);
			}
			else
			{
				outbuf.addWord(0x0110);
				outbuf.addWord(0x059b);
			}
		}
	}
	sendBuf(outbuf,0x02);

	kdDebug(14150) << "============================================================================" << endl;
	kdDebug(14150) << "============================================================================" << endl;

	isLoggedIn = true;
	emit loggedIn();
}

void OscarSocket::sendVersions(const WORD *families, const int len)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_FAMILIES)" << endl;
	WORD val;
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0017,0x0000,0x00000000);

	for(int i=0;i<len;i++)
	{
		outbuf.addWord(families[i]);
		if(families[i]==0x0001)
		{
			val=0x0003;
		}
		else if (families[i]==0x0013)
		{
			if(mIsICQ)
				val=0x0004; // for ICQ2002
			else
				val=0x0003;
		}
		else
			val=0x0001;

//		kdDebug(14150) << k_funcinfo << "family=" << families[i] << ", val=" << val << endl;
		outbuf.addWord(val);
	}
	sendBuf(outbuf,0x02);

}

void OscarSocket::sendIdleTime(DWORD time)
{
	if (!isLoggedIn)
		return;

	kdDebug(14150) << k_funcinfo "SEND (CLI_SNAC1_11), sending idle time, time=" << time << endl;
	bool newidle = (time!=0);
	if (newidle != idle) //only do stuff if idle status changed
	{
		idle = newidle;
		Buffer outbuf;
		outbuf.addSnac(0x0001,0x0011,0x0000,0x00000000);
		outbuf.addDWord(time);
		sendBuf(outbuf,0x02);
	}
}


void OscarSocket::requestBOSRights(void)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQBOS) Requesting BOS rights" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0009,0x0002,0x0000,0x00000002);
	sendBuf(outbuf,0x02);
}

void OscarSocket::parseBOSRights(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_REPLYBOS) " << endl;

	QPtrList<TLV> ql = inbuf.getTLVList();
	ql.setAutoDelete(TRUE);
	TLV *t;
	WORD maxpermits = 0, maxdenies = 0;
	if ((t = findTLV(ql,0x0001))) //max permits
		maxpermits = (t->data[0] << 8) | t->data[1];
	if ((t = findTLV(ql,0x0002))) //max denies
		maxdenies = (t->data[0] << 8) | t->data[1];
//	kdDebug(14150) << k_funcinfo << "maxpermits=" << maxpermits << ", maxdenies=" << maxdenies << endl;
	ql.clear();
	//sendGroupPermissionMask();
	//sendPrivacyFlags();

	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}

void OscarSocket::parseServerReady(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_FAMILIES), got list of families" << endl;

	int famcount; //the number of families received
	//WORD *families = new WORD[inbuf.length()];
	WORD *families = new WORD[(int)inbuf.length()/2];
	for (famcount=0; inbuf.length() > 1; famcount++)
	{
		families[famcount] = inbuf.getWord();
	}

	sendVersions(families, famcount); // send back a CLI_FAMILIES packet
	//emit serverReady(); // What is this exactly used for? [mETz]
	delete [] families;
}

/** parses server version info */
void OscarSocket::parseServerVersions(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_FAMILIES2), got list of families this server understands" << endl;

	int srvFamCount;
	for (srvFamCount=0; inbuf.length(); srvFamCount++)
	{
		kdDebug(14150) << k_funcinfo << "server family=" << inbuf.getWord() <<
			", server version=" << inbuf.getWord() << endl;
	}

	//The versions are not important to us at all
	//now we can request rates
	sendRateInfoRequest(); // CLI_RATESREQUEST
}

/** Parses Message of the day */
void OscarSocket::parseMessageOfTheDay(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_MOTD)" << endl;
	WORD id = inbuf.getWord();
	if (id < 4)
	{
		emit protocolError(i18n(
			"An unknown error occurred. Your connection may be lost. " \
			"The error was: \"AOL MOTD Error: your connection may be lost. ID: %1\"").arg(id), 0);
	}
}

// Requests location rights (CLI_REQLOCATION)
void OscarSocket::requestLocateRights(void)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQLOCATION), Requesting rights for location service" << endl;
	Buffer buf;
//	buf.addSnac(0x0002,0x0002,0x0000,0x00000002);
	buf.addSnac(0x0002,0x0002,0x0000,0x00000000);
	sendBuf(buf,0x02);
}

/** Requests a bunch of information (permissions, rights, my user info, etc) from server */
void OscarSocket::requestInfo()
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;
	requestMyUserInfo(); // CLI_REQINFO
	sendSSIRightsRequest();  // CLI_REQLISTS
	sendRosterRequest(); // CLI_CHECKROSTER
	requestLocateRights(); // CLI_REQLOCATION
	requestBuddyRights(); // CLI_REQBUDDY
	requestMsgRights(); // CLI_REQICBML
	requestBOSRights(); // CLI_REQBOS
	kdDebug(14150) << k_funcinfo << "resetting gotAllRights to 0!" << endl;
	gotAllRights=0;
	// next received packet should be a SRV_REPLYINFO
}

/** adds a mask of the groups that you want to be able to see you to the buffer */
void OscarSocket::sendGroupPermissionMask(void)
{
	Buffer outbuf;
	outbuf.addSnac(0x0009,0x0004,0x0000,0x00000000);
	outbuf.addDWord(0x0000001f);
	sendBuf(outbuf,0x02);
}

// adds a request for buddy list rights to the buffer
void OscarSocket::requestBuddyRights(void)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQBUDDY), Requesting rights for buddy service" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0003,0x0002,0x0000,0x00000000);
	sendBuf(outbuf,0x02);
}

// adds a request for msg rights to the buffer
void OscarSocket::requestMsgRights(void)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQICBM), Requesting rights for ICBM (instant messages)" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0004,0x0004,0x0000,0x00000000);
	sendBuf(outbuf,0x02);
}

// Parses the locate rights provided by the server (SRV_REPLYLOCATION)
void OscarSocket::parseLocateRights(Buffer &/*inbuf*/)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_REPLYLOCATION), TODO: Ignoring location rights" << endl;
	//we don't care what the locate rights are
	//and we don't know what they mean
	//  requestBuddyRights();

	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}

void OscarSocket::parseBuddyRights(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_REPLYBUDDY), TODO: Ignoring Buddy Rights" << endl;
	// TODO: use these values if possible

	while(1)
	{
		TLV t = inbuf.getTLV();
		if(t.data == 0L)
			break;

		Buffer tlvBuf(t.data, t.length);
		switch(t.type)
		{
			case 0x0001:
				kdDebug(14150) << k_funcinfo <<
					"max contactlist size     = " << tlvBuf.getWord() << endl;
				break;
			case 0x0002:
				kdDebug(14150) << k_funcinfo <<
					"max no. of watchers      = " << tlvBuf.getWord() << endl;
				break;
			case 0x0003:
				kdDebug(14150) << k_funcinfo <<
					"max online notifications = " << tlvBuf.getWord() << endl;
				break;
			default:
				break;
		}
		tlvBuf.clear();
	}

	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}

void OscarSocket::parseMsgRights(Buffer &inbuf)
{
	// docu: http://iserverd.khstu.ru/oscar/snac_04_05.html

	kdDebug(14150) << k_funcinfo << "RECV (SRV_REPLYICBM) Parsing ICBM rights" << endl;

	WORD channel = inbuf.getWord();
	kdDebug(14150) << k_funcinfo << "channel=" << channel << endl;

	DWORD messageFlags = inbuf.getDWord();
	kdDebug(14150) << k_funcinfo << "messageFlags=" << messageFlags << endl;
	/*
bit1: messages allowed for specified channel
bit2: missed calls notifications enabled for specified channel
bit4: client supports typing notifications
	*/

	WORD maxMessageSnacSize = inbuf.getWord();
	kdDebug(14150) << k_funcinfo << "maxMessageSnacSize=" << maxMessageSnacSize << endl;

	WORD maxSendWarnLvl = inbuf.getWord(); // max sender Warning Level
	kdDebug(14150) << k_funcinfo << "maxSendWarnLvl=" << maxSendWarnLvl << endl;

	WORD maxRecvWarnLvl = inbuf.getWord(); // max Receiver Warning Level
	kdDebug(14150) << k_funcinfo << "maxRecvWarnLvl=" << maxRecvWarnLvl << endl;

	WORD minMsgInterval = inbuf.getWord(); // minimum message interval (msec)
	kdDebug(14150) << k_funcinfo << "minMsgInterval=" << minMsgInterval << endl;

	/*WORD unknown = */inbuf.getWord();

	// After we get this from the server
	// we have to send some messaging parameters
	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}

void OscarSocket::parseIM(Buffer &inbuf)
{
	QByteArray cook(8);
/*
	WORD type = 0;
	WORD length = 0;
*/
	//This is probably the hardest thing to do in oscar
	//first comes an 8 byte ICBM cookie (random)
	// from icq docs:
	// this value is
	// ((time(NULL) - (8*60*60)) + DayOfWeek*60*60*24)) * 1500
	inbuf.getBlock(8);

	// Channel ID.
	//
	// Channel 0x0001 is the message channel.  There are
	// other channels for things called "rendevous"
	// which represent chat and some of the other new
	// features of AIM2/3/3.5.
	//
	// Channel 0x0002 is the Rendevous channel, which
	// is where Chat Invitiations and various client-client
	// connection negotiations come from.
	//
	// Channel 0x0004 is used for ICQ authorization, or
	// possibly any system notice.
	WORD channel = inbuf.getWord();

	// Extract the standard user info block.
	//
	// Note that although this contains TLVs that appear contiguous
	// with the TLVs read below, they are two different pieces.  The
	// userinfo block contains the number of TLVs that contain user
	// information, the rest are not even though there is no separation.
	//
	// That also means that TLV types can be duplicated between the
	// userinfo block and the rest of the message, however there should
	// never be two TLVs of the same type in one block.
	UserInfo u;
	parseUserInfo(inbuf, u);

	switch(channel)
	{
		case MSGFORMAT_SIMPLE: //normal IM
		{
			parseSimpleIM(inbuf, u);
			break;
		}; // END MSGFORMAT_SIMPLE

		case MSGFORMAT_ADVANCED: //AIM rendezvous, ICQ advanced messages
		{
			if (mIsICQ) // TODO: unify AIM and ICQ in this place
			{
				kdDebug(14150) << k_funcinfo << "IM received on channel 2 from '" << u.sn << "'" << endl;
				TLV tlv5tlv = inbuf.getTLV();
				kdDebug(14150) << k_funcinfo << "The first TLV is of type " << tlv5tlv.type << endl;
				if (tlv5tlv.type != 0x0005)
				{
					kdDebug(14150) << k_funcinfo << "Aborting because first TLV != TLV(5)" << endl;
					break;
				}

				Buffer tlv5(tlv5tlv.data, tlv5tlv.length);

				/*WORD ackType =*/ tlv5.getWord();
				DWORD msgTime = tlv5.getDWord();
				DWORD msgRandomId = tlv5.getDWord();
				char *capData = tlv5.getBlock(16);
				DWORD capFlag = parseCap(capData);
				delete [] capData;

				QPtrList<TLV> lst = tlv5.getTLVList();
				lst.setAutoDelete(TRUE);

				TLV *msgTLV = findTLV(lst,0x2711);  //message tlv
				if(!msgTLV)
				{
					kdDebug(14150) << k_funcinfo << "Aborting because TLV(10001) wasn't found (no message?)" << endl;
					break;
				}

				switch(capFlag)
				{
					case AIM_CAPS_ISICQ:
						// found in direct-connection stuff?
						break;

					case AIM_CAPS_ICQSERVERRELAY:
					{
						Buffer messageBuf(msgTLV->data, msgTLV->length);
						WORD len = messageBuf.getLEWord();
						if (len != 0x001b)
							kdDebug(14150) << k_funcinfo << "wrong len till SEQ1!" << endl;
						WORD tcpVer = messageBuf.getLEWord();
						//kdDebug(14150) << k_funcinfo << "len=" << len << ", tcpver=" << tcpVer << endl;
						char *cap=messageBuf.getBlock(16);
						WORD unk1 = messageBuf.getLEWord();
						DWORD unk2 = messageBuf.getLEDWord();
						BYTE unk3 = messageBuf.getLEByte();
						WORD seq1 = messageBuf.getLEWord();


						Buffer ack; // packet sent back as acknowledgment
						ack.addSnac(4, 11, 0, 0);
						ack.addDWord(msgTime);
						ack.addDWord(msgRandomId);
						ack.addWord(0x0002); // type-2 ack
						ack.addBUIN(u.sn.latin1());
						ack.addWord(0x0003); // unknown
						ack.addLEWord(len);
						ack.addLEWord(tcpVer);
						ack.addString(cap, 16);
						ack.addLEWord(unk1);
						ack.addLEDWord(unk2);
						ack.addLEByte(unk3);
						ack.addLEWord(seq1);

						parseAdvanceMessage(messageBuf, u, ack);
						break;
					}
					default: // TODO
						break;
				} // END switch(capFlag)

				break;
			}
			else
			{

				// ===========================================
				// TODO: unify AIM and ICQ parts
				// ===========================================

				unsigned int remotePort = 0;
				QString qh;
				QString message;
				WORD msgtype = 0x0000; //used to tell whether it is a direct IM requst, deny, or accept
				DWORD capflag = 0x00000000; //used to tell what kind of rendezvous this is
				/*OncomingSocket *sockToUse;*/ //used to tell which listening socket to use
				QString fileName; //the name of the file to be transferred (if any)
				long unsigned int fileSize = 0; //the size of the file(s) to be transferred

				kdDebug(14150) << k_funcinfo << "IM received on channel 2 from " << u.sn << endl;
				TLV tlv = inbuf.getTLV();
				kdDebug(14150) << k_funcinfo << "The first TLV is of type " << tlv.type;
				if (tlv.type == 0x0005) //connection info
				{
					Buffer tmpbuf(tlv.data, tlv.length);
					//embedded in the type 5 tlv are more tlv's
					//first 2 bytes are the request status
					// 0 - Request
					// 1 - Deny
					// 2 - Accept
					msgtype = tmpbuf.getWord();

					//next comes the cookie, which should match the ICBM cookie
					char *c = tmpbuf.getBlock(8);
					cook.duplicate(c,8);
					delete [] c;

					//the next 16 bytes are the capability block (what kind of request is this?)
					char *cap = tmpbuf.getBlock(0x10);
					capflag = parseCap(cap);
					if (capflag == 0x00000000)
						kdDebug(14150) << k_funcinfo << "unknown CAP: " << CapToString(cap) << endl;
					delete [] cap;

					//Next comes a big TLV chain of stuff that may or may not exist
					QPtrList<TLV> tlvlist = tmpbuf.getTLVList();
					TLV *cur;
					tlvlist.setAutoDelete(true);

					for(cur = tlvlist.first();cur;cur = tlvlist.next())
					{
						if (cur->type == 0x0002)
						{
							//IP address from the perspective of the client
							kdDebug(14150) << "ClientIP1: " << cur->data[0] << "."
								<< cur->data[1] << "." << cur->data[2] << "."
								<< cur->data[3]  << endl;
						}
						else if (cur->type == 0x0003)
						{
							//Secondary IP address from the perspective of the client
								kdDebug(14150) << "ClientIP2: " << cur->data[0] << "."
									<< cur->data[1] << "." << cur->data[2] << "."
									<< cur->data[3] << endl;
						}
						else if (cur->type == 0x0004) //Verified IP address (from perspective of oscar server)
						{
							DWORD tmpaddr = 0;
							for (int i=0;i<4;i++)
							{
								tmpaddr = (tmpaddr*0x100) + static_cast<unsigned char>(cur->data[i]);
							}
							qh = cur->data[0] + '.' + cur->data[1] + '.' + cur->data[2] + '.' + cur->data[3];
							kdDebug(14150) << "OscarIPRaw: " <<
								cur->data[0] << "." << cur->data[1] << "." <<
								cur->data[2] << "." << cur->data[3] << endl;
							kdDebug(14150) << "OscarIP: " << qh << endl;
						}
						else if (cur->type == 0x0005) //Port number
						{
							remotePort = (cur->data[0] << 8) | cur->data[1];
							kdDebug(14150) << k_funcinfo << "remotePort=" << remotePort << endl;
						}
						//else if (cur->type == 0x000a)
						//{
						//}
						//Error code
						else if (cur->type == 0x000b)
						{
							kdDebug(14150) << k_funcinfo << "ICBM ch 2 error code " <<
								((cur->data[1] << 8) | cur->data[0]) << endl;

							emit protocolError(
								i18n("Rendezvous with buddy failed. Please check your " \
									"internet connection or try the operation again later. " \
									"Error code %1.\n").arg((cur->data[1] << 8) | cur->data[0]), 0);
						}
						//Invitation message/ chat description
						else if (cur->type == 0x000c)
						{
							message = cur->data;
							kdDebug(14150) << k_funcinfo << "Invited to chat " << cur->data << endl;
						}
						//Character set
						else if (cur->type == 0x000d)
						{
							kdDebug(14150) << k_funcinfo << "Using character set " << cur->data << endl;
						}
						//Language
						else if (cur->type == 0x000e)
						{
							kdDebug(14150) << k_funcinfo << "Using language " << cur->data << endl;
						}
						//File transfer
						else if (cur->type == 0x2711)
						{
							Buffer thebuf(cur->data,cur->length);

							thebuf.getWord(); //more than 1 file? (0x0002 for multiple files)
							thebuf.getWord(); //number of files
							fileSize = thebuf.getDWord(); //total size
							char *fname = thebuf.getBlock(cur->length - 2 - 2 - 4 - 4); //file name
							thebuf.getDWord(); //DWord of 0x00000000
							fileName = fname;
							delete [] fname;
						} // END File transfer
						else
							kdDebug(14150) << k_funcinfo << "ICBM, unknown TLV type " << cur->type << endl;

						delete [] cur->data;
					} // END for (tlvlist...)
				}
				else
				{
					kdDebug(14150) << k_funcinfo << "IM: unknown TLV type " << tlv.type << endl;
				}

#if 0
				// Set the appropriate server socket
				sockToUse = serverSocket(capflag);

				if (msgtype == 0x0000) // initiate
				{
					kdDebug(14150) << k_funcinfo << "adding " << u.sn << " to pending list." << endl;
					if(capflag & AIM_CAPS_IMIMAGE) //if it is a direct IM rendezvous
					{
						sockToUse->addPendingConnection(u.sn, cook, 0L, qh, 4443, DirectInfo::Outgoing);
						emit gotDirectIMRequest(u.sn);
					}
					else // file send
					{
						sockToUse->addPendingConnection(u.sn, cook, 0L, qh, remotePort, DirectInfo::Outgoing);
						emit gotFileSendRequest(u.sn, message, fileName, fileSize);
					}
				}
				else if (msgtype == 0x0001) //deny
				{
					if(capflag & AIM_CAPS_IMIMAGE)
						emit protocolError(i18n("Direct IM request denied by %1").arg(u.sn),0);
					else
						emit protocolError(i18n("Send file request denied by %1").arg(QString(u.sn)),0);
					sockToUse->removeConnection(u.sn);
				}
#endif
			} // END if (mIsICQ)
			break;
		} // END MSGFORMAT_ADVANCED


		case MSGFORMAT_SERVER: // non-acknowledged, server messages (ICQ ONLY I THINK)
		{
			parseServerIM(inbuf, u);
			break;
		}; // END MSGFORMAT_SERVER

		default: // unknown channel
			kdDebug(14150) << "Error: unknown ICBM channel " << channel << endl;
	} // END switch(channel)
}



void OscarSocket::parseSimpleIM(Buffer &inbuf, const UserInfo &u)
{
	OscarContact *contact = static_cast<OscarContact*>(mAccount->contacts()[tocNormalize(u.sn)]);

	bool moreTLVs = true; // Flag to indicate if there are more TLV's to parse
	bool isAutoResponse = false; // This gets set if we are notified of an auto response
	WORD length = 0;

	kdDebug(14150) << k_funcinfo << "RECV TYPE-1 IM from '" << u.sn << "'" << endl;

	while(moreTLVs)
	{
		WORD type = inbuf.getWord();
		kdDebug(14150) << k_funcinfo << "TLV(" << type << ")" << endl;
		switch(type)
		{
			case 0x0002: //TLV(2), message block
			{
				// This is the total length of the rest of this message TLV
				length = inbuf.getWord();
				TLV caps = inbuf.getTLV(); // TLV(1281), CAPABILITIES
				if (caps.type == 1281)
				{
					//kdDebug(14150) << k_funcinfo << "TLV(1281), CAPABILITIES" << endl;
					Buffer capBuf(caps.data, caps.length);
					/*
					while(capBuf.length() > 0)
					{
						BYTE capPart = capBuf.getByte();
						kdDebug(14150) << k_funcinfo <<
							"capPart = '" << capPart << "'" << endl;

						if (capPart==0x06)
						{
							kdDebug(14150) << k_funcinfo <<
								"TLV(1281) says sender does UTF-8 :)" << endl;
						}
					}
					*/
					capBuf.clear();
				}
				delete [] caps.data;

				TLV tlvMessage = inbuf.getTLV(); // TLV(257), MESSAGE
				if(tlvMessage.type == 0x0101)
				{
					//kdDebug(14150) << k_funcinfo << "TLV(257), MESSAGE" << endl;
					Buffer msgBuf(tlvMessage.data, tlvMessage.length);

					WORD charsetNumber = msgBuf.getWord();
					/*WORD charsetSubset =*/ msgBuf.getWord();
					int messageLength = msgBuf.length();
					if (messageLength < 1)
						break;

					OscarMessage oMsg;

					if (charsetNumber == 0x0002) // UCS-2BE (or UTF-16)
					{
						//kdDebug(14150) << k_funcinfo << "UTF-16BE message" << endl;
						const unsigned short *txt = msgBuf.getWordBlock((int)messageLength/2);
						oMsg.setText(QString::fromUcs2(txt),
							mIsICQ ? OscarMessage::Plain : OscarMessage::AimHtml);
						delete [] txt;
					}
					else if (charsetNumber == 0x0003) // local encoding, usually iso8859-1
					{
						//kdDebug(14150) << k_funcinfo << "ISO8859-1 message" << endl;
						const char *messagetext = msgBuf.getBlock(messageLength);
						oMsg.setText(QString::fromLatin1(messagetext),
							mIsICQ ? OscarMessage::Plain : OscarMessage::AimHtml);
						delete [] messagetext;
					}
					else
					{ // BEGIN unknown or us-ascii
						/*kdDebug(14150) << k_funcinfo <<
							"Unknown encoding or US-ASCII, guessing encoding" << endl;*/
						const char *messagetext = msgBuf.getBlock(messageLength);
						oMsg.setText(ServerToQString(messagetext, contact, false),
							mIsICQ ? OscarMessage::Plain : OscarMessage::AimHtml);
						delete [] messagetext;
					} // END unknown or us-ascii


					parseMessage(u, oMsg, isAutoResponse ? MSG_AUTO : MSG_NORM, 0);

					msgBuf.clear();
				}
				else
				{
					kdDebug(14150) << k_funcinfo <<
						"Cannot find TLV(257), no message inside packet???" << endl;
				}

				//kdDebug(14150) << k_funcinfo << "deleting data from TLV(257)" << endl;
				delete [] tlvMessage.data; // getTLV uses getBlock() internally! same as aboves delete applies

				moreTLVs = (inbuf.length() > 0);
				break;
			} // END TLV(0x0002), normal message block

			case 0x0004: // AIM Away message
			{
				kdDebug(14150) << k_funcinfo << "AIM autoresponse." << endl;
				// There isn't actually a message in this TLV, it just specifies
				// that the message that was send was an autoresponse
				inbuf.getWord();
				// Set the autoresponse flag
				isAutoResponse = true;

				moreTLVs = (inbuf.length() > 0);
				break;
			} // END TLV(0x0004), AIM away message

			case 0x0008: // User Icon
			{
				kdDebug(14150) << k_funcinfo << "AIM USER ICON." << endl;
				// TODO support this
				// The length of the TLV
				length = inbuf.getWord();
				/*char *msg =*/ inbuf.getBlock(length);
				moreTLVs = (inbuf.length() > 0);
				break;
			}

			case 0x000b: //unknown
			{
				/*length = */inbuf.getWord();
				moreTLVs = (inbuf.length() > 0);
				break;
			}

			default: //unknown type
			{
				kdDebug(14150) << k_funcinfo <<
					"Unknown message type, type=" << type << endl;
				moreTLVs = (inbuf.length() > 0);
				break;
			}
		};
	}
}



void OscarSocket::parseServerIM(Buffer &inbuf, const UserInfo &u)
{
	kdDebug(14150) << k_funcinfo << "IM received on channel 4 from " << u.sn << endl;
	TLV tlv5tlv = inbuf.getTLV();
	kdDebug(14150) << k_funcinfo << "The first TLV is of type " << tlv5tlv.type << endl;
	if (tlv5tlv.type != 0x0005)
	{
		kdDebug(14150) << k_funcinfo << "Aborting because first TLV != TLV(5)" << endl;
		return;
	}

	Buffer tlv5(tlv5tlv.data, tlv5tlv.length);

	DWORD uin = tlv5.getLEDWord(); // little endian for no sane reason!
	if(QString::number(uin) != u.sn)
	{
		kdWarning(14150) << k_funcinfo <<
		"type-4 message uin does not match uin found in packet header!" << endl;
	}
	BYTE msgtype = tlv5.getByte();
	BYTE msgflags = tlv5.getByte();

	kdDebug(14150) << k_funcinfo <<
		"MSGFORMAT_SERVER; server message, TLV(5) length= " << tlv5tlv.length <<
		", uin=" << uin <<
		", type=" << msgtype <<
		", flags=" << msgflags << endl;

	// can be NULL if contact is not in contactlist!
	OscarContact *contact = static_cast<OscarContact*>(mAccount->contacts()[u.sn]);

	const char *msgText = tlv5.getLNTS();

	OscarMessage oMsg;
	oMsg.setText(ServerToQString(msgText, contact, false), OscarMessage::Plain);

	delete [] msgText; // getBlock allocates memory, we HAVE to free it again!

	if(!oMsg.text().isEmpty())
		parseMessage(u, oMsg, msgtype, msgflags);

	//kdDebug(14150) << k_funcinfo << "END" << endl;
} // END parseServerIM()


void OscarSocket::parseMessage(const UserInfo &u, OscarMessage &message, const BYTE type, const BYTE /*flags*/)
{
	switch(type)
	{
		case MSG_AUTO:
			kdDebug(14150) << k_funcinfo <<
				"Got an automatic message: " << message.text() << endl;
			emit receivedAwayMessage(u.sn, message.text()); // only sets contacts away message var
			message.setType(OscarMessage::Away);
			emit receivedMessage(u.sn, message); // also displays message in chatwin
			break;
		case MSG_NORM:
			kdDebug(14150) << k_funcinfo <<
				"Got a normal message: " << message.text() << endl;
			message.setType(OscarMessage::Normal);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_URL:
			kdDebug(14150) << k_funcinfo <<
				"Got an URL message: " << message.text() << endl;
			message.setType(OscarMessage::URL);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_AUTHREJ:
			kdDebug(14150) << k_funcinfo <<
				"Got an 'auth rejected' message: " << message.text() << endl;
			message.setType(OscarMessage::DeclinedAuth);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_AUTHACC:
			kdDebug(14150) << k_funcinfo <<
				"Got an 'auth granted' message: " << message.text() << endl;
			message.setType(OscarMessage::GrantedAuth);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_WEB:
			kdDebug(14150) << k_funcinfo <<
				"Got a web panel message: " << message.text() << endl;
			message.setType(OscarMessage::WebPanel);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_EMAIL:
			kdDebug(14150) << k_funcinfo <<
				"Got an email message: " << message.text() << endl;
			message.setType(OscarMessage::EMail);
			emit receivedMessage(u.sn, message);
			break;
		case MSG_CHAT:
		case MSG_FILE:
		case MSG_CONTACT:
		case MSG_EXTENDED:
			kdDebug(14150) << k_funcinfo <<
				"Got an unsupported message, dropping: " << message.text() << endl;
			break; // TODO: unsupported and for now dropped messages
		default:
			kdDebug(14150) << k_funcinfo <<
				"Got unknown message type, treating as normal: " << message.text() << endl;
			message.setType(OscarMessage::Normal);
			emit receivedMessage(u.sn, message);
			break;
	} // END switch
}


// parses the standard user info block
bool OscarSocket::parseUserInfo(Buffer &inbuf, UserInfo &u)
{
	u.userclass=0;
	u.evil=0;
	u.idletime = 0;
	u.sessionlen = 0;
	u.localip = 0;
	u.realip = 0;
	u.port = 0;
	u.fwType = 0;
	u.version = 0;
	u.icqextstatus=0;
	u.capabilities=0;

	if(inbuf.length() == 0)
	{
		kdDebug(14150) << k_funcinfo << "ZERO sized userinfo!" << endl;
		return false;
	}

	char *cb = inbuf.getBUIN(); // screenname/uin
	u.sn = tocNormalize(QString::fromLatin1(cb)); // screennames and uin are always us-ascii
	delete [] cb;

	// Next comes the warning level
	//for some reason aol multiplies the warning level by 10
	u.evil = (int)(inbuf.getWord() / 10);

	WORD tlvlen = inbuf.getWord(); //the number of TLV's that follow

	/*kdDebug(14150) << k_funcinfo
		<< "Contact: '" << u.sn <<
		"', number of TLVs following " << tlvlen << endl;*/

	for (unsigned int i=0; i<tlvlen; i++)
	{
		TLV t = inbuf.getTLV();
		Buffer tlvBuf(t.data,t.length);

		switch(t.type)
		{
			case 0x0001: //user class
			{
				u.userclass = tlvBuf.getWord();
				break;
			}
			case 0x0002: //member since time
			case 0x0005: // member since time (again)
			{
				u.membersince.setTime_t(tlvBuf.getDWord());
				break;
			}
			case 0x0003: //online since time
			{
				u.onlinesince.setTime_t(tlvBuf.getDWord());
				break;
			}
			case 0x0004: //idle time
			{
				u.idletime = tlvBuf.getWord();
				break;
			}
			case 0x0006:
			{
				u.icqextstatus = tlvBuf.getDWord();
				break;
			}
			case 0x000a: // IP in a DWORD [ICQ]
			{
				u.realip = htonl(tlvBuf.getDWord());
				break;
			}
			case 0x000c: // CLI2CLI
			{
				u.localip = htonl(tlvBuf.getDWord());
				u.port = tlvBuf.getDWord();
				u.fwType = static_cast<int>(tlvBuf.getWord());
				u.version = tlvBuf.getWord();
				// ignore the rest of the packet for now
				break;
			}
			case 0x000d: //capability info
			{
				u.capabilities = parseCapabilities(tlvBuf);
				break;
			}
			case 0x0010: //session length (for AOL users, in seconds)
			case 0x000f: //session length (for AIM users, in seconds)
			{
				u.sessionlen = tlvBuf.getDWord();
				break;
			}
			case 0x001e: // unknown, empty
				break;
			default: // unknown info type
			{
				/*kdDebug(14150) << k_funcinfo << "Unknown TLV(" << t.type <<
					") length=" << t.length << " in userinfo for user '" <<
					u.sn << "'" << tlvBuf.toString() << endl;*/
			}
		}; // END switch()
		tlvBuf.clear(); // unlink tmpBuf from tlv data
		delete [] t.data; // get rid of tlv data.
	} // END for (unsigned int i=0; i<tlvlen; i++)
	return true;
}



const DWORD OscarSocket::parseCapabilities(Buffer &inbuf)
{
//
// FIXME: port capabilities array to some qt based list class, makes usage of memcmp obsolete
//
	DWORD capflags = 0;

	#ifdef OSCAR_CAP_DEBUG
	QString dbgCaps = "CAPS: ";
	#endif

	while(inbuf.length() >= 0x10)
	{
		char *cap;
		cap = inbuf.getBlock(0x10);

		for (unsigned int i=0; oscar_caps[i].flag != AIM_CAPS_LAST; i++)
		{
			if (memcmp(&oscar_caps[i].data, cap, 0x10) == 0)
			{
				capflags |= oscar_caps[i].flag;

				#ifdef OSCAR_CAP_DEBUG
				switch(oscar_caps[i].flag)
				{
					case AIM_CAPS_BUDDYICON:
						dbgCaps += "AIM_CAPS_BUDDYICON ";
						break;
					case AIM_CAPS_VOICE:
						dbgCaps += "AIM_CAPS_VOICE ";
						break;
					case AIM_CAPS_IMIMAGE:
						dbgCaps += "AIM_CAPS_IMIMAGE ";
						break;
					case AIM_CAPS_CHAT:
						dbgCaps += "AIM_CAPS_CHAT ";
						break;
					case AIM_CAPS_GETFILE:
						dbgCaps += "AIM_CAPS_GETFILE ";
						break;
					case AIM_CAPS_SENDFILE:
						dbgCaps += "AIM_CAPS_SENDFILE ";
						break;
					case AIM_CAPS_GAMES2:
					case AIM_CAPS_GAMES:
						dbgCaps += "AIM_CAPS_GAMES ";
						break;
					case AIM_CAPS_SAVESTOCKS:
						dbgCaps += "AIM_CAPS_SAVESTOCKS ";
						break;
					case AIM_CAPS_SENDBUDDYLIST:
						dbgCaps += "AIM_CAPS_SENDBUDDYLIST ";
						break;
					case AIM_CAPS_ISICQ:
						dbgCaps += "AIM_CAPS_ISICQ ";
						break;
					case AIM_CAPS_APINFO:
						dbgCaps += "AIM_CAPS_APINFO ";
						break;
					case AIM_CAPS_RTFMSGS:
						dbgCaps += "AIM_CAPS_RTFMSGS ";
						break;
					case AIM_CAPS_EMPTY:
						dbgCaps += "AIM_CAPS_EMPTY ";
						break;
					case AIM_CAPS_ICQSERVERRELAY:
						dbgCaps += "AIM_CAPS_ICQSERVERRELAY ";
						break;
					case AIM_CAPS_IS_2001:
						dbgCaps += "AIM_CAPS_IS_2001 ";
						break;
					case AIM_CAPS_TRILLIANCRYPT:
						dbgCaps += "AIM_CAPS_TRILLIANCRYPT ";
						break;
					case AIM_CAPS_UTF8:
						dbgCaps += "AIM_CAPS_UTF8 ";
						break;
					case AIM_CAPS_IS_WEB:
						dbgCaps += "AIM_CAPS_IS_WEB ";
						break;
					case AIM_CAPS_INTEROPERATE:
						dbgCaps += "AIM_CAPS_INTEROPERATE ";
						break;

					default:
						QString capstring;
						capstring.sprintf("{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							cap[0], cap[1], cap[2], cap[3],cap[4], cap[5],
							cap[6], cap[7], cap[8], cap[9],
							cap[10], cap[11], cap[12], cap[13],
							cap[14], cap[15]);
						kdDebug(14150) << k_funcinfo << "Unknown Capability: " << capstring << endl;
				} // END switch
				#endif // OSCAR_CAP_DEBUG

				break;
			} // END if(memcmp...
		} // END for...
		delete [] cap;
	}
	#ifdef OSCAR_CAP_DEBUG
	kdDebug(14150) << k_funcinfo << dbgCaps << endl;
	#endif
	return capflags;
}


// TODO: Split up into type-1, type-2 and type-4 messaging
// currently we only do simple type-1
void OscarSocket::sendIM(const QString &message, OscarContact *contact, bool isAuto)
{
	//check to see if we have a direct connection to the contact
	#if 0
	OscarConnection *dc = mDirectIMMgr->findConnection(contact->contactName());
	if (dc)
	{
		kdDebug(14150) << k_funcinfo <<
			"Sending direct IM " << message <<
			" to " << contact->contactName() << endl;

		dc->sendIM(message, isAuto);
		return;
	}
	#endif

	kdDebug(14150) << k_funcinfo << "SEND (CLI_SENDMSG), msg='" << message <<
		"' to '" << contact->contactName() << "'" << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0004,0x0006,0x0000, toicqsrv_seq);
	toicqsrv_seq++;

	for (int i=0;i<8;i++) //generate random message cookie (MID, message ID)
		outbuf.addByte( (BYTE) rand());

	outbuf.addWord(0x0001); // message type, this is only type-1
	// TODO: support more types
	// 2 -> special messages (also known as advanced messages)
	// 4 -> url etc.

	outbuf.addByte((contact->contactName()).length()); //dest sn length
	outbuf.addString(contact->contactName().latin1(), contact->contactName().length()); //dest sn

	// ====================================================================================
	Buffer tlv2;
	tlv2.addWord(0x0501); // add TLV(0x0501) also known as TLV(1281)
	if(mIsICQ)
	{
		static const char icqfeatures[] = {0x01, 0x06}; // 0x06 == UTF support
		tlv2.addWord(sizeof(icqfeatures)); // add TLV length
		tlv2.addString(icqfeatures, sizeof(icqfeatures));
	}
	else
	{
		static const char aimfeatures[] = {0x01, 0x01, 0x01, 0x02};
		tlv2.addWord(sizeof(aimfeatures)); // add TLV length
		tlv2.addString(aimfeatures, sizeof(aimfeatures));
	}

	QTextCodec *codec = 0L;
	WORD charset = 0x0000; // default to ascii
	WORD charsubset = 0x0000;
	int length = message.length();
	unsigned char *utfMessage = 0L;

	codec=QTextCodec::codecForMib(3); // US-ASCII

	if(codec)
	{
		if(codec->canEncode(message))
		{
			//kdDebug(14150) << k_funcinfo << "Going to encode as US-ASCII" << endl;
			charset=0x0000; // send US-ASCII
		}
		else
		{
			codec=0L; // we failed encoding it as US-ASCII
			//kdDebug(14150) << k_funcinfo << "Cannot encode as US-ASCII" << endl;
		}
	}

	if(!codec && contact->hasCap(AIM_CAPS_UTF8))
	{
		// use UTF is peer supports it and encoding as US-ASCII failed
		length=message.length()*2;
		utfMessage=new unsigned char[length];
		for(unsigned int l=0; l<message.length(); l++)
		{
			utfMessage[l*2]=message.unicode()[l].row();
			utfMessage[(l*2)+1]=message.unicode()[l].cell();
		}
		charset=0x0002; // send UTF-16BE
	}
	else
	{
		//kdDebug(14150) << k_funcinfo << "Won't send as UTF-16BE, codec value=" << (void *)codec << endl;
	}

	// no codec and no charset and per-contact encoding set
	if(!codec && charset != 0x0002 && contact->encoding() != 0)
	{
		codec=QTextCodec::codecForMib(contact->encoding());
		if(codec)
		{
			charset=0x0003;
			/*kdDebug(14150) << k_funcinfo <<
				"Using per-contact encoding, encoding name:" << codec->name() << endl;*/
		}
		else
		{
			//kdDebug(14150) << k_funcinfo << "Could not find QTextCodec for per-contact encoding!" << endl;
		}
	}
	else
	{
		//kdDebug(14150) << k_funcinfo << "Won't use per-contact encoding, codec value=" << (void *)codec << endl;
	}

	if(!codec && charset != 0x0002) // it's neither unicode nor did we find a codec so far!
	{
		kdDebug(14150) << k_funcinfo <<
			"Couldn't find suitable encoding for outgoing message, " <<
			"encoding using ISO-8859-1, prepare for receiver getting unreadable text :)" << endl;
		charset=0x0003;
		codec=QTextCodec::codecForMib(4); // ISO-8859-1
	}

	tlv2.addWord(0x0101); //add TLV(0x0101) also known as TLV(257)
	tlv2.addWord(length + 0x04); // add TLV length
	tlv2.addWord(charset); // normal char set
	tlv2.addWord(charsubset); // normal char set

	if(utfMessage)
	{
		kdDebug(14150) << k_funcinfo << "Outgoing message encoded as 'UTF-16BE'" << endl;
		tlv2.addString(utfMessage, length); // the actual message
		delete [] utfMessage;
	}
	else
	{
		kdDebug(14150) << k_funcinfo << "Outgoing message encoded as '" << codec->name() << "'" << endl;
		QCString outgoingMessage=codec->fromUnicode(message);
		tlv2.addString(outgoingMessage, length); // the actual message
	}
	// ====================================================================================

	outbuf.addTLV(0x0002, tlv2.length(), tlv2.buffer());

	if(isAuto) // No clue about this stuff, probably AIM-specific [mETz]
	{
		outbuf.addWord(0x0004);
		outbuf.addWord(0x0000);
	}

	if(mIsICQ)
	{
		//outbuf.addWord(0x0003); // TLV.Type(0x03) - request an ack from server
		//outbuf.addWord(0x0000);

		outbuf.addWord(0x0006); // TLV.Type(0x06) - store message if recipient offline
		outbuf.addWord(0x0000);
	}

	sendBuf(outbuf,0x02);
}


void OscarSocket::parseUserOnline(Buffer &inbuf)
{
	UserInfo u;
	if (parseUserInfo(inbuf, u))
	{
		//kdDebug(14150) << k_funcinfo << "RECV SRV_USERONLINE, name=" << u.sn << endl;
		emit gotContactChange(u);
	}
}

void OscarSocket::parseUserOffline(Buffer &inbuf)
{
	UserInfo u;
	if (parseUserInfo(inbuf, u))
	{
		//kdDebug(14150) << k_funcinfo << "RECV SRV_USEROFFLINE, name=" << u.sn << endl;
		emit gotOffgoingBuddy(u.sn);
	}
}

void OscarSocket::sendWarning(const QString &target, bool isAnonymous)
{
	Buffer outbuf;
	outbuf.addSnac(0x0004,0x0008,0x0000,0x00000000);
	if (isAnonymous)
		outbuf.addWord(0x0001);
	else
		outbuf.addWord(0x0000);
	outbuf.addByte(target.length());
	outbuf.addString(target.latin1(),target.length());
	sendBuf(outbuf,0x02);
}

void OscarSocket::sendChangePassword(const QString &newpw, const QString &oldpw)
{
	// FIXME: This does not work :-(

	kdDebug(14150) << k_funcinfo << "Changing password from " << oldpw << " to " << newpw << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0007,0x0004,0x0000,0x00000000);
	outbuf.addTLV(0x0002,newpw.length(),newpw.latin1());
	outbuf.addTLV(0x0012,oldpw.length(),oldpw.latin1());
	sendBuf(outbuf,0x02);
}

void OscarSocket::sendChatJoin(const QString &/*name*/, const int /*exchange*/)
{
	Buffer outbuf;
	outbuf.addSnac(0x0001,0x0004,0x0000,0x00000000);
	outbuf.addWord(0x000d);
	//outbuf.addChatTLV(0x0001,exchange,name,0x0000); //instance is 0x0001 here for a test
	sendBuf(outbuf,0x02);
	kdDebug(14150) << "Send chat join thingie (That's a technical term)" << endl;
}

#if 0
void OscarSocket::parseRedirect(Buffer &inbuf)
{
	// FIXME: this function does not work, but it is never
	// called unless you want to connect to the advertisements server
	kdDebug(14150) << "Parsing redirect" << endl;

	OscarConnection *servsock = new OscarConnection(getSN(),"Redirect",Redirect,QByteArray(8));

	QPtrList<TLV> tl = inbuf.getTLVList();
//	int n;
	QString host;
	tl.setAutoDelete(true);

	if (!findTLV(tl,0x0006) && !findTLV(tl,0x0005) && !findTLV(tl,0x000e))
	{
		tl.clear();
		emit protocolError(
			i18n("An unknown error occurred. Please check " \
				"your internet connection. The error message " \
				"was: \"Not enough information found in server redirect\""), 0);
		return;
	}
	for (TLV *tmp = tl.first(); tmp; tmp = tl.next())
	{
		switch (tmp->type)
		{
			case 0x0006: //auth cookie
/*				for (int i=0;i<tmp->length;i++)
					servsock->mAuthCookie[i] = tmp->data[i];*/
				break;
			case 0x000d: //service type
				//servsock->mConnType = (tmp->data[1] << 8) | tmp->data[0];
				break;
			case 0x0005: //new ip & port
			{
/*				host = tmp->data;
				n = host.find(':');
				if (n != -1)
				{
					servsock->mHost = host.left(n);
					servsock->mPort = host.right(n).toInt();
				}
				else
				{
					servsock->mHost = host;
					servsock->mPort = peerPort();
				}
				kdDebug(14150) << "Set host to " << servsock->mHost <<
					", port to " << servsock->mPort << endl;*/
				break;
			}

			default: //unknown
				kdDebug(14150) << "Unknown tlv type in parseredirect: " << tmp->type << endl;
				break;
		}
		delete [] tmp->data;
	}
	tl.clear();
	//sockets.append(servsock);
	delete servsock;
	kdDebug(14150) << "Socket added to connection list!" << endl;
}
#endif

/*
  Request a direct IM session with someone
	type == 0: request
	type == 1: deny
	type == 2: accept
*/
void OscarSocket::sendDirectIMRequest(const QString &sn)
{
	sendRendezvous(sn,0x0000,AIM_CAPS_IMIMAGE);
}

void OscarSocket::parseSrvMsgAck(Buffer &inbuf)
{
	//8 byte cookie is first
	/*char *ck =*/ inbuf.getBlock(8);
	//delete [] ck;
	WORD type = inbuf.getWord();

	char *sn = inbuf.getBUIN();
	QString nm = QString::fromLatin1(sn);
	delete [] sn;

	kdDebug(14150) << k_funcinfo << "RECV (SRV_SRVACKMSG) sn=" << nm << ", type=" << type << endl;

	emit gotAck(nm,type);
}

void OscarSocket::parseMsgAck(Buffer &inbuf)
{

	WORD sublen, seq2;
	BYTE msgFlags, msgType;
	WORD msgStatus, msgPrio;

	inbuf.getBlock(8);

	inbuf.getWord(); // message-type, only type-2 is acknowledged so this is always 2

	char *tmp = inbuf.getBUIN();
	QString uin = QString::fromLatin1(tmp);
	delete [] tmp;

	inbuf.getWord(); // unk

	sublen = inbuf.getLEWord(); // len of following subchunk
	//kdDebug(14150) << k_funcinfo << "sublen=" << sublen << endl;
	inbuf.getBlock(sublen); // ignore subchunk
	//kdDebug(14150) << k_funcinfo << "len after subchunk=" << inbuf.length() << endl;

	inbuf.getLEWord();
	seq2 = inbuf.getLEWord();
	inbuf.getBlock(12); // ignore 12 zero bytes
	//kdDebug(14150) << k_funcinfo << "len after 12 zero bytes=" << inbuf.length() << endl;

	msgType = inbuf.getByte(); //
	msgFlags = inbuf.getByte(); // type and flags have wrong order because it's a little-endian word
	msgStatus = inbuf.getLEWord();
	msgPrio = inbuf.getLEWord();

	WORD txtLen = inbuf.getLEWord();
	char *txtStr = inbuf.getBlock(txtLen);
	QString text = QString::fromLatin1(txtStr); // TODO: encoding
	delete [] txtStr;

	kdDebug(14150) << k_funcinfo << "RECV (ACKMSG) uin=" << uin <<
		" msgType=" << (int)msgType << " msgFlags=" << (int)msgFlags <<
		" msgStatus=" << (int)msgStatus << " msgPrio=" << (int)msgPrio <<
		" text='"  << text << "'" << endl;

	if(msgFlags == MSG_FLAG_GETAUTO)
	{
		emit receivedAwayMessage(uin, text);
	}

	//TODO: there can be more data after text
}

void OscarSocket::sendLocationInfo(const QString &profile, const unsigned long caps)
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_SETUSERINFO/CLI_SET_LOCATION_INFO)" << endl;
	Buffer outbuf, capBuf;

	unsigned long sendCaps;
	if(caps==0)
	{
		if(mIsICQ)
			sendCaps = KOPETE_ICQ_CAPS;
		else
			sendCaps = KOPETE_AIM_CAPS;
	}
	else
		sendCaps = caps;

	outbuf.addSnac(0x0002,0x0004,0x0000,0x00000000);
	if(!profile.isNull() && !mIsICQ)
	{
		static const QString defencoding = "text/aolrtf; charset=\"us-ascii\"";
		outbuf.addTLV(0x0001, defencoding.length(), defencoding.latin1());
		outbuf.addTLV(0x0002, profile.length(), profile.local8Bit());
		//kdDebug(14150) << k_funcinfo << "adding profile=" << profile << endl;
	}

	for (int i=0; oscar_caps[i].flag != AIM_CAPS_LAST; i++)
	{
		if (oscar_caps[i].flag & sendCaps)
			capBuf.addString(oscar_caps[i].data, 16);
	}
	//kdDebug(14150) << k_funcinfo << "adding capabilities, size=" << capBuf.length() << endl;
	outbuf.addTLV(0x0005, capBuf.length(), capBuf.buffer());

	sendBuf(outbuf,0x02);
}

void OscarSocket::parseRateChange(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_RATE_LIMIT_WARN)" << endl;

	WORD code = inbuf.getWord();

	switch(code)
	{
		case 0x0001:
			kdDebug(14150) << k_funcinfo <<
				"Rate limits parameters changed" << endl;
			break;
		case 0x0002:
			kdDebug(14150) << k_funcinfo <<
				"Rate limits warning (current level < alert level)" << endl;
			break;
		case 0x0003:
			kdDebug(14150) << k_funcinfo <<
				"Rate limit hit (current level < limit level)" << endl;
			break;
		case 0x0004:
			kdDebug(14150) << k_funcinfo <<
				"Rate limit clear (current level > clear level)" << endl;
			break;
		default:
			kdDebug(14150) << k_funcinfo <<
				"unknown code for rate limit warning" << endl;
	}

	WORD rateclass = inbuf.getWord();

	DWORD windowSize = inbuf.getDWord();
	DWORD clearLevel = inbuf.getDWord();
	DWORD alertLevel = inbuf.getDWord();
	DWORD limitLevel = inbuf.getDWord();
	DWORD disconnectLevel = inbuf.getDWord();
	DWORD currentLevel = inbuf.getDWord();
	DWORD maxLevel = inbuf.getDWord();
	DWORD lastTime = inbuf.getDWord();
	BYTE currentState = inbuf.getByte();

	if (code != 0x0002) // do not spam me just with warnings, only critical limits please
	{
		kdDebug(14150) << k_funcinfo <<
		"RATE LIMIT DATA (class id " << rateclass << ") -----" << endl <<
		"  windowSize= " << windowSize << endl <<
		"  clearLevel= " << clearLevel << endl <<
		"  alertLevel= " << alertLevel << endl <<
		"  limitLevel= " << limitLevel << endl <<
		"  disconnectLevel= " << disconnectLevel << endl <<
		"  currentLevel= " << currentLevel << endl <<
		"  maxLevel= " << maxLevel << endl <<
		"  lastTime= " << lastTime << endl <<
		"  currentState= " << (WORD)currentState << endl;
	}

	if (currentLevel <= disconnectLevel)
	{
		emit protocolError(i18n("The account %1 will be disconnected for " \
			"exceeding the rate limit. Please wait approximately 10 minutes" \
			" before reconnecting.")
			.arg(mAccount->accountId()), 0);
		//let the account properly clean itself up
		mAccount->disconnect(KopeteAccount::Manual);
	}
	else
	{
		if (code == 0x0002 || code == 0x0003)
		{
			kdDebug(14150) << k_funcinfo "WARNING about rate-limit in class "
				<< rateclass << " received. Current level: "
				<< currentLevel << endl;
		}
	}

/*
 Docs from http://iserverd.khstu.ru/oscar/snac_01_0a.html

0x0001 Rate limits parameters changed
0x0002 Rate limits warning (current level < alert level)
0x0003 Rate limit hit (current level < limit level)
0x0004 Rate limit clear (current level become > clear level)

xx xx		word	Message code (see above)
xx xx		word	Rate class ID
xx xx xx xx	dword	Window size
xx xx xx xx	dword	Clear level
xx xx xx xx	dword	Alert level
xx xx xx xx	dword	Limit level
xx xx xx xx	dword	Disconnect level
xx xx xx xx	dword	Current level
xx xx xx xx	dword	Max level
xx xx xx xx	dword	Last time
xx			byte	Current state
*/
}

void OscarSocket::doLogoff()
{
	mSSIData.clear();
	if(isLoggedIn && (socketStatus() == OscarConnection::Connected))
	{
		kdDebug(14150) << k_funcinfo << "Sending sign off request" << endl;
		Buffer outbuf;
		sendBuf(outbuf, 0x04);
		mSocket->close();
	}
	else
	{
		if(socketStatus() == OscarConnection::Connecting ||
			socketStatus() == OscarConnection::Connected )
		{
			kdDebug(14150) << k_funcinfo <<
				"ERROR =====" <<
				"we're either not logged in correctly or something" <<
				"wicked happened, closing down socket..." << endl;

			//mSocket->close();
			mSocket->reset();

			// FIXME ?
			// The above socket calls _should_ trigger a connectionClosed,
			// I hope it really does [mETz, 05-05-2004]
			//emit connectionClosed(QString::null);
		}
	}
}


// Gets this user status through BLM service
void OscarSocket::sendAddBuddylist(const QString &contactName)
{
	kdDebug(14150) << k_funcinfo << "Sending BLM add buddy" << endl;
	QStringList Buddy = contactName;
	sendBuddylistAdd(Buddy);
}

void OscarSocket::sendDelBuddylist(const QString &contactName)
{
	kdDebug(14150) << k_funcinfo << "Sending BLM del buddy" << endl;
	QStringList Buddy = contactName;
	sendBuddylistDel(Buddy);
}





void OscarSocket::parseError(WORD family, WORD snacID, Buffer &inbuf)
{
	QString msg;
	WORD reason = inbuf.getWord();
	kdDebug(14150) << k_funcinfo <<
		"Got an error, SNAC family=" << family << ", reason=" << reason << endl;


	if (reason < msgerrreasonlen)
	{
		switch (family)
		{
			case OSCAR_FAM_2:
				msg = i18n("Sending userprofile failed: %1").arg(msgerrreason[reason]);
				break;
			case OSCAR_FAM_4:
				// Ignores rate to client errors, usually caused by querying away messages
				if (reason == 3)
					return;
				msg = i18n("Your message did not get sent because the following" \
					" error occurred: %1").arg(msgerrreason[reason]);
				break;
			case OSCAR_FAM_21:
			{
				if (reason == 2)
				{
					msg = i18n("Your ICQ information request was denied by the " \
						"ICQ-Server, please try again later.");
				}
				else
				{
					msg = i18n("Your ICQ information request failed.\n" \
						"%1").arg(msgerrreason[reason]);
				}
				break;
			}
			default:
				msg = i18n("Generic Packet error: %1").arg(msgerrreason[reason]);
				break;
		}
	}
	else
	{
		if (family == OSCAR_FAM_2)
		{
			msg = i18n("Sending userprofile failed: Unknown Error.\n" \
				"Please report a bug at http://bugs.kde.org");
		}
		else if (family == OSCAR_FAM_4)
		{
			msg = i18n("Your message did not get sent: Unknown Error.\n" \
				"Please report a bug at http://bugs.kde.org");
		}
		else
		{
			msg = i18n("Generic Packet error: Unknown Error.\n" \
				"Please report a bug at http://bugs.kde.org");
		}
	}

	emit protocolError(msg, reason);
	// in case a special request failed
	emit snacFailed(snacID);
}

/* Request, deny, or accept a rendezvous session with someone
type == 0: request
type == 1: deny
type == 2: accept
*/
void OscarSocket::sendRendezvous(const QString &/*sn*/, WORD /*type*/, DWORD /*rendezvousType*/, const KFileItem */*finfo*/)
{
	kdDebug(14150) << "DISABLED!" << endl;
#if 0
	OncomingSocket *sockToUse = serverSocket(rendezvousType);
	Buffer outbuf;
	outbuf.addSnac(0x0004,0x0006,0x0000,0x00000000);
	QByteArray ck(8);
	//generate a random message cookie
	for (int i=0;i<8;i++)
	{
		ck[i] = static_cast<BYTE>(rand());
	}

	//add this to the list of pending connections if it is a request
	if(type == 0)
	{
		sockToUse->addPendingConnection(sn, ck, finfo, QString::null, 0,
			DirectInfo::Incoming);
	}

	outbuf.addString(ck,8);
	//channel 2
	outbuf.addWord(0x0002);
	//destination sn
	outbuf.addByte(sn.length());
	outbuf.addString(sn.latin1(),sn.length());
	//add a blank TLV of type 3
	outbuf.addTLV(0x0003,0x0000,NULL);
	//add a huge TLV of type 5
	outbuf.addWord(0x0005);
	if(!finfo) //this is a simple direct IM
	{
		if (type == 0x0000)
			outbuf.addWord(2+8+16+6+8+6+4);
		else
			outbuf.addWord(2+8+16);
	}
	else //this is a file transfer request
	{
		if (type == 0x0000)
		{
			outbuf.addWord(2+8+16+6+8+6+4+2+2+2+2+4+
				finfo->url().fileName().length()+4);
		}
		else
		{
			outbuf.addWord(2+8+16);
		}
	}

	outbuf.addWord(type); //2
	outbuf.addString(ck,8); //8
	for (int i=0;oscar_caps[i].flag != AIM_CAPS_LAST;i++)
	{
		if (oscar_caps[i].flag & rendezvousType)
		{
			outbuf.addString(oscar_caps[i].data,0x10);
			break;
		}
	} //16

	if(type == 0x0000 ) //if this is an initiate rendezvous command
	{
		//TLV (type a)
		outbuf.addWord(0x000a);
		outbuf.addWord(0x0002);
		outbuf.addWord(0x0001); //6
		//TLV (type 3)
		outbuf.addWord(0x0003);
		outbuf.addWord(0x0004);

		if (sockToUse->mSocket->socketStatus() < KExtendedSocket::created)
		{  //make sure the socket stuff is properly set up
			kdDebug(14150) << k_funcinfo << "SERVER SOCKET NOT SET UP... " <<
			"returning from sendRendezvous" << endl;

			emit protocolError(i18n("Error setting up listening socket." \
					" The request will not be send."), 0);
			return;
		}

		outbuf.addDWord(static_cast<DWORD>(setIPv4Address(sockToUse->mSocket->host()))); //8
		//TLV (type 5)
		outbuf.addWord(0x0005);
		outbuf.addWord(0x0002); //8
		outbuf.addWord(sockToUse->mSocket->port().toUShort()); //6
		//TLV (type f)
		outbuf.addTLV(0x000f,0x0000,NULL); //4

		if ( finfo )
		{
			outbuf.addWord(0x2711); //2
			outbuf.addWord(2+2+4+finfo->url().fileName().length()+4); //2
			outbuf.addWord(0x0001); //more than 1 file? (0x0002 for multiple -- implement later)
			outbuf.addWord(0x0001); //number of files
			outbuf.addDWord(finfo->size());
			outbuf.addString(finfo->url().fileName().latin1(),
				finfo->url().fileName().length());
			outbuf.addDWord(0x00000000);
		}
	}

	kdDebug(14150) << "Sending direct IM, type " << type << " from " <<
		sockToUse->mSocket->host() << ", port " << sockToUse->mSocket->port() << endl;

	sendBuf(outbuf,0x02);
#endif
}

void OscarSocket::sendDirectIMDeny(const QString &sn)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;
	sendRendezvous(sn,0x0001,AIM_CAPS_IMIMAGE);
}

void OscarSocket::sendDirectIMAccept(const QString &sn)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;

	sendRendezvous(sn,0x0002,AIM_CAPS_IMIMAGE);
	/*
	if(!mDirectIMMgr->establishOutgoingConnection(sn))
	{
		kdDebug(14150) << k_funcinfo << sn <<
			" not found in pending connection list" << endl;
	}
	*/
}

void OscarSocket::parseMissedMessage(Buffer &inbuf)
{
	while (inbuf.length() > 0)
	{
		// get the channel (this isn't used anywhere)
		/*WORD channel =*/ inbuf.getWord();

		// get user info
		UserInfo u;
		parseUserInfo(inbuf, u);

		// get number of missed messages
		WORD nummissed = inbuf.getWord();

		//the number the aol servers report seems to be one too many
		nummissed--;

		// get reason for missed messages
		WORD reason = inbuf.getWord();

		QString errstring = i18n(
			"You missed one message from %1. Reason given:",
			"You missed %n messages from %1. Reason given:",
			nummissed).arg(u.sn) + "\n";
		switch (reason)
		{
			case 0: //invalid message
				errstring += i18n("It was invalid.",
					"They were invalid.", nummissed);
				break;
			case 1: //message(s) too large
				errstring += i18n("It was too large.",
					"They were too large.", nummissed);
				break;
			case 2: //rate limit exceeded
				errstring += i18n("The client exceeded the rate limit.");
				break;
			case 3: //evil sender
				errstring += i18n("The sender's warning level is too high.");
				break;
			case 4: //evil receiver
				errstring += i18n("Your warning level is too high.");
				break;
			default: //unknown reason
				errstring += i18n("Unknown reasons.");
				break;
		};
		emit protocolError(errstring,0);
	}
}

void OscarSocket::sendSSIRightsRequest()
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_REQLISTS)" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0013,0x0002,0x0000,0x00000002);
	sendBuf(outbuf,0x02);
}

void OscarSocket::sendSSIRequest(void)
{
	kdDebug(14150) << "SEND (CLI_REQROSTER), " <<
		"requesting serverside contactlist for the FIRST time" << endl;
	Buffer outbuf;
	outbuf.addSnac(0x0013,0x0004,0x0000,0x00020004);
	sendBuf(outbuf,0x02);
}

void OscarSocket::parseSSIRights(Buffer &/*inbuf*/)
{
	kdDebug(14150) << k_funcinfo << "RECV (SRV_REPLYLISTS) IGNORING" << endl;
	//List of TLV's
		//TLV of type 4 contains a bunch of words, representing maxmimums
		// word 0 of TLV 4 data: max contacts
		// word 1 of TLV 4 data: max groups
		// word 2 of TLV 4 data: max visible-list entries
		// word 3 of TLV 4 data: max invisible-list entries
//	sendSSIRequest();
	gotAllRights++;
	if (gotAllRights==7)
	{
		kdDebug(14150) << k_funcinfo "gotAllRights==7" << endl;
		sendInfo();
	}
}


void OscarSocket::sendInfo()
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;

	// greater 7 and thus sendInfo() is not getting called again
	// except on reconnnect
	gotAllRights=99;

	sendLocationInfo(loginProfile); // CLI_SETUSERINFO
	loginProfile = QString::null;

	sendMsgParams(); // CLI_SETICBM

	sendIdleTime(0); // CLI_SNAC1_11, sent before CLI_SETSTATUS

	if(mIsICQ)
		sendICQStatus(loginStatus); // send initial login status

	if (!mIsICQ)
	{
		sendGroupPermissionMask(); // unknown to icq docs
		sendPrivacyFlags(); // unknown to icq docs
	}

	sendClientReady(); // CLI_READY
	if (mIsICQ)
	{
		sendReqOfflineMessages(); // CLI_REQOFFLINEMSGS
		startKeepalive();
	}

	requestMyUserInfo();
}


// Sends parameters for ICBM messages (CLI_SETICBM)
void OscarSocket::sendMsgParams()
{
	kdDebug(14150) << k_funcinfo << "SEND (CLI_SETICBM)" << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0004,0x0002,0x0000,0x00000002);

	// max channels
	//this is read-only, and must be set to 0 here
	outbuf.addWord(0x0000);

	//these are all read-write
	// channel-flags
	// outbuf.addDWord(mIsICQ ? 0x00000003 : 0x0000000b);
	// bit 1 : messages allowed (always 1 or you cannot send IMs)
	// bit 2 : missed call notifications enabled
	// bit 4 : typing notifications enabled
	outbuf.addDWord(0x0000000b);

	//max message length (8000 bytes)
	outbuf.addWord(0x1f40);
	//max sender warning level (999)
	outbuf.addWord(0x03e7);
	//max receiver warning level  (999)
	outbuf.addWord(0x03e7);
	//min message interval limit  (0 msec)
	outbuf.addWord(0x0000);
	// unknown parameter
	outbuf.addWord(0x0000);

	sendBuf(outbuf,0x02);
}


// Reads a FLAP header from the input
FLAP OscarSocket::getFLAP()
{
	BYTE start = 0x00;
	char peek[6];
	FLAP fl;
	fl.error = false;

	if (mSocket->peekBlock(&peek[0], 6) != 6 )
	{
		kdDebug(14150) << k_funcinfo <<
			"Error reading 6 bytes for FLAP" << endl;
		fl.error = true;
		return fl;
	}

	Buffer buf(&peek[0], 6);
	start = buf.getByte();

	if (start == 0x2a)
	{
		//get the channel ID
		fl.channel = buf.getByte();
		//kdDebug(14150) << k_funcinfo << "FLAP channel=" << fl.channel << endl;

		//get the sequence number
		fl.sequence_number = buf.getWord();
		//kdDebug(14150) << k_funcinfo << "FLAP sequence_number=" << fl.sequence_number << endl;

		// get the packet length
		fl.length = buf.getWord();
		//kdDebug(14150) << k_funcinfo << "FLAP length=" << fl.length << endl;
		//kdDebug(14150) << k_funcinfo << "bytes available=" << mSocket->bytesAvailable() << endl;

		if(mSocket->bytesAvailable() < fl.length+6)
		{
			/*kdDebug(14150) << k_funcinfo <<
				"Not enough data in recv buffer to read the full FLAP, aborting" << endl;*/
			fl.error = true;
		}
	}
	else
	{
		kdDebug(14150) << k_funcinfo <<
			"Error reading FLAP, start byte is '" << start << "'" << endl;
		fl.error = true;
	}

	// get rid of FLAP header because we successfully retrieved it
	if (!fl.error)
	{
		/*kdDebug(14150) << k_funcinfo <<
			"Returning FLAP, channel=" << fl.channel << ", length=" << fl.length << endl;*/
		mSocket->readBlock(0L, 6);
	}
	return fl;
}

/*
// Called when a direct IM is received
void OscarSocket::OnDirectIMReceived(QString message, QString sender, bool isAuto)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;

	OscarMessage oMsg;
	oMsg.setText(message, OscarMessage::Plain);
	oMsg.setType(isAuto ? OscarMessage::Away : OscarMessage::Normal);

	emit receivedMessage(sender, oMsg);
}

// Called when a direct IM connection suffers an error
void OscarSocket::OnDirectIMError(QString errmsg, int num)
{
	//let's just emit a protocolError for now
	emit protocolError(errmsg, num);
}

// Called whenever a direct IM connection gets a typing notification
void OscarSocket::OnDirectMiniTypeNotification(QString screenName, int notify)
{
	//for now, just emit a regular typing notification
	emit gotMiniTypeNotification(screenName, notify);
}

// Called when a direct IM connection bites the dust
void OscarSocket::OnDirectIMConnectionClosed(QString name)
{
	emit directIMConnectionClosed(name);
}

// Called when a direct connection is set up and ready for use
void OscarSocket::OnDirectIMReady(QString name)
{
	emit connectionReady(name);
}
*/
#if 0
void OscarSocket::sendFileSendRequest(const QString &sn, const KFileItem &finfo)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;
	sendRendezvous(sn, 0x0000, AIM_CAPS_SENDFILE, &finfo);
}

OscarConnection * OscarSocket::sendFileSendAccept(const QString &sn,
	const QString &fileName)
{
	sendRendezvous(sn, 0x0001, AIM_CAPS_SENDFILE);
	mFileTransferMgr->addFileInfo(sn,
		new KFileItem(KFileItem::Unknown, KFileItem::Unknown, fileName));

	return mFileTransferMgr->establishOutgoingConnection(sn);
}

void OscarSocket::sendFileSendDeny(const QString &sn)
{
	sendRendezvous(sn, 0x0002, AIM_CAPS_SENDFILE);
}

void OscarSocket::OnFileTransferBegun(OscarConnection *con, const QString& file,
	const unsigned long size, const QString &recipient)
{
	kdDebug(14150) << k_funcinfo << "emitting transferBegun()" << endl;
	emit transferBegun(con, file, size, recipient);
}
#endif

#if 0
OncomingSocket *OscarSocket::serverSocket(DWORD capflag)
{
	if ( capflag & AIM_CAPS_IMIMAGE ) //direct im
		return mDirectIMMgr;
	else  //must be a file transfer?
		return mFileTransferMgr;
}
#endif

void OscarSocket::parseConnectionClosed(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "RECV (DISCONNECT)" << endl;

	// This is a part of icq login procedure,
	// Move this into its own function!
	QPtrList<TLV> lst=inbuf.getTLVList();
	lst.setAutoDelete(TRUE);
/*
	kdDebug(14150) << "contained TLVs:" << endl;
	TLV *t;
	for(t=lst.first(); t; t=lst.next())
	{
		kdDebug(14150) << "TLV(" << t->type << ") with length " << t->length << endl;
	}
*/
	TLV *uin=findTLV(lst,0x0001);
	if(uin)
	{
		kdDebug(14150) << k_funcinfo << "found TLV(1) [UIN], uin=" << uin->data << endl;
		delete [] uin->data;
	}

	TLV *descr=findTLV(lst,0x0004);
	if(!descr)
		descr=findTLV(lst,0x000b);
	if(descr)
	{
		kdDebug(14150) << k_funcinfo << "found TLV(4) [DESCRIPTION] reason=" << descr->data << endl;
		delete [] descr->data;
	}

	TLV *err=findTLV(lst,0x0008);
	if (!err)
		err=findTLV(lst,0x0009);
	if (err)
	{
		kdDebug(14150) << k_funcinfo << k_funcinfo << "found TLV(8) [ERROR] error= " <<
			((err->data[0] << 8)|err->data[1]) << endl;

		WORD errorNum = ((err->data[0] << 8)|err->data[1]);

		switch(errorNum)
		{
			case 0x0001: // multiple logins (on same UIN)
			{
// 				kdDebug(14150) << k_funcinfo <<
// 					"multiple logins (on same UIN)!!!" << endl;
				emit protocolError(
					i18n("You have logged in more than once with the same %1," \
						" this login is now disconnected.").arg((mIsICQ ? "UIN" : "buddy name")), 0);
				break;
			}

			case 0x0004:
			case 0x0005: // bad password
			{
// 				kdDebug(14150) << k_funcinfo << "bad password!!!" << endl;
				emit protocolError(
					i18n("Could not log on to %1 with account %2 as the password was" \
						" incorrect.").arg((mIsICQ ? "ICQ" : "AIM")).arg(getSN()), 5);
				break;
			}

			case 0x0007: // non-existant ICQ#
			case 0x0008: // non-existant ICQ#
			{
// 				kdDebug(14150) << k_funcinfo << "non-existant ICQ#" << endl;
				emit protocolError(
					i18n("Could not log on to %1 with nonexistent account %2.").arg(
						(mIsICQ ? "ICQ" : "AIM")).arg(getSN()), 0);
				break;
			}

			case 0x0015: // too many clients from same IP
			case 0x0016: // too many clients from same IP
			{
/*				kdDebug(14150) << k_funcinfo <<
					"too many clients from same IP" << endl;*/
				emit protocolError(
					i18n("Could not log on to %1 as there are too many clients" \
						" from the same computer.").arg(
							(mIsICQ ? "ICQ" : "AIM")), 0);
				break;
			}

			case 0x0018: // rate exceeded (turboing)
			{
				/*kdDebug(14150) << k_funcinfo <<
					"rate exceeded (maybe reconnecting too fast), " \
					"server-ban for at least 10 mins!!!" << endl;*/
				emit protocolError(
					i18n("Server has blocked %1 account %2 for sending messages too quickly." \
						" Wait ten minutes and try again. If you continue to try, you will" \
						" need to wait even longer.").arg(
							(mIsICQ ? "ICQ" : "AIM")).arg(getSN()), 0);
				break;
			}
		}

		if (errorNum != 0x0000)
			mAccount->disconnect(KopeteAccount::Manual); //doLogoff();

		delete [] err->data;
	}

	TLV *server=findTLV(lst,0x0005);
	if (server)
	{
		kdDebug(14150) << k_funcinfo << "found TLV(5) [SERVER]" << endl;
		QString ip=server->data;
		int index=ip.find(':');
		bosServer=ip.left(index);
		ip.remove(0,index+1); //get rid of the colon and everything before it
		bosPort=ip.toInt();
		kdDebug(14150) << k_funcinfo << "We should reconnect to server '" << bosServer <<
			"' on port " << bosPort << endl;
		delete[] server->data;
	}

	TLV *cook=findTLV(lst,0x0006);
	if (cook)
	{
		kdDebug(14150) << "found TLV(6) [COOKIE]" << endl;
		mCookie=cook->data;
		mCookieLength=cook->length;
//		delete [] cook->data;
		connectToBos();
	}
	lst.clear();
}

void OscarSocket::sendAuthRequest(const QString &contact, const QString &reason)
{
	kdDebug(14150) << k_funcinfo << "contact='" << contact << "', reason='" << reason <<
		"'" << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0013,0x0018,0x0000,0x00000000);

	outbuf.addBUIN(contact.ascii()); //dest sn
	outbuf.addBSTR(reason.local8Bit());
	outbuf.addWord(0x0000);
	sendBuf(outbuf,0x02);
}

void OscarSocket::sendAuthReply(const QString &contact, const QString &reason, bool grant)
{
	kdDebug(14150) << k_funcinfo << "contact='" << contact << "', reason='" << reason <<
		"', grant=" << grant << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0013,0x001a,0x0000,0x00000000);
	outbuf.addBUIN(contact.ascii()); //dest sn
	outbuf.addByte(grant ? 0x01 : 0x00);
	outbuf.addBSTR(reason.local8Bit());
	sendBuf(outbuf,0x02);
}

void OscarSocket::parseAuthReply(Buffer &inbuf)
{
	kdDebug(14150) << k_funcinfo << "Called." << endl;

	char *tmp = 0L;
	BYTE grant = 0;

	tmp = inbuf.getBUIN();
	QString contact = ServerToQString(tmp, 0L, false);
	delete [] tmp;

	grant = inbuf.getByte();

	tmp = inbuf.getBSTR();
	QString reason = ServerToQString(tmp, 0L, false);
	delete [] tmp;

	emit gotAuthReply(contact, reason, (grant==0x01));
}


void OscarSocket::sendBuddylistAdd(QStringList &contacts)
{
	kdDebug(14150) << k_funcinfo << "SEND CLI_ADDCONTACT (add to local userlist?)" << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0003,0x0004,0x0000,0x00000000);
	for(QStringList::Iterator it = contacts.begin(); it != contacts.end(); ++it)
	{
		QCString contact = (*it).latin1();
		outbuf.addByte(contact.length());
		outbuf.addString(contact, contact.length());
	}
	sendBuf(outbuf,0x02);
}

void OscarSocket::sendBuddylistDel(QStringList &contacts)
{
	kdDebug(14150) << k_funcinfo << "SEND CLI_DELCONTACT" << endl;

	Buffer outbuf;
	outbuf.addSnac(0x0003,0x0005,0x0000,0x00000000);
	for(QStringList::Iterator it = contacts.begin(); it != contacts.end(); ++it)
	{
		QCString contact = (*it).latin1();
		outbuf.addByte(contact.length());
		outbuf.addString(contact, contact.length());
	}
	sendBuf(outbuf,0x02);
}


const QString OscarSocket::ServerToQString(const char* string, OscarContact *contact, bool isUtf8)
{
	//kdDebug(14150) << k_funcinfo << "called" << endl;

	int length = strlen(string);
	int cresult = -1;
	QTextCodec *codec = 0L;

	if(contact != 0L)
	{
		if(contact->encoding() != 0)
			codec = QTextCodec::codecForMib(contact->encoding());
		else
			codec = 0L;
#ifdef CHARSET_DEBUG
		if(codec)
		{
			kdDebug(14150) << k_funcinfo <<
				"using per-contact encoding, MIB=" << contact->encoding() << endl;
		}
#endif
	}

	if(!codec && isUtf8) // in case the per-contact codec didn't work we check if the caller thinks this message is utf8
	{
		codec = QTextCodec::codecForMib(106); //UTF-8
		if(codec)
		{
			cresult = codec->heuristicContentMatch(string, length);
#ifdef CHARSET_DEBUG
			kdDebug(14150) << k_funcinfo <<
				"result for FORCED UTF-8 = " << cresult <<
				", message length = " << length << endl;
#endif
			/*if(cresult < (length/2)-1)
				codec = 0L;*/
		}
	}

	return KopeteMessage::decodeString( string, codec );
}


void OscarSocket::addBuddyToAckMap(const QString &contactName, const QString &groupName, const DWORD id)
{
	kdDebug(14150) << k_funcinfo << "Mapping ID " << id << " to buddy " << contactName << endl;
	AckBuddy buddy;
	buddy.contactName = contactName;
	buddy.groupName = groupName;
	m_ackBuddyMap[id] = buddy;
}

AckBuddy OscarSocket::ackBuddy(const DWORD id)
{
	AckBuddy buddy;
	QMap<DWORD, AckBuddy>::Iterator it;
	for (it = m_ackBuddyMap.begin() ; it != m_ackBuddyMap.end() ; it++)
	{
		if (it.key() == id)
		{
			kdDebug(14150) << k_funcinfo << "Found buddy " << it.data().contactName << ", group " << it.data().groupName << endl;
			buddy = it.data();
			m_ackBuddyMap.remove(it);
			break;
		}
	}
	return(buddy);
}



#include "oscarsocket.moc"
// vim: set noet ts=4 sts=4 sw=4:
