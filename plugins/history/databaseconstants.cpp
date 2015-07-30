#include "databaseconstants.h"

QString DatabaseConstants::columnId()
{
	return "id";
}

QString DatabaseConstants::columnTimeStamp()
{
	return "timestamp";
}

QString DatabaseConstants::columnMessage()
{
	return "message";
}

QString DatabaseConstants::columnProtocol()
{
	return "protocol";
}

QString DatabaseConstants::columnAccount()
{
	return "account";
}

QString DatabaseConstants::columnDirection()
{
	return "direction";
}

QString DatabaseConstants::columnImportance()
{
	return "importance";
}

QString DatabaseConstants::columnContact()
{
	return "contact";
}

QString DatabaseConstants::columnSubject()
{
	return "subject";
}

QString DatabaseConstants::columnSession()
{
	return "session";
}

QString DatabaseConstants::columnSessionName()
{
	return "session_name";
}

QString DatabaseConstants::columnFrom()
{
	return "from";
}

QString DatabaseConstants::columnFromName()
{
	return "from_name";
}

QString DatabaseConstants::columnTo()
{
	return "to";
}

QString DatabaseConstants::columnToName()
{
	return "to_name";
}

QString DatabaseConstants::columnState()
{
	return "state";
}

QString DatabaseConstants::columnType()
{
	return "type";
}

QString DatabaseConstants::columnIsGroup()
{
	return "is_group";
}

QString DatabaseConstants::sql_prepareForMessageInsert()
{
	return "INSERT INTO `messages` (`timestamp`, `message`, `account`, `protocol`, `direction`, `importance`, `contact`, `subject`, "
	       " `session`, `session_name`, `from`, `from_name`, `to`, `to_name`, `state`, `type`, `is_group`) "
	       " VALUES (:timestamp, :message, :account, :protocol, :direction, :importance, :contact, :subject, :session, :session_name, "
	       " :from, :from_name, :to, :to_name, :state, :type, :is_group)";
}

QString DatabaseConstants::sql_createMessagesTable()
{
	return "CREATE TABLE IF NOT EXISTS \"messages\" ("
			"\"" + columnId() + "\" Integer Primary Key Autoincrement Not Null, "
			"\"" + columnTimeStamp() + "\" Integer, "
			"\"" + columnMessage() + "\" Text, "
			"\"" + columnProtocol() + "\" Text Not Null, "
			"\"" + columnAccount() + "\" Text Not Null, "
			"\"" + columnDirection() + "\" Integer Not Null, "
			"\"" + columnImportance() + "\" Integer, "
			"\"" + columnContact() + "\" Text, "
			"\"" + columnSubject() + "\" Text, "
			"\"" + columnSession() + "\" Text, "
			"\"" + columnSessionName() + "\" Text, "
			"\"" + columnFrom() + "\" Text, "
			"\"" + columnFromName() + "\" Text, "
			"\"" + columnTo() + "\" Text, "
			"\"" + columnToName() + "\" Text, "
			"\"" + columnState() + "\" Integer, "
			"\"" + columnType() + "\" Integer, "
					      "\"" + columnIsGroup() + "\" Integer Default'0') ";
}

QString DatabaseConstants::sql_getContactList(QString accountId)
{
	return "SELECT DISTINCT contact, protocol FROM messages WHERE account = '" + accountId + "'";
}
