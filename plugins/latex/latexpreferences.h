/*
    Kopete Latex Plugin

    Copyright (c) 2004 by Duncan Mac-Vicar Prett   <duncan@kde.org>

    Kopete    (c) 2001-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef LatexPREFERENCES_H
#define LatexPREFERENCES_H

#define KDE3_SUPPORT
#include <kcmodule.h>
#undef KDE3_SUPPORT
#include <qstring.h>

namespace Ui { class LatexPrefsUI; }

/**
  *@author Duncan Mac-Vicar Prett
  */

class LatexPreferences : public KCModule
{
	Q_OBJECT
public:

	explicit LatexPreferences(QWidget *parent = 0, const QStringList &args = QStringList());
	~LatexPreferences();

	virtual void save();
	virtual void load();

private:
	Ui::LatexPrefsUI *m_preferencesDialog;
private slots:
	void slotModified();
};

#endif
