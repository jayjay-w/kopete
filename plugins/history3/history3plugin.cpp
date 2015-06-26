#include "history3plugin.h"

#include <kaboutdata.h>
#include <kgenericfactory.h>

#include "kopetechatsessionmanager.h"

typedef KGenericFactory<History3Plugin> History3PluginFactory;
static const KAboutData aboutData("kopete_history3", 0, ki18n("History3"), "0.0.1");

K_EXPORT_COMPONENT_FACTORY(kopete_history3, History3PluginFactory(&aboutData))

History3Plugin::History3Plugin(QObject *parent, const QStringList &/*args*/)
 : Kopete::Plugin(History3PluginFactory::componentData(), parent)
{
	dbHelper = new DatabaseLogger(this);
	dbHelper->initDatabase(DatabaseLogger::SQLITE);

	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(handleKopeteMessage(Kopete::Message&)));
}

History3Plugin::~History3Plugin()
{

}

void History3Plugin::handleKopeteMessage(Kopete::Message &msg)
{
	dbHelper->logMessage(msg);
}
