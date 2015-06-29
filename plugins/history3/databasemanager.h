#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>

namespace Kopete {
	class Account;
	class Contact;
	class Message;
	class Protocol;
}

/**
 * @brief The DatabaseManager class is used to manage all database connected activities
 * of the history plugin.
 */
class DatabaseManager : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief List of the different database systems we use. SQLite is the default,
	 * and the user is able to select other database systems from the plugin
	 * preferences.
	 */
	enum DatabaseType {
		SQLITE = 0
	};

	/**
	  * Class destructor.
	  * */
	~DatabaseManager();

	/**
	 * @brief Initializes the database based on the selected database format.
	 * This creates the database if it does not exist. For SQLite, this entails
	 * creating the kopete_history3.db file in the application data folder. For other
	 * database systems, the database is created on the server, and then all of the
	 * needed tables are be initialized.
	 * @param dbType
	 */
	void initDatabase(DatabaseType dbType);

	/**
	 * @brief Inserts a chat message into the database. All chat related details such as
	 * protocol, subject, recepients etc are also logged into the database.
	 * @param message a Kopete::Message object containing the message to be logged into the
	 * database.
	 */
	void insertMessage(Kopete::Message &message);

	/**
	 * @brief The current instance of the DatabaseManager class.
	 * @return the value of mInstance
	 */
	static DatabaseManager *instance();
private:
	/**
	 * @brief DatabaseManager class constructor. If there is no instance of the class in
	 * existance, a new one will be created.
	 * @param parent The QObject parent of this class
	 */
	explicit DatabaseManager(QObject *parent = 0);

	/**
	 * @brief QSqlDatabase containing the database used to log the chats.
	 */
	QSqlDatabase db;

	/**
	 * @brief The current instance of the DatabaseManager class.
	 */
	static DatabaseManager *mInstance;
};

#endif // DATABASEMANAGER_H
