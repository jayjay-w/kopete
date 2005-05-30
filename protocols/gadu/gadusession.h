// -*- Mode: c++-mode; c-basic-offset: 2; indent-tabs-mode: t; tab-width: 2; -*-
//
// Copyright (C) 2003-2004	 Grzegorz Jaskiewicz <gj at pointblue.com.pl>
// Copyright (C) 2002	 	Zack Rusin <zack@kde.org>
//
// gadusession.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef GADUSESSION_H
#define GADUSESSION_H

#include "kopeteaccount.h"

#include <qvaluelist.h>
#include <qptrlist.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qcstring.h>
#include <qhostaddress.h>

#include "gaducontactlist.h"

#include <libgadu.h>

struct KGaduMessage {
	QString		message;	// Unicode
	unsigned int	sender_id;	// sender's UIN
	QDateTime	sendTime;
	QByteArray	rtf;
};

struct KGaduLoginParams {
	uin_t uin;
	QString password;
	bool useTls;
	int status;
	QString statusDescr;
	unsigned int server;
	bool forFriends;
	unsigned int client_addr;
	unsigned int client_port;
};

struct KGaduNotify {
	int status;
	QHostAddress remote_ip;
	unsigned short remote_port;
	bool fileCap;
	int version;
	int image_size;
	int time;
	QString description;
	unsigned int contact_id;
};

typedef QPtrList<KGaduNotify> KGaduNotifyList;

struct ResLine{
	QString uin;
	QString firstname;
	QString surname;
	QString nickname;
	QString age;
	QString city;
	QString orgin;
	QString meiden;
	QString gender;
	int status;
};

typedef QValueList<ResLine> SearchResult;

class QSocketNotifier;
class QStringList;
namespace Kopete { class Message; }
class GaduRichTextFormat;

class GaduSession : public QObject
{
	Q_OBJECT

public:
	GaduSession( QObject* parent = 0, const char* name = 0 );
	virtual ~GaduSession();
	bool	isConnected() const;
	int	status() const;
	QString contactsToString( GaduContactsList* contactsList );
	bool	stringToContacts( GaduContactsList& , const QString& );
	static QString failureDescription( gg_failure_t );
	static QString errorDescription( int err );
	static QString stateDescription( int state );
	void dccRequest( const unsigned int );
	unsigned int getPersonalInformation();
	/*
	 * Initiates search in public directory, we need to be logged on to perform search !
	 * This returns 0, if you are unable to search (fe you are not logged on, you don't have memory)
	 * This does not checks parametrs !
	 * Calling this function more times with the same params, will continue this search as long as
	 * @ref pubDirSearchClose() will not be called
	 * You must set @ref pubDirSearchResult() signal before calling this function, otherwise no result
	 * will be returned
	 */
	unsigned int pubDirSearch( QString&, QString&, QString&, int, QString&, int, int, int, bool );

public slots:
	void	login( KGaduLoginParams* login );
	void	logoff( Kopete::Account::DisconnectReason reason = Kopete::Account::Manual );
	int	notify( uin_t*, int );
	int	addNotify( uin_t );
	int	removeNotify( uin_t );
	int	sendMessage( uin_t recipient, const Kopete::Message& msg, int msgClass );
	int	changeStatus( int, bool forFriends = false );
	int	changeStatusDescription( int, const QString&, bool forFriends = false );
	int	ping();

	void	requestContacts();

	/*
	*  Releases all allocated memory needed to perform search.
	*  This will be done on each @ref pubDirNewSearch(), if previuos is not released
	*/
	void pubDirSearchClose();
	void exportContactsOnServer( GaduContactsList* );

signals:
	void error( const QString&, const QString& );
	void messageReceived( KGaduMessage* );
	void ackReceived( unsigned int );
	void notify( KGaduNotifyList* );
	void contactStatusChanged( KGaduNotify* );
	void pong();
	void connectionFailed( gg_failure_t failure );
	void connectionSucceed( );
	void disconnect( Kopete::Account::DisconnectReason );
	void pubDirSearchResult( const SearchResult&, unsigned int );
	void userListRecieved( const QString& );
	void userListExported();
	void incomingCtcp( unsigned int );

protected slots:
	void enableNotifiers( int );
	void disableNotifiers();
	void checkDescriptor();
	void login( struct gg_login_params* );

private:

	void sendResult( gg_pubdir50_t );
	void handleUserlist( gg_event* );
	void notify60( gg_event* );
	void destroySession();
	void destroyNotifiers();
	void createNotifiers( bool connect );

	gg_session*		session_;
	QSocketNotifier*	read_;
	QSocketNotifier*	write_;
	gg_login_params		params_;
	QTextCodec*		textcodec;
	int			searchSeqNr_;
	GaduRichTextFormat*	rtf;
};

#endif
