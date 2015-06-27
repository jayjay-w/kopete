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

class DatabaseManager : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief List of the different database systems we use.
	 */
	enum DatabaseType {
		SQLITE = 0,
		MYSQL = 1,
		POSTGRESQL = 2
	};

	explicit DatabaseManager(QObject *parent = 0);

	~DatabaseManager();

	/**
	 * @brief Initialize the database based on the selected database format.
	 * This will create tha database if it does not exist.
	 * @param dbType
	 */
	void InitDatabase(DatabaseType dbType);

	void insertMessage(Kopete::Message &message);

	static DatabaseManager *instance();
private:
	QSqlDatabase db;
	static DatabaseManager *mInstance;
};

#endif // DATABASEMANAGER_H
