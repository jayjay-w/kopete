#ifndef HISTORY3PLUGIN_H
#define HISTORY3PLUGIN_H

#include <QVariantList>
#include "kopeteplugin.h"
#include "chathistoryhandler.h"


class History3Plugin : public Kopete::Plugin
{
	Q_OBJECT
public:
	History3Plugin(QObject *parent, const QVariantList &);
	~History3Plugin();

public slots:
	void handleKopeteMessage(Kopete::Message &msg);

private:
	ChatHistoryHandler *chatHandler;
};

#endif // HISTORY3PLUGIN_H
