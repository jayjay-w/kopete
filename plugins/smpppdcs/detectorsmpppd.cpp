/*
    detectorsmpppd.cpp
 
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

#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>

#include "iconnector.h"
#include "detectorsmpppd.h"

#include "smpppdclient.h"

DetectorSMPPPD::DetectorSMPPPD(IConnector * connector)
	: DetectorDCOP(connector) {}

DetectorSMPPPD::~DetectorSMPPPD() {}

/*!
    \fn DetectorSMPPPD::checkStatus()
 */
void DetectorSMPPPD::checkStatus() {
    kDebug(14312) << k_funcinfo << "Checking for online status..." << endl;

	m_kinternetApp = getKInternetDCOP();
	if(m_client && m_kinternetApp != "") {
		switch(getConnectionStatusDCOP()) {
			case CONNECTED:
				m_connector->setConnectedStatus(true);
				return;
			case DISCONNECTED:
				m_connector->setConnectedStatus(false);
				return;
			default:
				break;
		}
	}
	
	SMPPPD::Client c;
	
	static KConfig *config = KGlobal::config();
	config->setGroup(SMPPPDCS_CONFIG_GROUP);
	unsigned int port = config->readUnsignedNumEntry("port", 3185);
	QString    server = config->readEntry("server", "localhost").utf8();
	
	c.setPassword(config->readEntry("Password", "").utf8());
	
	if(c.connect(server, port)) {
		m_connector->setConnectedStatus(c.isOnline());
	} else {
		kDebug(14312) << k_funcinfo << "not connected to smpppd => I'll try again later" << endl;
		m_connector->setConnectedStatus(false);
	}
}

void DetectorSMPPPD::smpppdServerChange() {
    kDebug(14312) << k_funcinfo << "Server changed. Disconnect to SMPPPD" << endl;
}
