#include "databaselogger.h"

#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopeteaccount.h"
#include "kopetechatsession.h"
#include "kopeteprotocol.h"
#include "kopetechatsessionmanager.h"

#include <QDate>
#include <KStandardDirs>
#include <QSqlQuery>
#include <QDebug>

DatabaseLogger::DatabaseLogger(QObject *parent)
	: QObject(parent)
{
	qDebug() << "Initializing history3 plugin";
	initDatabase(SQLITE);
}

DatabaseLogger::~DatabaseLogger()
{
	if (db.open()) {
		db.close();
	}
}

void DatabaseLogger::initDatabase(DatabaseLogger::dbType dbType)
{
	if (db.open())
		db.close();

	if (dbType == SQLITE) {
		QString dbPath = KStandardDirs::locateLocal("appdata", "kopete_history3.db");
		db = QSqlDatabase::addDatabase("QSQLITE", "kopete-history");
		db.setDatabaseName(dbPath);

		if (!db.open()) {
			//Database not open
			return;
		}

		//Check if the messages table exists
		QSqlQuery tableCheckQuery("SELECT name FROM sqlite_master WHERE type='table'", db);
		tableCheckQuery.exec();

		QStringList tables;
		while (tableCheckQuery.next()) {
			tables.append(tableCheckQuery.value(0).toString());
		}

		if (!tables.contains("messages")) {
			//messages table not found. So we create it
			db.exec(
						"CREATE TABLE \"messages\" ("
						"\"id\" Integer Primary Key Autoincrement Not Null, "
						"\"timestamp\" Integer, "
						"\"message\" Text, "
						"\"protocol\" Text Not Null, "
						"\"account\" Text Not Null, "
						"\"direction\" Integer Not Null, "
						"\"importance\" Integer, "
						"\"contact\" Text, "
						"\"subject\" Text, "
						"\"session\" Text, "
						"\"session_name\" Text, "
						"\"from\" Text, "
						"\"from_name\" Text, "
						"\"to\" Text, "
						"\"to_name\" Text, "
						"\"state\" Integer, "
						"\"type\" Integer, "
						"\"is_group\" Integer Default'0') "
						);
		}
	}
}

void DatabaseLogger::logMessage(Kopete::Message &message)
{
	QSqlQuery query(db);

	query.prepare("INSERT INTO `messages` (`timestamp`, `message`, `account`, `protocol`, `direction`, `importance`, `contact`, `subject`, "
		      " `session`, `session_name`, `from`, `from_name`, `to`, `to_name`, `state`, `type`, `is_group`) "
		      " VALUES (:timestamp, :message, :account, :protocol, :direction, :importance, :contact, :subject, :session, :session_name, "
		      " :from, :from_name, :to, :to_name, :state, :type, :is_group)");

	query.bindValue(":timestamp", message.timestamp().toString("yyyyMMddHHmmsszzz"));
	query.bindValue(":message", message.parsedBody());
	query.bindValue(":account", message.manager()->account()->accountId());
	query.bindValue(":protocol", message.manager()->account()->protocol()->pluginId());
	query.bindValue(":direction", QString::number(message.direction()));
	query.bindValue(":importance", QString::number(message.importance()));
	query.bindValue(":contact", "");
	query.bindValue(":subject", message.subject());
	query.bindValue(":session", "");
	query.bindValue(":session_name", "");
	query.bindValue(":from", message.from()->contactId());
	query.bindValue(":from_name", message.from()->displayName());
	if (message.to().count() == 1) {
		query.bindValue(":to", message.to().at(0)->contactId());
		query.bindValue(":to_name", message.to().at(0)->displayName());
		query.bindValue(":is_group", "0");
	} else {
		//Save the to and to_name fields as comma delimited lists of the recepient ids and names
		QString to, to_name;
		for (int i = 0; i < message.to().count(); i++) {
			to.append(message.to().at(i)->contactId() + ",");
			to_name.append(message.to().at(i)->displayName() + ",");
		}
		to.chop(1);
		to_name.chop(1);
		query.bindValue(":to", to);
		query.bindValue(":to_name", to_name);
		query.bindValue(":is_group", "1");
	}
	query.bindValue(":state", QString::number(message.state()));
	query.bindValue(":type", QString::number(message.type()));

	query.exec();
}

QList<Kopete::Message> DatabaseLogger::search(QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}

QList<Kopete::Message> DatabaseLogger::search(Kopete::Account *account, Kopete::Contact *remote_contact, QDate startDate, QDate endDate, QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}


