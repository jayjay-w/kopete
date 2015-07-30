#include "chathistoryhandler.h"
#include "databasemanager.h"
#include "kopetemessage.h"
#include "kopetechatsessionmanager.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"

ChatHistoryHandler *ChatHistoryHandler::mInstance = 0;

typedef KGenericFactory<ChatHistoryHandler> HistoryPluginFactory;
static const KAboutData aboutData("kopete_history", 0, ki18n("History"), "0.1", ki18n("Kopete history logging application."));
K_EXPORT_COMPONENT_FACTORY(kopete_history, HistoryPluginFactory(&aboutData))
ChatHistoryHandler::ChatHistoryHandler(QObject *parent, const QStringList &)
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

ChatHistoryHandler *ChatHistoryHandler::instance()
{
	if (!mInstance) {
		mInstance = new ChatHistoryHandler(0, QStringList());
	}

	return mInstance;
}

void ChatHistoryHandler::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
}
