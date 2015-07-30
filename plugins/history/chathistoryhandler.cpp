#include "chathistoryhandler.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"

ChatHistoryHandler *ChatHistoryHandler::mInstance = 0;

K_PLUGIN_FACTORY(HistoryPluginFactory, registerPlugin<ChatHistoryHandler>();)
K_EXPORT_PLUGIN(HistoryPluginFactory( "kopete_history" ))

ChatHistoryHandler::ChatHistoryHandler(QObject *parent, const QVariantList &)
	: Kopete::Plugin(HistoryPluginFactory::componentData(), parent)
{

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
