#include "databaselogger.h"
#include "kopetemessage.h"

#include <QDate>

DatabaseLogger::DatabaseLogger(QObject *parent)
	: QObject(parent)
{
	//Initialize the database. Currently this is only targeting SQLite, but once we
	//add more database systems, we will pick the database system defined in the
	//preferences
	DatabaseManager::instance()->InitDatabase(DatabaseManager::SQLITE);
}

DatabaseLogger::~DatabaseLogger()
{

}

void DatabaseLogger::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
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


