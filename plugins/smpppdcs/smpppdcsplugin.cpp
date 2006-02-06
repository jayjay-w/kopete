/*
    smpppdcsplugin.cpp
 
    Copyright (c) 2002-2003 by Chris Howells         <howells@kde.org>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004-2006 by Heiko Schaefer        <heiko@rangun.de>
 
    Kopete    (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; version 2 of the License.               *
    *                                                                       *
    *************************************************************************
*/

#include "onlineinquiry.h"
#include "smpppdcsplugin.h"

#include <qtimer.h>
//Added by qt3to4:
#include <QByteArray>

#include <kdebug.h>
#include <kconfig.h>
#include <kgenericfactory.h>

#include "kopeteprotocol.h"
#include "kopetepluginmanager.h"
#include "kopeteaccountmanager.h"

#include "detectornetstat.h"
#include "detectorsmpppd.h"

typedef KGenericFactory<SMPPPDCSPlugin> SMPPPDCSPluginFactory;
K_EXPORT_COMPONENT_FACTORY(kopete_smpppdcs, SMPPPDCSPluginFactory("kopete_smpppdcs"))

SMPPPDCSPlugin::SMPPPDCSPlugin(QObject *parent, const char * name, const QStringList& /* args */)
        : DCOPObject("SMPPPDCSIface"), Kopete::Plugin(SMPPPDCSPluginFactory::instance(), parent, name),
        m_detectorSMPPPD(NULL), m_detectorNetstat(NULL), m_timer(NULL),
m_onlineInquiry(NULL) {

    m_pluginConnected = false;

    // we wait for the allPluginsLoaded signal, to connect as early as possible after startup
    connect(Kopete::PluginManager::self(), SIGNAL(allPluginsLoaded()),
            this, SLOT(allPluginsLoaded()));

    m_onlineInquiry   = new OnlineInquiry();
    m_detectorSMPPPD  = new DetectorSMPPPD(this);
    m_detectorNetstat = new DetectorNetstat(this);
}

SMPPPDCSPlugin::~SMPPPDCSPlugin() {

    kDebug(14312) << k_funcinfo << endl;

    delete m_timer;
    delete m_detectorSMPPPD;
    delete m_detectorNetstat;
    delete m_onlineInquiry;
}

void SMPPPDCSPlugin::allPluginsLoaded() {

    m_timer = new QTimer();
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( slotCheckStatus() ) );

    if(useSmpppd()) {
        m_timer->start(30000);
    } else {
        // we use 1 min interval, because it reflects the old connectionstatus plugin behaviour
        m_timer->start(60000);
    }

    slotCheckStatus();
}

bool SMPPPDCSPlugin::isOnline() {
    return m_onlineInquiry->isOnline(useSmpppd());
}

void SMPPPDCSPlugin::slotCheckStatus() {
    if(useSmpppd()) {
        m_detectorSMPPPD->checkStatus();
    } else {
        m_detectorNetstat->checkStatus();
    }
}

void SMPPPDCSPlugin::setConnectedStatus( bool connected ) {
    kDebug(14312) << k_funcinfo << connected << endl;

    // We have to handle a few cases here. First is the machine is connected, and the plugin thinks
    // we're connected. Then we don't do anything. Next, we can have machine connected, but plugin thinks
    // we're disconnected. Also, machine disconnected, plugin disconnected -- we
    // don't do anything. Finally, we can have the machine disconnected, and the plugin thinks we're
    // connected. This mechanism is required so that we don't keep calling the connect/disconnect functions
    // constantly.

    if ( connected && !m_pluginConnected ) {
        // The machine is connected and plugin thinks we're disconnected
        kDebug(14312) << k_funcinfo << "Setting m_pluginConnected to true" << endl;
        m_pluginConnected = true;
        connectAllowed();
        kDebug(14312) << k_funcinfo << "We're connected" << endl;
    } else if ( !connected && m_pluginConnected ) {
        // The machine isn't connected and plugin thinks we're connected
        kDebug(14312) << k_funcinfo << "Setting m_pluginConnected to false" << endl;
        m_pluginConnected = false;
        disconnectAllowed();
        kDebug(14312) << k_funcinfo << "We're offline" << endl;
    }
}

void SMPPPDCSPlugin::connectAllowed()
{
	static KConfig *config = KGlobal::config();
	config->setGroup(SMPPPDCS_CONFIG_GROUP);
	QStringList list = config->readListEntry("ignoredAccounts");
	
	Kopete::AccountManager * m = Kopete::AccountManager::self();
	foreach(Kopete::Account *account, m->accounts())
	{
		if(!list.contains(account->protocol()->pluginId() + "_" + account->accountId())) {
			account->connect();
		}
	}
}

void SMPPPDCSPlugin::disconnectAllowed() {
    static KConfig *config = KGlobal::config();
    config->setGroup(SMPPPDCS_CONFIG_GROUP);
	QStringList list = config->readListEntry("ignoredAccounts");
	
	Kopete::AccountManager * m = Kopete::AccountManager::self();
	foreach(Kopete::Account *account, m->accounts())
	{
		if(!list.contains(account->protocol()->pluginId() + "_" + account->accountId())) {
			account->disconnect();
		}
	}
}

/*!
    \fn SMPPPDCSPlugin::useSmpppd() const
 */
bool SMPPPDCSPlugin::useSmpppd() const {
    static KConfig *config = KGlobal::config();
    config->setGroup(SMPPPDCS_CONFIG_GROUP);
    return config->readBoolEntry("useSmpppd", false);
}

QString SMPPPDCSPlugin::detectionMethod() const {
    if(useSmpppd()) {
        return "smpppd";
    } else {
        return "netstat";
    }
}

/*!
    \fn SMPPPDCSPlugin::smpppdServerChanged(const QString& server)
 */
void SMPPPDCSPlugin::smpppdServerChanged(const QString& server)
{
	static KConfig *config = KGlobal::config();
	config->setGroup(SMPPPDCS_CONFIG_GROUP);
	QString oldServer = config->readEntry("server", "localhost").utf8();
	
	if(oldServer != server) {
		kDebug(14312) << k_funcinfo << "Detected a server change" << endl;
		m_detectorSMPPPD->smpppdServerChange();
	}
}

#include "smpppdcsplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:
