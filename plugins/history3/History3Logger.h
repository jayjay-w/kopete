#ifndef HISTORRY3LOGGER_H
#define HISTORY3LOGGER_H

#include <QObject>
#include <QSqlDatabase>


namespace Kopete {
class Message;
class Contact;
class Account;
class MetaContact;
}


class History3Logger : public QObject {
    Q_OBJECT
public:

private:
    History3Logger();
    ~History3Logger();

    static History3Logger* instance;
    QSqlDatabase history_database;
};

#endif
