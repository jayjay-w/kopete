#ifndef DATABASEHELPER_H
#define DATABASEHELPER_H

#include <QObject>
#include <QSqlDatabase>

namespace Kopete {
	class Account;
	class Contact;
	class Message;
	class Protocol;
}

class QDate;

class DatabaseHelper : public QObject
{
	Q_OBJECT
public:
	enum dbType {
		SQLITE = 0,
		MYSQL = 1,
		POSTGRESQL = 2
	};

	/**
	 * @brief Constructs a new DataBaseHelper class instance. There should only be one
	 * instance for every instance of Kopete running.
	 */
	explicit DatabaseHelper(QObject *parent = 0);

	~DatabaseHelper();

	/**
	 * @brief Initializes the database. This should create the db based on the preferences
	 * that the user has set.
	 * @param dbType
	 */
	void initDatabase(dbType dbType);

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
	static DatabaseHelper *instance();
private:
	static DatabaseHelper *mInstance;
	QSqlDatabase db;
};

#endif // DATABASEHELPER_H
