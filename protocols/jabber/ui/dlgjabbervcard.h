/***************************************************************************
                          dlgjabbervcard.h  -  description
                             -------------------
    begin                : Thu Aug 08 2002
    copyright            : (C) 2002 by Till Gerken
    email                : till@tantalo.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DLGJABBERVCARD_H
#define DLGJABBERVCARD_H

#include "dlgvcard.h"
#include "jabcommon.h"
#include "jabtasks.h"

class dlgJabberVCard : public dlgVCard
{ 
	Q_OBJECT

	public:
		dlgJabberVCard( QWidget* parent = 0, const char* name = 0, JT_VCard *vCard = 0 );
		~dlgJabberVCard();
    
		void assignVCard(JT_VCard *vCard);
    
	public slots:
		void slotClose();
		void slotSaveNickname();

	signals:
		void updateNickname(const QString);		
};

#endif // DLGJABBERVCARD_H
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

