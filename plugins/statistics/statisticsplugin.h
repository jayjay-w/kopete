/*
    statisticsplugin.h

    Copyright (c) 2003-2004 by Marc Cramdal        <marc.cramdal@yahoo.fr>


    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef STATISTICSPLUGIN_H
#define STATISTICSPLUGIN_H

#include <map>
using namespace std;

#include <qobject.h>
#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>

#include "kopeteplugin.h"

#include "kopetemessage.h"
#include "kopetemessagehandler.h"
#include "kopeteonlinestatus.h"

class QString;

class StatisticsDB;
class StatisticsContact;

class KopeteView;
class KActionCollection;

/** \section Kopete Statistics Plugin
 *
 * \subsection intro_sec Introduction
 *
 * This plugin aims at giving detailed statistics on metacontacts, for instance, how long was
 * the metacontact online, how long was it busy etc. 
 * In the future, it will maybe make prediction on when the contact should be available for chat.
 *
 * \subsection install_sec How it works ...
 * Each Metacontact is bound to a StatisticsContact which has access to the SQLITE database.
 * This StatisticsContact stores the last status of the metacontact; the member function onlineStatusChanged is called when the 
 * metacontact status changed (this is managed in the slot slotOnlineStatusChanged of StatisticsPlugin) and then the DB is 
 * updated for the contact.
 * 
 * More exactly the DB is updated only if the oldstatus was not Offline

 * To have an idea how it works, here is a table :
 * 
 * <table>
  * <tr>
 * <td>Event</td><td>Changes to database</td><td>oldStatus</td>
 * </tr>
  * <tr>
 * <td>John 17:44 Away <i>(connexion)</i></td><td> - <i>(oldstatus was offline)</i></td><td>oldstatus = away </td>
 * </tr>
 * <tr>
 * <td>John 18:01 Online</td><td>(+) Away 17:44 18:01</td><td>oldstatus = online</td>
 * </tr>
 * <tr>
 * <td>John 18:30 Offline <i>(disconnect)</i></td><td>(+) Online 18:01 18:30</td><td>oldstatus = offline</td>
 * </tr>
 * <tr>
 * <td>John 18:45 Online <i>(connexion)</i></td><td> - <i>(oldstatus was offline)</i></td><td>oldstatus = online</td>
 * </tr>
 * <tr>
 * <td>John 20:30 Offline <i>(disconnect)</i></td><td>(+) Online 18:45 20:30</td><td>oldstatus = offline</td>
 * </tr>
 * </table>
 *  
 * etc.
 * 
 * \subsection install_sec Some little stats
 * This plugin is able to record some other stats, not based on events. Theyre saved in the commonstat table in which we store stats
 * like this :
 * 
 * <code>statname, statvalue1, statvalue2</code>
 * 
 * Generally, we store the value, and its ponderation. If an average on one hundred messages says that the contact X takes about 
 * 3 seconds between two messages, we store "timebetweentwomessages", "3", "100"
 *
 *
 *
 * StatisticsPlugin is the main Statistics plugin class.
 * Contains mainly slots.
 */
class StatisticsPlugin : public Kopete::Plugin
{
	Q_OBJECT
public:
	/// Standard plugin constructors
	StatisticsPlugin(QObject *parent, const char *name, const QStringList &args);
	~StatisticsPlugin();
	
	/// Method to access m_db member
	StatisticsDB *db() { return m_db; }
		
public slots:
	
	/** \brief This slot is called when the status of a contact changed.
	 *
	 *   Then it searches for the contact bind to the metacontact who triggered the signal and calls 
	 *   the specific StatisticsContact::onlineStatusChanged of the StatisticsContact to update the StatisticsContact status,
	 *   and maybe, update the database.
 	 */
	void slotOnlineStatusChanged(Kopete::MetaContact *contact, Kopete::OnlineStatus::StatusType status );

	/** 
	 * Builds and show the StatisticsDialog for a contact
	 */	   
	void slotViewStatistics();
		
	/**
	 *  
	 * Extract the metaContactId from the message, and calls the 
	 * StatisticsContact::newMessageReceived(Kopete::Message& m) function
	 * for the corresponding contact
	 */
	void slotAboutToReceive(Kopete::Message& m);
	
	/*
	 * Managing views
	 */
	
	/**
	 * \brief Only connects the Kopete::ChatSession::closing() signal to our slotViewClosed().
	 */
	void slotViewCreated(Kopete::ChatSession* session);
			  
	/**
	 * One aim of this slot is to be able to stop recording time between two messages 
	 * for the contact in the chatsession. But, we only 
	 * want to do this if the contact is not in an other chatsession.
	 */
	void slotViewClosed(Kopete::ChatSession* session);

	/**
	 * Slot called when a new metacontact is added to make some slots connections and to create a new
	 * StatisticsContact object.
     * 
	 * In the constructor, we connect the metacontacts already existing to some slots, but we need to do this
	 * when new metacontacts are added.
	 * This function is also called when we loop over the contact list in the constructor.
 	*/
	void slotMetaContactAdded(Kopete::MetaContact *mc);
private:	
	StatisticsDB *m_db;
	/** Associate a StatisticsContact to a Kopete::MetaContact id to retrieve
	* the StatisticsContact corresponding to the MetaContact in the slots
	*/
	map<QString, StatisticsContact*> statisticsContactMap; 
	
};


#endif
