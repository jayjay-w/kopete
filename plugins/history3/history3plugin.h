#ifndef HISTORY3PLUGIN_H
#define HISTORY3PLUGIN_H

#include <QStringList>
#include "kopeteplugin.h"
#include "databasehelper.h"


class History3Plugin : public Kopete::Plugin
{
	Q_OBJECT
public:
	History3Plugin(QObject *parent, const QStringList &);
	~History3Plugin();

public slots:
	void handleKopeteMessage(Kopete::Message &msg);

private:
	DatabaseHelper *dbHelper;
};

#endif // HISTORY3PLUGIN_H
