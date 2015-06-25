#include "databasehelper.h"

#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopeteaccount.h"
#include "kopetechatsession.h"
#include "kopeteprotocol.h"

#include <QDate>
#include <KStandardDirs>
#include <QSqlQuery>
#include <QDebug>

DatabaseHelper::DatabaseHelper(QObject *parent)
	: QObject(parent)
{
	qDebug() << "Initializing history3 plugin";
	initDatabase(SQLITE);
}

DatabaseHelper::~DatabaseHelper()
{
	if (db.open()) {
		db.close();
	}
}

void DatabaseHelper::initDatabase(DatabaseHelper::dbType dbType)
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
						"\"timestamp\" Text, "
						"\"message\" Text, "
						"\"protocol\" Text "
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

#include <QSqlError>
#include <QMessageBox>
void DatabaseHelper::logMessage(Kopete::Message &message)
{
	QSqlQuery query(db);

	query.prepare("INSERT INTO `messages` (`timestamp`, `message`, `protocol`, `direction`, `importance`, `contact`, `subject`, "
		      "`session`, `session_name`, `from`, `from_name`, `to`, `to_name`, `state`, `type`, `is_group`) "
		      " VALUES (:timestamp, :message, :protocol, :direction, :importance, :contact, :subject, :session, :session_name, "
		      ":from, :from_name, :to, :to_name, :state, :type, :is_group)");

	query.bindValue(":timestamp", message.timestamp().toString());
	query.bindValue(":message", message.parsedBody());
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
		query.bindValue(":is_group", "1");
	}
	query.bindValue(":state", QString::number(message.state()));
	query.bindValue(":type", QString::number(message.type()));
	query.bindValue(":is_group", "");

	query.exec();

	if (query.lastError().isValid()) {
		QMessageBox::critical(0, "Error", QDateTime::currentDateTime().toString() + "\n" + query.lastError().text() + "\n" + query.lastQuery());
	}
}

QList<Kopete::Message> DatabaseHelper::search(QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}

QList<Kopete::Message> DatabaseHelper::search(Kopete::Account *account, Kopete::Contact *remote_contact, QDate startDate, QDate endDate, QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}


