#include "historyplugin.h"

#include <kaboutdata.h>
#include <kgenericfactory.h>

#include "kopetechatsessionmanager.h"

K_PLUGIN_FACTORY(HistoryPluginFactory, registerPlugin<HistoryPlugin>();)
K_EXPORT_PLUGIN(HistoryPluginFactory( "kopete_history" ))


HistoryPlugin::HistoryPlugin(QObject *parent, const QVariantList &/*args*/)
 : Kopete::Plugin(HistoryPluginFactory::componentData(), parent)
{
	chatHandler = new ChatHistoryHandler(this);

	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(handleKopeteMessage(Kopete::Message&)));
}

HistoryPlugin::~HistoryPlugin()
{

}

void HistoryPlugin::handleKopeteMessage(Kopete::Message &msg)
{
	chatHandler->logMessage(msg);
}
