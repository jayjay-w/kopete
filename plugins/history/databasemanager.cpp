#include "kapplication.h"
#include "databasemanager.h"
#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopeteaccount.h"
#include "kopetechatsession.h"
#include "kopeteprotocol.h"
#include "kopetechatsessionmanager.h"

#include "databaseconstants.h"

#include <KStandardDirs>
#include <QVariant>


DatabaseManager* DatabaseManager::mInstance = 0;

DatabaseManager::DatabaseManager(QObject *parent)
	: QObject(parent)
{
	mInstance = this;
}

void DatabaseManager::bindQueryValue(QSqlQuery query, QString columnName, QString value)
{
	query.bindValue(columnName, value);
}

DatabaseManager::~DatabaseManager()
{

}

void DatabaseManager::initDatabase(DatabaseManager::DatabaseType dbType)
{
	//Close the database, in case it is open.
	if (db.isOpen())
		db.close();

	//Create the database tables based on the selected database system
	switch (dbType) {
	case SQLITE:
		//For SQLite, we store the database in the user's application data folder.
		QString dbPath = KStandardDirs::locateLocal("appdata", "kopete_history.db");
		db = QSqlDatabase::addDatabase("QSQLITE", "kopete-history");
		db.setDatabaseName(dbPath);

		//Abort in case the database creation/opening fails.
		if (!db.open()) {
			//Database not open
			return;
		}

		//If the messages table does not exist, lets create it.
		db.exec(DatabaseConstants::sql_createMessagesTable());
	}
}

void DatabaseManager::insertMessage(Kopete::Message &message)
{
	QSqlQuery query(db);

	//Prepare an SQL query to insert the message to the db
	query.prepare(DatabaseConstants::sql_prepareForMessageInsert());

	//Add the values to the database fields.
	bindQueryValue(query, DatabaseConstants::columnTimeStamp(), QString::number(message.timestamp().toTime_t()));
	bindQueryValue(query, DatabaseConstants::columnMessage(), message.parsedBody());
	bindQueryValue(query, DatabaseConstants::columnAccount(), message.manager()->account()->accountId());
	bindQueryValue(query, DatabaseConstants::columnProtocol(), message.manager()->account()->protocol()->pluginId());
	bindQueryValue(query, DatabaseConstants::columnDirection(), QString::number(message.direction()));
	bindQueryValue(query, DatabaseConstants::columnImportance(), QString::number(message.importance()));

	QString contact;
	if (message.direction() == Kopete::Message::Outbound) {
		contact = message.to().at(0)->contactId();
	} else {
		contact = message.from()->contactId();
	}
	bindQueryValue(query, DatabaseConstants::columnContact(), contact);

	bindQueryValue(query, DatabaseConstants::columnSubject(), message.subject());
	bindQueryValue(query, DatabaseConstants::columnSession(), "");
	bindQueryValue(query, DatabaseConstants::columnSessionName(), "");
	bindQueryValue(query, DatabaseConstants::columnFrom(), message.from()->contactId());
	bindQueryValue(query, DatabaseConstants::columnFromName(), message.from()->displayName());

	//If we are dealing with only one recepient, we save this as a single user chat
	if (message.to().count() == 1) {
		bindQueryValue(query, DatabaseConstants::columnTo(), message.to().at(0)->contactId());
		bindQueryValue(query, DatabaseConstants::columnToName(), message.to().at(0)->displayName());
		bindQueryValue(query, DatabaseConstants::columnIsGroup(), "0");
	} else {
		//Otherwise we will create a string with the contact ids of the recepients, and another string to
		//hold the contact names of the recepients. Both these strings are comma delimited.
		QString to, to_name;
		foreach (Kopete::Contact *c, message.to()) {
			to.append(c->contactId() + ",");
			to_name.append(c->displayName() + ",");
		}
		to.chop(1);
		to_name.chop(1);
		bindQueryValue(query, DatabaseConstants::columnTo(), to);
		bindQueryValue(query, DatabaseConstants::columnToName(), to_name);
		bindQueryValue(query, DatabaseConstants::columnIsGroup(), "1");
	}
	bindQueryValue(query, DatabaseConstants::columnState(), QString::number(message.state()));
	bindQueryValue(query, DatabaseConstants::columnType(), QString::number(message.type()));

	//Save the query to the database.
	query.exec();
}

DatabaseManager *DatabaseManager::instance()
{
	//Check if there is an instance of this class. If there is none, a new one will be created.
	if (!mInstance) {
		mInstance = new DatabaseManager(kapp);
	}

	return mInstance;
}