#ifndef DATABASECONSTANTS_H
#define DATABASECONSTANTS_H

#include <QObject>

/**
 * The DatabaseConstants class is used to provide static representations
 * of the various strings used by the database in the history plugin.
 */
class DatabaseConstants : public QObject
{
	Q_OBJECT
public:
	explicit DatabaseConstants(QObject *parent = 0);
	/**
     * These are the names of the various database columns
     */
	static QString columnId();
	static QString columnTimeStamp();
	static QString columnMessage();
	static QString columnProtocol();
	static QString columnAccount();
	static QString columnDirection();
	static QString columnImportance();
	static QString columnContact();
	static QString columnSubject();
	static QString columnSession();
	static QString columnSessionName();
	static QString columnFrom();
	static QString columnFromName();
	static QString columnTo();
	static QString columnToName();
	static QString columnState();
	static QString columnType();
	static QString columnIsGroup();

	/**
     * Query string for inserting a new message
     */
	static QString prepareForMessageInsert();

	/**
     * Query string for creating the messsages table
     */
	static QString createMessagesTable();
};

#endif // DATABASECONSTANTS_H
