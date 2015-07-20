#ifndef CHATHISTORYHANDLER_H
#define CHATHISTORYHANDLER_H

#include "databasemanager.h"
#include <QObject>

class QDate;

/**
 * @brief The ChatHistoryHandler class is used to handle all history connected activities, and
 * then class the respective class (logger, searcher etc).
 */
class ChatHistoryHandler : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief Constructs a new ChatHistoryHandler class instance. There should only be one
	 * instance for every instance of Kopete running.
	 */
	explicit ChatHistoryHandler(QObject *parent = 0);

	~ChatHistoryHandler();

	/**
	 * @brief Insert a new chat message to the database.
	 * @param message The message to be logged. The message details to be stored in the database
	 * will be extracted from here.
	 */
	void logMessage(Kopete::Message &message);

	/**
	 * @brief Search the chat history database for a specific string in the message.
	 * @param text The string to search for.
	 * @return A list of messages matching the search criteria.
	 */
	QList<Kopete::Message> search(QString searchText);

	/**
	 * @brief Search the history for chats with a specific contact
	 * @param account The account we want to seach chats for
	 * @param remote_contact The contact whose messages we want to search
	 * @param startDate Start date for the search period
	 * @param endDate End date for the search period
	 * @param searchText The string to search for
	 * @return A list of messages matching the search criteria
	 */
	QList<Kopete::Message> search(Kopete::Account *account, Kopete::Contact *remote_contact, QDate startDate,
				      QDate endDate, QString searchText);

	/**
	 * @brief This is used to get the database helper for the current Kopete instance.
	 * @return a pointer to DatabaseHelper
	 */
	static ChatHistoryHandler *instance();
private:
	static ChatHistoryHandler *mInstance;
};

#endif // CHATHISTORYHANDLER_H
