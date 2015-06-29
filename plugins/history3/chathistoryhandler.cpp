#include "chathistoryhandler.h"
#include "kopetemessage.h"

#include <QDate>

ChatHistoryHandler::ChatHistoryHandler(QObject *parent)
	: QObject(parent)
{
	//Initialize the database. Currently this is only targeting SQLite, but once we
	//add more database systems, we will pick the database system defined in the
	//preferences
	DatabaseManager::instance()->initDatabase(DatabaseManager::SQLITE);
}

ChatHistoryHandler::~ChatHistoryHandler()
{

}

void ChatHistoryHandler::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
}

QList<Kopete::Message> ChatHistoryHandler::search(QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}

QList<Kopete::Message> ChatHistoryHandler::search(Kopete::Account *account, Kopete::Contact *remote_contact, QDate startDate, QDate endDate, QString searchText)
{
	QList<Kopete::Message> searchResults;

	return searchResults;
}


