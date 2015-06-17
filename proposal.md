
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

##### DatabaseHelper
This is the database interface class. It will contain the functions that will be used for communication with the database.

###### Structure:
```cpp
public:
	DatabaseHelper(QObject *parent);
	~DatabaseHelper();
	
	static DatabaseHelper *instance();
private:
	static DatabaseHelper *m_instance;
	QSqlDatabase m_db;	
```

#### DbInsertHelper
```cpp
	void insert(Kopete::Contact *from, Kopete::Contact *to, QString message, QDateTime timeStamp = QDateTime::currentDateTime);
```

#### DBSearchHelper
```cpp
	QList<Kopete::Message> search(QString text);
	QList<Kopete::Message> search(Kopete::Contact *contact, QString text);
	QList<Kopete::Message> search(Kopete::Contact *from);
	QList<Kopete::Message> search(Kopete::Contact *to);
	QList<Kopete::Message> search(Kopete::Contact *from, Kopete::Contact *to);
	QList<Kopete::Message> search(Kopete::Contact *from, QDate date);
	QList<Kopete::Message> search(Kopete::Contact *to, QDate date);
```


##### HistoryPlugin
The main plugin class. I will use this to control the other classes.  There will be only one instance of this class, and it will be used to specify the default logging destination (SQLite or other RDBMS).

The structure is the same as the current plugins’:
```cpp
	HistoryPlugin(QObject *parent, QStringList args); //- Class constructor.
	~HistoryPlugin(); //- Destructor
	void messageDisplayed(Kopete::Message m); //- This will handle all incoming and  outgoing messages, and log them to the database.
	void viewHistory(); //- This will be called to display the HistoryViewer dialog.
	void sessionClosed(); //- This will be called whenever a chat session ends, so that the logging to the database can be finalised.
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




##### HistoryLogger
Handling of all the logging. For every open chat window, we will have an instance of the HistoryLogger class, and each message will be logged on receipt and on sending, based on the default logging  destination.

###### Structure:
```cpp
	QList<QDate> getDays(Kopete::MetaContact c, QString search = “”); //- Get a list of days that chats for a particular contact exist.
	void appendMessage(Kopete::Message msg, Kopete::Contact c); //- Append a message to the database.
	bool messageExists(Kopete::Message msg, Kopete::Contact c); //- Check if a particular message for a particular contact exists in the database.
	QList<Kopete::Message> readMessages(QDate date, Kopete::MetaContact contact); //- return a list logged of messages for a particular contact.

	HistoryLogger(); //- Constructor
	~HistoryLogger(); //- Class Destructor
	QSqlDatabase m_db; //- Database used for logging chats
	static HistoryLogger *m_instance; //- Pointer to get the current instance of the class.
```

##### HistoryPreferences
This will have a user interface, and it will allow the user to configure the plugin. The user will be able to configure visual aspects of the plugin such as colors, fonts, number of lines to display etc. The user will also be able to configure the default logging destination, and if an RDBMS is selected, they will be given an option to specify the database system details such as credentials and server names.



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

###### Table: history
Columns:   
* entry_id (int, primary key)
* timestamp (text) - Date and time of the message
* message (text) - The content of the message in HTML format
* contact_name (text) - Name of contact sending/receiving messahe
* subject (text) - If applicable, the message subject
* importance (text?)
* background_coloe (text) - If applicable
* is_group_message (bool) - Is this a group message
* group_id (text) - Unique id of the group, if this is a group message.
* protocol (text) - (the protocol in use)
* direction (text) - (either incoming or outgoing)
* local_id (text) - (the account being used locally)
* remote_id (text) - (the remote account)
* message_type(text) - the type of message (html, plain text etc)

###### Table: groups
* group_id (text) - A unique identifier for the group
* description (text) - A human readable description of the group
* subject (text) - Topic being discussed.
* group_type (text) - Type of group (IRC, Skype etc)

#### Types of groups
For group messages, we will be handling messages under different protocols:
* Skype - For skype, we will use a string hash as the group's unique id.
* IRC - The irc network/channel name can be used as the group unique id. (Eg Freenode/#kopete)
* Jabber - The room name will be used as the unique id

### Week 3 (8th June - 14th June)

##### Chat logging
This will implement chat logging, and also come up with the chat viewer. This will include creating the user interface to browse chats and search them .


### Week 4 (15th June - 21st June)
##### Mid term preparation 
I will prepare for the mid term evaluation, by ensuring that the code written before this period is up to spec and working as intended.

### Week 5 (22nd June - 5th July)
##### Break 
I will take a break to go to school to apply for my internship. This internship will start in October, so it will not affect my Google Summer of Code work in any way.

### Weeks 6 and 7 (6th July - 19th July)
##### Import and Export
I shall dedicate these two weeks to work on the import/export feature. Kopete should be able to import logs from other instant messaging apps. On adding of a new account, it should check the config paths, and detect if any of the other apps such as Telepathy have logs for that particular account. We can then prompt the user on whether or not to import these logs.
I shall also work on the export feature, where the user can export the logs as text files, or even stand alone html documents that can be viewed on the browser.

### Week 8 (20th July - 26th July)
##### Backup / Restore
I shall work on a feature to enable the user back up the chat history, and restore it later. This backup will be saved in a compressed file, and It can be used when migrating to a new computer.

### Week 9 (27 July - 2nd August)
##### Testing and UI Polishing
This week shall be dedicated to thorough testing, UI polishing and other improvements. I shall also focus on speed improvements, in the searching of logs, importing and exporting.

### Rest of the Program (3rd August - 17th August)
##### Bug fixes
I will focus on bug fixes based on users' and developers' feedback and documentation writing. I have also dedicated a lot of time for this last segment, so that it can cater for any unforeseen delays in the preceding segments.

I will also start preparing for the code submission to Google.


