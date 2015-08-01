#include "chathistoryplugin.h"
#include "databasemanager.h"
#include "kopetemessage.h"
#include "kopetechatsessionmanager.h"
#include <kaboutdata.h>
#include <kgenericfactory.h>
#include "kapplication.h"
#include <kaction.h>
#include <kactioncollection.h>
#include "historybrowser.h"

ChatHistoryPlugin *ChatHistoryPlugin::mInstance = 0;

typedef KGenericFactory<ChatHistoryPlugin> HistoryPluginFactory;
static const KAboutData aboutData("kopete_history", 0, ki18n("History"), "0.1", ki18n("Kopete history logging application."));
K_EXPORT_COMPONENT_FACTORY(kopete_history, HistoryPluginFactory(&aboutData))
ChatHistoryPlugin::ChatHistoryPlugin(QObject *parent, const QStringList &)
	: Kopete::Plugin(HistoryPluginFactory::componentData(), parent)
{
	//Initialize the database.
	//TODO: Implement other DB Systems (MySQL, PostgreSQL etc)
	DatabaseManager::instance()->initDatabase(DatabaseManager::SQLITE);
	connect (Kopete::ChatSessionManager::self(), SIGNAL(aboutToDisplay(Kopete::Message&)), this, SLOT(logMessage(Kopete::Message&)));

	KAction *viewHistoryAction = new KAction(KIcon("view-history"), i18n("View &History"), this);
	actionCollection()->addAction("viewHistoryAction", viewHistoryAction);
	viewHistoryAction->setShortcut(KShortcut(Qt::CTRL + Qt::Key_H));
	connect (viewHistoryAction, SIGNAL(triggered(bool)), this, SLOT(viewHistory()));
	setXMLFile("historyui.rc");

	//When a new chat window is created, fire the chatViewCreated slot
	connect (Kopete::ChatSessionManager::self(), SIGNAL(viewCreated(KopeteView*)), SLOT(chatViewCreated(KopeteView*)));
}

ChatHistoryPlugin::~ChatHistoryPlugin()
{

}

void ChatHistoryPlugin::viewHistory()
{
	HistoryBrowser *historyWindow = new HistoryBrowser(0);
	historyWindow->show();
}

void ChatHistoryPlugin::chatViewCreated(KopeteView *v)
{
	Kopete::ChatSession *currentChatSession = v->msgManager();

	if (!currentChatSession) {
		return;
	}

	const Kopete::ContactPtrList &chatMembers = currentChatSession->members();
	Kopete::Contact *contact = chatMembers.at(0);
	QList<Kopete::Message> messages = DatabaseManager::instance()->getMessages(contact);
	v->appendMessages(messages);
}

ChatHistoryPlugin *ChatHistoryPlugin::instance()
{
	if (!mInstance) {
		mInstance = new ChatHistoryPlugin(0, QStringList());
	}

	return mInstance;
}

void ChatHistoryPlugin::logMessage(Kopete::Message &message)
{
	DatabaseManager::instance()->insertMessage(message);
}
