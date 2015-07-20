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
#include <QSqlQuery>
#include <QVariant>


DatabaseManager* DatabaseManager::mInstance = 0;

DatabaseManager::DatabaseManager(QObject *parent)
	: QObject(parent)
{
	mInstance = this;
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
		QString dbPath = KStandardDirs::locateLocal("appdata", "kopete_history3.db");
		db = QSqlDatabase::addDatabase("QSQLITE", "kopete-history");
		db.setDatabaseName(dbPath);

		//Abort in case the database creation/opening fails.
		if (!db.open()) {
			//Database not open
			return;
		}

		//If the messages table does not exist, lets create it.
		db.exec(
					"CREATE TABLE IF NOT EXISTS \"messages\" ("
                    "\"" + DatabaseConstants::columnId() + "\" Integer Primary Key Autoincrement Not Null, "
                    "\"" + DatabaseConstants::columnTimeStamp() + "\" Integer, "
                    "\"" + DatabaseConstants::columnMessage() + "\" Text, "
                    "\"" + DatabaseConstants::columnProtocol() + "\" Text Not Null, "
                    "\"" + DatabaseConstants::columnAccount() + "\" Text Not Null, "
                    "\"" + DatabaseConstants::columnDirection() + "\" Integer Not Null, "
                    "\"" + DatabaseConstants::columnImportance() + "\" Integer, "
                    "\"" + DatabaseConstants::columnContact() + "\" Text, "
                    "\"" + DatabaseConstants::columnSubject() + "\" Text, "
                    "\"" + DatabaseConstants::columnSession() + "\" Text, "
                    "\"" + DatabaseConstants::columnSessionName() + "\" Text, "
                    "\"" + DatabaseConstants::columnFrom() + "\" Text, "
                    "\"" + DatabaseConstants::columnFromName() + "\" Text, "
                    "\"" + DatabaseConstants::columnTo() + "\" Text, "
                    "\"" + DatabaseConstants::columnToName() + "\" Text, "
                    "\"" + DatabaseConstants::columnState() + "\" Integer, "
                    "\"" + DatabaseConstants::columnType() + "\" Integer, "
                    "\"" + DatabaseConstants::columnIsGroup() + "\" Integer Default'0') "
					);
	}
}

void DatabaseManager::insertMessage(Kopete::Message &message)
{
	QSqlQuery query(db);

	//Prepare an SQL query to insert the message to the db
	query.prepare("INSERT INTO `messages` (`timestamp`, `message`, `account`, `protocol`, `direction`, `importance`, `contact`, `subject`, "
		      " `session`, `session_name`, `from`, `from_name`, `to`, `to_name`, `state`, `type`, `is_group`) "
		      " VALUES (:timestamp, :message, :account, :protocol, :direction, :importance, :contact, :subject, :session, :session_name, "
		      " :from, :from_name, :to, :to_name, :state, :type, :is_group)");

	//Add the values to the database fields.
    query.bindValue(":" + DatabaseConstants::columnTimeStamp() , QString::number(message.timestamp().toTime_t()));
    query.bindValue(":" + DatabaseConstants::columnMessage(), message.parsedBody());
    query.bindValue(":" + DatabaseConstants::columnAccount(), message.manager()->account()->accountId());
    query.bindValue(":" + DatabaseConstants::columnProtocol(), message.manager()->account()->protocol()->pluginId());
    query.bindValue(":" + DatabaseConstants::columnDirection(), QString::number(message.direction()));
    query.bindValue(":" + DatabaseConstants::columnImportance(), QString::number(message.importance()));

	QString contact;
	if (message.direction() == Kopete::Message::Outbound) {
		contact = message.to().at(0)->contactId();
	} else {
		contact = message.from()->contactId();
	}
    query.bindValue(":" + DatabaseConstants::columnContact(), contact);

    query.bindValue(":" + DatabaseConstants::columnSubject(), message.subject());
    query.bindValue(":" + DatabaseConstants::columnSession(), "");
    query.bindValue(":" + DatabaseConstants::columnSessionName(), "");
    query.bindValue(":" + DatabaseConstants::columnFrom(), message.from()->contactId());
    query.bindValue(":" + DatabaseConstants::columnFromName(), message.from()->displayName());

	//If we are dealing with only one recepient, we save this as a single user chat
	if (message.to().count() == 1) {
        query.bindValue(":" + DatabaseConstants::columnTo(), message.to().at(0)->contactId());
        query.bindValue(":" + DatabaseConstants::columnToName(), message.to().at(0)->displayName());
        query.bindValue(":" + DatabaseConstants::columnIsGroup(), "0");
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
        query.bindValue(":" + DatabaseConstants::columnTo(), to);
        query.bindValue(":" + DatabaseConstants::columnToName(), to_name);
        query.bindValue(":" + DatabaseConstants::columnIsGroup(), "1");
	}
    query.bindValue(":" + DatabaseConstants::columnState(), QString::number(message.state()));
    query.bindValue(":" + DatabaseConstants::columnType(), QString::number(message.type()));

	//Save the query to the database.
	query.exec();
}

DatabaseManager *DatabaseManager::instance()
{
	//Check if there is an instance of this class. If there is none, a new one will be created.
	if (!mInstance)
		mInstance = new DatabaseManager(kapp);

	return mInstance;
}
