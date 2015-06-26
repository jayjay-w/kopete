#ifndef HISTORY3PLUGIN_H
#define HISTORY3PLUGIN_H

#include <QVariantList>
#include "kopeteplugin.h"
#include "databaselogger.h"


class History3Plugin : public Kopete::Plugin
{
	Q_OBJECT
public:
	History3Plugin(QObject *parent, const QVariantList &);
	~History3Plugin();

public slots:
	void handleKopeteMessage(Kopete::Message &msg);

private:
	DatabaseLogger *dbHelper;
};

#endif // HISTORY3PLUGIN_H
