#ifndef SMSSERVICE_H
#define SMSSERVICE_H

#include "kopetemessage.h"

#include <qstring.h>
#include <qwidget.h>
#include <qobject.h>

class SMSContact;
class KopeteAccount;

class SMSService : public QObject
{
public:
	SMSService(KopeteAccount* account = 0);
	virtual ~SMSService();

	virtual void setAccount(KopeteAccount* account);

	virtual void send(const KopeteMessage& msg) = 0;
	virtual QWidget* configureWidget(QWidget* parent) = 0;

	virtual int maxSize() = 0;
	virtual const QString& description() = 0;
public slots:
	virtual void savePreferences() = 0;
signals:
	void messageSent(const KopeteMessage&);
protected:
	KopeteAccount* m_account;
} ;

#endif //SMSSERVICE_H
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

