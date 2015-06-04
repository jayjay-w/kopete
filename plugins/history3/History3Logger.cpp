#include "History3Logger.h"

//Qt includes
#include <QSqlQuery>
#include <QDateTime>

//KDE includes
#include <kstandarddirs.h>

//Kopete includes
#include "kopetemessage.h"
#include "kopetecontact.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"

History3Logger* History3Logger::m_instance = 0;


History3Logger::History3Logger()
{
	//Initial database setup
	//The default db will be SQLITE, then we can add MySQL and others later
	QString dbPath = KStandardDirs::locateLocal("appdata", "kopete_history3.db");
	history_database = QSqlDatabase::addDatabase("QSQLITE", "kopete-history3");
	history_database.setDatabaseName(dbPath);
	if (!history_database.open()) {
		return;
	}

	//Check for history table, and create it if it does not exist.
	QSqlQuery table_query("SELECT name FROM sqlite_master WHERE type='table'", history_database);
	table_query.exec();

	QStringList tables;
	while (table_query.next()) {
		tables.append(table_query.value(0).toString());
	}

	if (!tables.contains("history")) {
		history_database.exec(QString(
					      "CREATE TABLE history"
					      "(entry_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
					      "protocol TEXT, "
					      "direction TEXT, "
					      "account TEXT, "
					      "local_id TEXT, "
					      "local_nick TEXT, "
					      "remote_id TEXT, "
					      "remote_nick TEXT, "
					      "date_time TEXT, "
					      "message TEXT"
					      ")"
					      ));
	}
}


History3Logger::~History3Logger()
{
	history_database.close();
}

void History3Logger::appendMessage(const Kopete::Message &msg, const Kopete::Contact *c)
{
	if(!msg.from())
		return;

	// If no contact are given: If the manager is availiable, use the manager's
	// first contact (the channel on irc, or the other contact for others protocols
	const Kopete::Contact *c = ct;
	if(!c && msg.manager() )
	{
		QList<Kopete::Contact*> mb=msg.manager()->members() ;
		c = mb.first();
	}
	if(!c)  //If the contact is still not initialized, use the message author.
		c =   msg.direction()==Kopete::Message::Outbound ? msg.to().first() : msg.from()  ;


	if(!m_metaContact)
	{ //this may happen if the contact has been moved, and the MC deleted
		if(c && c->metaContact())
			m_metaContact=c->metaContact();
		else
			return;
	}

	const Kopete::Contact *me;
	const Kopete::Contact *other;

	if(msg.direction() == msg.Inbound) {
		me = msg.to().first();
		other = msg.from();
	} else if (msg.direction() == msg.Outbound) {
		me = msg.from();
		other = msg.to.first();
	} else {
		return;
	}

	QSqlQuery query(history_database);

	query.prepare("INSERT INTO history (direction, protocol, account, local_id, local_nick, remote_id, remote_nick, date_time, message) "
		      "VALUES (:direction, :protocol, :account, :local_id, :local_nick, :remote_id, :remote_nick, :date_time, :message)");
	query.bindValue(":direction", msg.direction());
	query.bindValue(":local_id",me->contactId() );
	query.bindValue(":local_nick", me->displayName());
	query.bindValue(":remote_id",other->contactId() );
	query.bindValue(":remote_nick", other->displayName());
	query.bindValue(":date_time",msg.timestamp());
	query.bindValue(":protocol", ct->protocol()->pluginId());
	query.bindValue(":account", ct->account()->accountId());
	query.bindValue(":message", msg.plainBody());
	query.exec();
}

bool History3Logger::messageExists(const Kopete::Message &msg, const Kopete::Contact *c)
{
	//Check if the specified message exists in the database
	if (!msg.from())
		return true;

	const Kopete::Contact *ct = c;
	if (!ct && msg.manager()) {
		QList<Kopete::Contact*> mb=msg.manager()->members();
		ct = mb.first();
	}
	if (!ct) {
		ct = msg.direction()==Kopete::Message::Outbound ? msg.to.first() : msg.from();
	}

	const Kopete::Contact *local;
	const Kopete::Contact *remote;

	if (msg.direction() == msg.Inbound) {
		local = msg.to.first();
		remote = msg.from();
	} else if (msg.direction() == msg.Outbound) {
		local = msg.from();
		remote = msg.to.first();
	} else {
		return true;
	}

	QSqlQuery query(history_database);

	query.prepare("SELECT 1 FROM history WHERE direction = :direction AND protocol = :protocol AND account= :account AND local_id = :local_id AND remote_id = :remote_id AND date_time = :date_time AND message = :message");

	query.bindValue(":direction",  msg.direction());
	query.bindValue(":local_id", local->contactId());
	query.bindValue(":remote_id", remote->contactId());
	query.bindValue(":date_time", msg.timestamp());
	query.bindValue(":protocol", c->protocol()->pluginId());
	query.bindValue(":account", c->account()->accountId());
	query.bindValue(":message", msg.plainBody());

	query.exec();

	if (query.next())
		return true;

	return false;
}
