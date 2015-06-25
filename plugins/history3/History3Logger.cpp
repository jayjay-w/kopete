#include "History3Logger.h"

//Qt includes
#include <QSqlQuery>
#include <QDateTime>

//KDE includes
#include <kstandarddirs.h>

//Kopete includes
#include "kopetemessage.h"
#include "kopetecontact.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"

History3Logger* History3Logger::m_instance = 0;


History3Logger::History3Logger()
{

}


History3Logger::~History3Logger()
{
	history_database.close();
}

void History3Logger::appendMessage(const Kopete::Message &msg, const Kopete::Contact *ct)
{

}

bool History3Logger::messageExists(const Kopete::Message &msg, const Kopete::Contact *c)
{

	return false;
}
