#include "history3plugin.h"

#include <kaboutdata.h>
#include <kgenericfactory.h>

#include "kopetechatsessionmanager.h"

K_PLUGIN_FACTORY(History3PluginFactory, registerPlugin<History3Plugin>();)
K_EXPORT_PLUGIN(History3PluginFactory( "kopete_history3" ))


History3Plugin::History3Plugin(QObject *parent, const QVariantList &/*args*/)
 : Kopete::Plugin(History3PluginFactory::componentData(), parent)
{
	chatHandler = new ChatHistoryHandler(this);

	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(handleKopeteMessage(Kopete::Message&)));
}

History3Plugin::~History3Plugin()
{

}

void History3Plugin::handleKopeteMessage(Kopete::Message &msg)
{
	chatHandler->logMessage(msg);
}
