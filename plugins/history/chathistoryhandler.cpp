#include "chathistoryhandler.h"
#include "kopetemessage.h"
#include "kopetechatsessionmanager.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"

#include <QDate>

ChatHistoryHandler *ChatHistoryHandler::mInstance = 0;

K_PLUGIN_FACTORY(HistoryPluginFactory, registerPlugin<ChatHistoryHandler>();)
K_EXPORT_PLUGIN(HistoryPluginFactory( "kopete_history" ))

ChatHistoryHandler::ChatHistoryHandler(QObject *parent, const QVariantList &)
	: Kopete::Plugin(HistoryPluginFactory::componentData(), parent)
{
	//Initialize the database. Currently this is only targeting SQLite, but once we
	//add more database systems, we will pick the database system defined in the
	//preferences
	DatabaseManager::instance()->initDatabase(DatabaseManager::SQLITE);
	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(logMessage(Kopete::Message&)));
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

ChatHistoryHandler *ChatHistoryHandler::instance()
{
	if (!mInstance)
		mInstance = new ChatHistoryHandler(0, QVariantList());

	return mInstance;
}
