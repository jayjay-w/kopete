#ifndef HISTORYPLUGIN_H
#define HISTORYPLUGIN_H

#include <QVariantList>
#include "kopeteplugin.h"
#include "chathistoryhandler.h"


class HistoryPlugin : public Kopete::Plugin
{
	Q_OBJECT
public:
    HistoryPlugin(QObject *parent, const QVariantList &);
    ~HistoryPlugin();

public slots:
	void handleKopeteMessage(Kopete::Message &msg);

private:
	ChatHistoryHandler *chatHandler;
};

#endif // HISTORYPLUGIN_H
