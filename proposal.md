
# Improved Kopete History Plugin

#### Main Goals by Joshua:
 * Improve current set of plugins - come up with a better fully functional plugin
 * Import chats from more IM applications
 * Export chat to CSV, HTML etc
 * History backup/restore

#### What current plugins lack:
 * Good Search interface
 * HTML message support
 * flexible SQL backend
 * better UI
 * Slow Speed
 * Bad Search support
 * Bad html support
 * bad support for multi user chat
 * import/export feature ( -> import from gmail/facebook)

#### Primary Goals:
* To structure the code in such a way that it is easy to follow, even by a non developer.
* Write proper comments wherever required - and - also if not required :)


#### Comments:
* Lack of OOO experience - class naming convention in OOO

The plugin shall be made up of these core classes, which will have the same format and naming used in the current plugin:

##### DatabaseManager
This is the database interface class. It will contain the functions that will be used for communication with the database.

###### Structure:
```cpp
public:
	enum DatabaseType {
		SQLITE = 0
	};

	~DatabaseManager();
	
	initDatabase(DatabaseType dbType);

	void insertMessage(Kopete::Message &message);

	static DatabaseManager *instance();
private:

	explicit DatabaseManager(QObject *parent = 0);

	QSqlDatabase db;

	static DatabaseManager *mInstance;	
```

#### ChatHistoryHandler
This class will be used to coordinate the other history classes (Logging, Searching, Backup/Restore).

This is the current structure, which is still being improved:
```cpp
	explicit ChatHistoryHandler(QObject *parent = 0);

	~ChatHistoryHandler();

	void logMessage(Kopete::Message &message);

	QList<Kopete::Message> search(QString searchText);

	QList<Kopete::Message> search(Kopete::Account *account, Kopete::Contact *remote_contact, QDate startDate,
				      QDate endDate, QString searchText);

	static ChatHistoryHandler *instance();
private:
	static ChatHistoryHandler *mInstance;
```

##### HistoryPlugin
The main plugin class. I will use this to initialize the plugin, and connect the main application to the plugin.  There will be only one instance of this class.

The structure is similar to the current plugins’:
```cpp
class History3Plugin : public Kopete::Plugin
{
	Q_OBJECT
public:
	History3Plugin(QObject *parent, const QVariantList &);
	~History3Plugin();

public slots:
	void handleKopeteMessage(Kopete::Message &msg);
	void viewHistory();
private:
	ChatHistoryHandler *chatHandler;
};
```
##### HistoryImport/History Export
These classes will be used to import and export Kopete logs, and logs to/from other IM applications. I plan on making it plugin aware, so that it will be easy to add support for other IM applications.

The import feature will use XML. The plugins will parse the source application’s logs, and then pass them to the HistoryImport class as an XML string for logging.

###### Structure:
```cpp
	Import;
	HistoryImport(); //- Class constructor;
	~HistoryImport(); //- Class destructor;
	
	bool startImport(QString XML);  //- A function to parse the provided XML string and log it to the database.
	void previewImport(QString XML); //- This will display a preview of the Imported chats, and will allow the user to confirm before importing.
```

##### Export

Chats will be exported to destinations such as HTML, XML, text documents etc.	
```cpp
	HistoryExport(); //- Class constructor
	~HistoryExport(); //- Class destructor

	bool startExport(QList<Kopete::Message> chats, Kopete::History::ExportDest destination); //- Start the export process to the selected destination.

```

##### HistoryPreferences
This will have a user interface, and it will allow the user to configure the plugin. The user will be able to configure visual aspects of the plugin such as colors, fonts, number of lines to display etc. The user will also be able to configure the default logging destination, and if an RDBMS is selected, they will be given an option to specify the database system details such as credentials and server names.

##### Others
There will be other classes, more so the ones for handling database threads and chat backups. I will update this document as deeded.


## Tentative Timeline
### Before community bonding
Continue familiarizing myself with Kopete's code structure, and do more research on the history plugin.

### 27 April to 25th May
##### Community Bonding Period
I will use this period to work with my mentors to come up with a draft of the class structure to be used.
During this period, I will also continue sharpening my understanding of Kopete imaging and do more research on the tasks to do over the summer.

### Week 1 (27th May - 1st June)
#####Code Preparation
The first step will be to start working on the existing classes that need to be expanded. By the end of these two weeks, I will have started writing the new classes.

### Week 2 (1st June - 7th June)
##### Database structure
The database structure will be as follows:

###### Table: messages
```sql
CREATE TABLE "messages" (  
   "id" Integer Primary Key Autoincrement Not Null, --Unique message identifier
   "timestamp" Integer, --When the message was handled
   "message" Text, --HTML containing the message contents
   "protocol" Text Not Null, --Protocol used (Kopete::Protocol::pluginId())
   "account" Text Not Null, --Account used (Kopete::Account::accountId())
   "direction" Integer Not Null, --(Inbound = 0, Outbound=1, Internal=2) (Kopete::Message::MessageDirection)
   "importance" Integer, -- (Low, Normal, Highlight) (Kopete::Message) (Kopete::Message::MessageImportance)
   "contact" Text, -- The local contact used in this message (if applicable). (Kopete::Contact::ContactId()).
   "subject" Text, --If applicable, this will store the subject of the message
   "session" Text, -- Internal session identifier.
   "session_name" Text, -- A human readable name for the session.
   "from" Text, --Internal identifier for the message sender
   "from_name" Text, --Human readable name of the message sender
   "to" Text, --Internal identifier for the message recipient
   "to_name" Text, --Human readable name of the message recipient.
   "state" Integer, --(Unknown = 0, Sending = 1, Sent = 2, Error = 3)
   "type" Integer, --The type of message. (TypeNormal, TypeAction, TypeFileTransferRequest, TypeVoiceClipRequest) (Kopete::Message::MessageType)
   "is_group" Integer Default='0' --If this is set to 1, then we know we are in multi user mode.
```

### By Mid Term
##### Chat logging and Mid term preparation
Here, I will implement chat logging. The plugin should be able to store chats to the database.

I will prepare for the mid term evaluation, by ensuring that the code written before this period is up to spec and working as intended.

### Weeks 6 (6th July - 11th July)
##### Chat browser and improved database communication
I will implement a GUI for browsing of chats stored in the database. I also plan to move all of the database code to
a different thread to avoid blocking of the UI thread,

###Week 7 (12th July - 19th July)
##### Import and Export
I shall dedicate this week to work on the import/export feature. Kopete should be able to import logs from other instant messaging apps. On adding of a new account, it should check the config paths, and detect if any of the other apps such as Telepathy have logs for that particular account. We can then prompt the user on whether or not to import these logs.
I shall also work on the export feature, where the user can export the logs as text files, or even stand alone html documents that can be viewed on the browser.

### Week 8 (20th July - 26th July)
##### MySQL/Postgre SQL
I shall take time to implement other RDBMS. This will include:
-A UI for entering database credentials
-Proper logging of chats to the selected database system.

### Week 9 (27 July - 2nd August)
##### Backup / Restore
I shall work on a feature to enable the user back up the chat history, and restore it later. This backup will be saved in a compressed file, and It can be used when migrating to a new computer.

### Week 10 (3rd August - 9th August)
##### Testing and UI Polishing
This week shall be dedicated to thorough testing, UI polishing and other improvements. I shall also focus on speed improvements, in the searching of logs, importing and exporting.

### Rest of the Program (Until 17th August)
##### Bug fixes
I will focus on bug fixes based on users' and developers' feedback and documentation writing. I have also dedicated a lot of time for this last segment, so that it can cater for any unforeseen delays in the preceding segments.

I will also start preparing for the code submission to Google.


