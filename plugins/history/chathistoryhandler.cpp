#include "chathistoryhandler.h"
#include "databasemanager.h"
#include "kopetemessage.h"
#include "kopetechatsessionmanager.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"

ChatHistoryHandler *ChatHistoryHandler::mInstance = 0;

K_PLUGIN_FACTORY(HistoryPluginFactory, registerPlugin<ChatHistoryHandler>();)
K_EXPORT_PLUGIN(HistoryPluginFactory( "kopete_history" ))

ChatHistoryHandler::ChatHistoryHandler(QObject *parent, const QVariantList &)
	: Kopete::Plugin(HistoryPluginFactory::componentData(), parent)
{
	//Initialize the database.
	//TODO: Implement other DB Systems (MySQL, PostgreSQL etc)
	DatabaseManager::instance()->initDatabase(DatabaseManager::SQLITE);
	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(logMessage(Kopete::Message&)));
}

ChatHistoryHandler::~ChatHistoryHandler()
{

}

ChatHistoryHandler *ChatHistoryHandler::instance()
{
	if (!mInstance) {
		mInstance = new ChatHistoryHandler(0, QVariantList());
	}

	return mInstance;
}

void ChatHistoryHandler::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
}
