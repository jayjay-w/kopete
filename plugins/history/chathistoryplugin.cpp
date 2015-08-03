#include "chathistoryplugin.h"
#include "databasemanager.h"
#include "kopetemessage.h"
#include "kopetechatsessionmanager.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"

ChatHistoryPlugin *ChatHistoryPlugin::mInstance = 0;

K_PLUGIN_FACTORY(ChatHistoryPluginFactory, registerPlugin<ChatHistoryPlugin>();)
K_EXPORT_PLUGIN(ChatHistoryPluginFactory( "kopete_history" ))

ChatHistoryPlugin::ChatHistoryPlugin(QObject *parent, const QVariantList &)
	: Kopete::Plugin(ChatHistoryPluginFactory::componentData(), parent)
{
	//Initialize the database.
	//TODO: Implement other DB Systems (MySQL, PostgreSQL etc)
	DatabaseManager::instance()->initDatabase(DatabaseManager::SQLITE);
	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)),
		 this, SLOT(logMessage(Kopete::Message&)));
}

ChatHistoryPlugin::~ChatHistoryPlugin()
{

}

ChatHistoryPlugin *ChatHistoryPlugin::instance()
{
	if (!mInstance) {
		mInstance = new ChatHistoryPlugin(0, QVariantList());
	}

	return mInstance;
}

void ChatHistoryPlugin::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
}
