#ifndef CHATHISTORYHANDLER_H
#define CHATHISTORYHANDLER_H

#include <QVariantList>
#include "kopeteplugin.h"
#include <QObject>

namespace Kopete {
	class Message;
}

/**
 * The ChatHistoryHandler class is used to handle all history connected activities, and
 * then call the respective class (logger, searcher etc).
 */
class ChatHistoryHandler : public Kopete::Plugin
{
	Q_OBJECT
public:
	/**
     * Constructs a new ChatHistoryHandler class instance. There should only be one
      * instance for every instance of Kopete running.
      */
	explicit ChatHistoryHandler(QObject *parent, const QVariantList &);

	~ChatHistoryHandler();
	static ChatHistoryHandler *instance();

public slots:
	/**
     * Insert a new chat message to the database.
	 * @param message The message to be logged. The message details to be stored in the database
	 * will be extracted from here.
	 */
	void logMessage(Kopete::Message &message);
private:
	static ChatHistoryHandler *mInstance;
};

#endif // CHATHISTORYHANDLER_H
