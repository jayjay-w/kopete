#include "databasehelper.h"

#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopeteaccount.h"

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
						"\"is_group\" Integer Default='0') "
						);
		}
	}
}

void DatabaseHelper::logMessage(Kopete::Message *message)
{

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


