/***************************************************************************
                          highlightpreferences.cpp  -  description
                             -------------------
    begin                : mar 14 2003
    copyright            : (C) 2003 by Olivier Goffart
    email                : ogoffart@tiscalinet.be
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include <kcombobox.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kparts/componentfactory.h>
#include <klocale.h>
#include <klistview.h>
#include <kcolorcombo.h>
#include <klineeditdlg.h>
#include <kurlrequester.h>
#include <kregexpeditorinterface.h>


#include "filter.h"
#include "highlightplugin.h"
#include "highlightprefsbase.h"
#include "highlightpreferences.h"




HighlightPreferences::HighlightPreferences(const QString &pixmap,QObject *parent)
							: ConfigModule(i18n("Highlight"),i18n("Highlight Plugin"),pixmap,parent)
{
	donttouch=true;
	( new QVBoxLayout( this ) )->setAutoAdd( true );
	preferencesDialog = new HighlightPrefsUI(this);

	connect(preferencesDialog->m_list , SIGNAL(selectionChanged()) , this , SLOT(slotCurrentFilterChanged()));
	connect(preferencesDialog->m_add , SIGNAL(pressed()) , this , SLOT(slotAddFilter()));
	connect(preferencesDialog->m_remove , SIGNAL(pressed()) , this , SLOT(slotRemoveFilter()));
	connect(preferencesDialog->m_rename , SIGNAL(pressed()) , this , SLOT(slotRenameFilter()));
	connect(preferencesDialog->m_editregexp , SIGNAL(pressed()) , this , SLOT(slotEditRegExp()));

	//Maybe here i should use a slot per widget, but i am too lazy
	connect(preferencesDialog->m_case , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_regexp , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_setImportance , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_setBG , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_setFG , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_search , SIGNAL(textChanged(const QString&)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_sound , SIGNAL(stateChanged(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_search , SIGNAL(textChanged(const QString&)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_importance , SIGNAL(activated(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_FG , SIGNAL(activated(int)) , this , SLOT(slotSomethingHasChanged()));
	connect(preferencesDialog->m_BG , SIGNAL(activated(int)) , this , SLOT(slotSomethingHasChanged()));

	reopen();
	donttouch=false;
}

HighlightPreferences::~HighlightPreferences()
{
}


void HighlightPreferences::reopen()
{
	donttouch=true;
	preferencesDialog->m_list->clear();
	m_filterItems.clear();

	QPtrList<Filter>  filters = HighlightPlugin::plugin()->filters();
	bool first=true;
	for(Filter *f=filters.first() ; f; f=filters.next() )
	{
		QListViewItem* lvi= new QListViewItem(preferencesDialog->m_list);
		lvi->setText(0,f->displayName );
		m_filterItems.insert(lvi,f);
		if(first)
			preferencesDialog->m_list->setSelected(lvi, true);
		first=false;
	}
	donttouch=false;
}

void HighlightPreferences::save()
{
	HighlightPlugin::plugin()->save();
}


void HighlightPreferences::slotCurrentFilterChanged()
{
	donttouch=true;
	Filter *current;
	if(!preferencesDialog->m_list->selectedItem() || !(current=m_filterItems[preferencesDialog->m_list->selectedItem()]))
	{
		preferencesDialog->m_search->setEnabled(false);
		preferencesDialog->m_case->setEnabled(false);
		preferencesDialog->m_regexp->setEnabled(false);
		preferencesDialog->m_importance->setEnabled(false);
		preferencesDialog->m_setImportance->setEnabled(false);
		preferencesDialog->m_BG->setEnabled(false);
		preferencesDialog->m_setBG->setEnabled(false);
		preferencesDialog->m_FG->setEnabled(false);
		preferencesDialog->m_setFG->setEnabled(false);
		preferencesDialog->m_soundFN->setEnabled(false);
		preferencesDialog->m_sound->setEnabled(false);
		preferencesDialog->m_editregexp->setEnabled(false);
		donttouch=false;
		return;
	}
	preferencesDialog->m_search->setEnabled(true);
	preferencesDialog->m_case->setEnabled(true);
	preferencesDialog->m_regexp->setEnabled(true);
	preferencesDialog->m_setImportance->setEnabled(true);
	preferencesDialog->m_setBG->setEnabled(true);
	preferencesDialog->m_setFG->setEnabled(true);
	preferencesDialog->m_sound->setEnabled(true);

	
	preferencesDialog->m_search->setText(current->search);
	preferencesDialog->m_case->setChecked(current->caseSensitive);
	preferencesDialog->m_regexp->setChecked(current->isRegExp);
	preferencesDialog->m_editregexp->setEnabled(current->isRegExp);
	preferencesDialog->m_importance->setCurrentItem(current->importance);
	preferencesDialog->m_setImportance->setChecked(current->setImportance);
	preferencesDialog->m_importance->setEnabled(current->setImportance);
	preferencesDialog->m_BG->setColor(current->BG);
	preferencesDialog->m_setBG->setChecked(current->setBG);
	preferencesDialog->m_BG->setEnabled(current->setBG);
	preferencesDialog->m_FG->setColor(current->FG);
	preferencesDialog->m_setFG->setChecked(current->setFG);
	preferencesDialog->m_FG->setEnabled(current->setFG);
	preferencesDialog->m_soundFN->setURL(current->soundFN);
	preferencesDialog->m_sound->setChecked(current->playSound);
	preferencesDialog->m_soundFN->setEnabled(current->playSound);
	
	
	donttouch=false;
}

void HighlightPreferences::slotAddFilter()
{
	Filter *filtre=HighlightPlugin::plugin()->newFilter();
	QListViewItem* lvi= new QListViewItem(preferencesDialog->m_list);
	lvi->setText(0,filtre->displayName );
	m_filterItems.insert(lvi,filtre);
	preferencesDialog->m_list->setSelected(lvi, true);
}

void HighlightPreferences::slotRemoveFilter()
{
	QListViewItem *lvi=preferencesDialog->m_list->selectedItem();
	if(!lvi)
		return;
	Filter *current=m_filterItems[lvi];
	if(!current)
		return;
	
	m_filterItems.remove(lvi);
	delete lvi;
	HighlightPlugin::plugin()->removeFilter(current);
}

void HighlightPreferences::slotRenameFilter()
{
	QListViewItem *lvi=preferencesDialog->m_list->selectedItem();
	if(!lvi)
		return;
	Filter *current=m_filterItems[lvi];
	if(!current)
		return;
		
	bool ok;
	QString newname = KLineEditDlg::getText(
		i18n( "Rename Filter" ), i18n( "Please enter the new name for the Filter" ), current->displayName, &ok );
	if( !ok )
		return;
	current->displayName=newname;
	lvi->setText(0,newname);
}


void HighlightPreferences::slotSomethingHasChanged()
{
	Filter *current;
	if(donttouch || !preferencesDialog->m_list->selectedItem() || !(current=m_filterItems[preferencesDialog->m_list->selectedItem()]))
		return;

	current->search=preferencesDialog->m_search->text();
	current->caseSensitive=preferencesDialog->m_case->isChecked();
	current->isRegExp=preferencesDialog->m_regexp->isChecked();
	preferencesDialog->m_editregexp->setEnabled(current->isRegExp);
	current->importance=preferencesDialog->m_importance->currentItem();
	current->setImportance=preferencesDialog->m_setImportance->isChecked();
	preferencesDialog->m_importance->setEnabled(current->setImportance);
	current->BG=preferencesDialog->m_BG->color();
	current->setBG=preferencesDialog->m_setBG->isChecked();
	preferencesDialog->m_BG->setEnabled(current->setBG);
	current->FG=preferencesDialog->m_FG->color();
	current->setFG=preferencesDialog->m_setFG->isChecked();
	preferencesDialog->m_FG->setEnabled(current->setFG);
//	current->soundFN=preferencesDialog->m_soundFN->URL();
	current->playSound=preferencesDialog->m_sound->isChecked();
	preferencesDialog->m_soundFN->setEnabled(current->playSound);
}

void HighlightPreferences::slotEditRegExp()
{
	QDialog *editorDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );
	if ( editorDialog )
	{
		// kdeutils was installed, so the dialog was found fetch the editor interface
		KRegExpEditorInterface *editor = static_cast<KRegExpEditorInterface *>( editorDialog->qt_cast( "KRegExpEditorInterface" ) );
		Q_ASSERT( editor ); // This should not fail!
		// now use the editor.
		editor->setRegExp(preferencesDialog->m_search->text());

		// Finally exec the dialog
		if(editorDialog->exec() == QDialog::Accepted )
		{
			preferencesDialog->m_search->setText(editor->regExp());
		}
		
	}
	else
	{
		// Don't offer the dialog.
	}

}

#include "highlightpreferences.moc"
