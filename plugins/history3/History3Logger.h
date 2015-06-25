#ifndef HISTORRY3LOGGER_H
#define HISTORY3LOGGER_H

#include <QObject>
#include <QSqlDatabase>

class QDate;

namespace Kopete {
class Message;
class Contact;
class Account;
class MetaContact;
}


class History3Logger : public QObject {
	Q_OBJECT
public:
	static History3Logger* instance() {
		if (!m_instance) {
			m_instance = new History3Logger();
		}

		return m_instance;
	}

	static void drop() {
		delete m_instance;
		m_instance = 0;
	}

	void appendMessage(const Kopete::Message &msg, const Kopete::Contact *c=0L);
	bool messageExists(const Kopete::Message &msg, const Kopete::Contact *ct=0L);

private:
	History3Logger();
	~History3Logger();

	static History3Logger* m_instance;
	QSqlDatabase history_database;
};

#endif
