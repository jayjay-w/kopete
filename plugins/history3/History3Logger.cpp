#include "History3Logger.h"

//Qt includes
#include <QSqlQuery>

//KDE includes
#include <kstandarddirs.h>

//Kopete includes
#include "kopetemessage.h"
#include "kopetecontact.h"

History3Logger::History3Logger()
{
    //Initial database setup
    //The default db will be SQLITE, then we can add MySQL and others later
    QString dbPath = KStandardDirs::locateLocal("appdata", "kopete_history3.db");
    history_database = QSqlDatabase::addDatabase("QSQLITE", "kopete-history3");
    history_database.setDatabaseName(dbPath);
    if (!history_database.open()) {
        return;
    }

    //Check for history table, and create it if it does not exist.
    QSqlQuery table_query("SELECT name FROM sqlite_master WHERE type='table'", history_database);
    table_query.exec();

    QStringList tables;
    while (table_query.next()) {
        tables.append(table_query.value(0).toString());
    }

    if (!tables.contains("history")) {
        history_database.exec(QString(
                                  "CREATE TABLE history"
                                  "(entry_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
                                  "protocol TEXT, "
                                  "account TEXT, "
                                  "local_id TEXT, "
                                  "local_nick TEXT, "
                                  "remote_id TEXT, "
                                  "remote_nick TEXT, "
                                  "date_time TEXT, "
                                  "message TEXT"
                                  ")"
                                  ));
    }
}


History3Logger::~History3Logger()
{
    history_database.close();
}
