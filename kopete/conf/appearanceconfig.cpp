/*
    appearanceconfig.cpp  -  Kopete Look Feel Config

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "appearanceconfig.h"

#include <qcheckbox.h>
#include <qdir.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qhbuttongroup.h>
#include <qstringlist.h>
#include <qtextedit.h>
#include <qvgroupbox.h>
#include <qspinbox.h>
#include <qslider.h>

#include <klineedit.h>
#include <klistbox.h>
#include <kcolorcombo.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotifydialog.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <ktabctl.h>
#include <kglobalsettings.h>
#include <kpushbutton.h>
#include <kfontdialog.h>
#include <ktrader.h>
#include <klibloader.h>

#include <ktexteditor/highlightinginterface.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include "appearanceconfig_general.h"
#include "appearanceconfig_contactlist.h"
#include "appearanceconfig_chatwindow.h"
#include "appearanceconfig_chatappearance.h"
#include "configmodule.h"
#include "kopetechatwindow.h"
#include "kopeteprefs.h"
#include "kopetemessage.h"
#include "kopetecontact.h"
#include "kopeteaway.h"
#include "kopeteawayconfigui.h"
#include "styleeditdialog.h"


#include <qtabwidget.h>

AppearanceConfig::AppearanceConfig(QWidget * parent) :
	ConfigModule (
		i18n("Behaviour"),
		i18n("Here You Can Personalize Kopete"),
		"appearance",
		parent )
{
	(new QVBoxLayout(this))->setAutoAdd(true);
	QTabWidget *mAppearanceTabCtl = new QTabWidget(this,"mAppearanceTabCtl");

	editedItem = 0L;

	// "General" TAB =============================================================
	mPrfsGeneral = new AppearanceConfig_General(mAppearanceTabCtl);
	connect(mPrfsGeneral->configSound, SIGNAL(clicked()), this, SLOT(slotConfigSound()));
	connect(mPrfsGeneral->mShowTrayChk, SIGNAL(clicked()), this, SLOT(slotShowTrayChanged()));
	mAppearanceTabCtl->addTab( mPrfsGeneral, i18n("&General") );

	// "Away" TAB ========================================================
	mAwayConfigUI = new KopeteAwayConfigUI(mAppearanceTabCtl);
	mAppearanceTabCtl->addTab( mAwayConfigUI, i18n("&Away Settings") );

	// "Contact List" TAB ========================================================
	mPrfsContactlist = new AppearanceConfig_Contactlist(mAppearanceTabCtl);
	mAppearanceTabCtl->addTab( mPrfsContactlist, i18n("Contact &List") );

	// "Emoticons" TAB ===========================================================
	mEmoticonsTab = new QFrame(mAppearanceTabCtl);
	(new QVBoxLayout(mEmoticonsTab, KDialog::marginHint(), KDialog::spacingHint()))->setAutoAdd(true);
	mUseEmoticonsChk = new QCheckBox ( i18n("&Use emoticons"), mEmoticonsTab );
	icon_theme_list = new KListBox ( mEmoticonsTab, "icon_theme_list" );
	connect(mUseEmoticonsChk, SIGNAL(toggled(bool)), this, SLOT(slotUseEmoticonsChanged( bool )));
	mAppearanceTabCtl->addTab( mEmoticonsTab, i18n("&Emoticons") );

	// "Chat Window" TAB =========================================================
	mPrfsChatWindow = new AppearanceConfig_ChatWindow(mAppearanceTabCtl);
	mAppearanceTabCtl->addTab( mPrfsChatWindow, i18n("&Interface") );
	connect(mPrfsChatWindow->mTransparencyEnabled, SIGNAL(toggled(bool)), this, SLOT(slotTransparencyChanged(bool)));

	// "Chat Appearance" TAB =====================================================
	mPrfsChatAppearance = new AppearanceConfig_ChatAppearance(mAppearanceTabCtl);
	mPrfsChatAppearance->htmlFrame->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
	QVBoxLayout *l = new QVBoxLayout( mPrfsChatAppearance->htmlFrame );

	preview = new KHTMLPart( mPrfsChatAppearance->htmlFrame, "preview" );
	preview->setJScriptEnabled( false ) ;
	preview->setJavaEnabled( false );
	preview->setPluginsEnabled( false );
	preview->setMetaRefreshEnabled( false );

	KHTMLView *htmlWidget = preview->view();
	htmlWidget->setMarginWidth(4);
	htmlWidget->setMarginHeight(4);
	htmlWidget->setFocusPolicy( NoFocus );
	htmlWidget->setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) );
	l->addWidget( htmlWidget );

	mAppearanceTabCtl->addTab( mPrfsChatAppearance, i18n("Chat &Appearance") );
	connect(mPrfsChatAppearance->highlightEnabled, SIGNAL(toggled(bool)), this, SLOT(slotHighlightChanged()));
	connect(mPrfsChatAppearance->foregroundColor, SIGNAL(changed(const QColor &)), this, SLOT(slotHighlightChanged()));
	connect(mPrfsChatAppearance->backgroundColor, SIGNAL(changed(const QColor &)), this, SLOT(slotHighlightChanged()));
	connect(mPrfsChatAppearance->fontFace, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
	connect(mPrfsChatAppearance->textColor, SIGNAL(changed(const QColor &)), this, SLOT(slotUpdatePreview()));
	connect(mPrfsChatAppearance->bgColor, SIGNAL(changed(const QColor &)), this, SLOT(slotUpdatePreview()));
	connect(mPrfsChatAppearance->linkColor, SIGNAL(changed(const QColor &)), this, SLOT(slotUpdatePreview()));
	connect(mPrfsChatAppearance->styleList, SIGNAL(selectionChanged( QListBoxItem *)), this, SLOT(slotUpdatePreview()));
	connect(mPrfsChatAppearance->addButton, SIGNAL(clicked()), this, SLOT(slotAddStyle()));
	connect(mPrfsChatAppearance->editButton, SIGNAL(clicked()), this, SLOT(slotEditStyle()));
	connect(mPrfsChatAppearance->deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteStyle()));
	// ===========================================================================

	reopen(); // load settings from config
	slotTransparencyChanged(mPrfsChatWindow->mTransparencyEnabled->isChecked());
	slotShowTrayChanged();

	// sync actions, config and prefs-dialog
	connect(KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()));
}

AppearanceConfig::~AppearanceConfig()
{
}

void AppearanceConfig::save()
{
//	kdDebug(14000) k_funcinfo << "called." << endl;
	KopetePrefs *p = KopetePrefs::prefs();

	// "General" TAB
	p->setShowTray( mPrfsGeneral->mShowTrayChk->isChecked() );
	p->setStartDocked ( mPrfsGeneral->mStartDockedChk->isChecked() );
	p->setUseQueue ( mPrfsGeneral->mUseQueueChk->isChecked() );
	p->setTrayflashNotify ( mPrfsGeneral->mTrayflashNotifyChk->isChecked() );
	p->setBalloonNotify ( mPrfsGeneral->mBalloonNotifyChk->isChecked() );
	p->setSoundIfAway( mPrfsGeneral->mSoundIfAwayChk->isChecked() );

	// "Contact List" TAB
	p->setTreeView ( mPrfsContactlist->mTreeContactList->isChecked() );
	p->setShowOffline ( mPrfsContactlist->mShowOfflineUsers->isChecked() );
	p->setSortByGroup ( mPrfsContactlist->mSortByGroup->isChecked() );
	p->setGreyIdleMetaContacts( mPrfsContactlist->mGreyIdleMetaContacts->isChecked() );

	// Another TAB
	p->setIconTheme( icon_theme_list->currentText() );
	p->setUseEmoticons ( mUseEmoticonsChk->isChecked() );

	// "Chat Window" TAB
	p->setRaiseMsgWindow( mPrfsChatWindow->cb_RaiseMsgWindowChk->isChecked() );
	p->setShowEvents( mPrfsChatWindow->cb_ShowEventsChk->isChecked() );
	p->setChatWindowPolicy ( mPrfsChatWindow->chatWindowGroup->id(mPrfsChatWindow->chatWindowGroup->selected()) );
	p->setInterfacePreference( mPrfsChatWindow->interfaceGroup->id(mPrfsChatWindow->interfaceGroup->selected()) );
	p->setTransparencyColor( mPrfsChatWindow->mTransparencyTintColor->color() );
	p->setTransparencyEnabled( mPrfsChatWindow->mTransparencyEnabled->isChecked() );
	p->setTransparencyValue( mPrfsChatWindow->mTransparencyValue->value() );
	p->setChatViewBufferSize ( mPrfsChatWindow->mChatViewBufferSize->value() );
// 	p->setCTransparencyColor( mPrfsChatWindow->mCTransparencyColor->color() );
// 	p->setCTransparencyEnabled( mPrfsChatWindow->mCTransparencyEnabled->isChecked() );
// 	p->setCTransparencyValue( mPrfsChatWindow->mCTransparencyValue->value() );
	p->setBgOverride( mPrfsChatWindow->mTransparencyBgOverride->isChecked() );
	p->setHighlightEnabled(mPrfsChatAppearance->highlightEnabled->isChecked());
	p->setHighlightBackground(mPrfsChatAppearance->backgroundColor->color());
	p->setHighlightForeground(mPrfsChatAppearance->foregroundColor->color());

	p->setChatStyles( mChatStyles );
	p->setBgColor( mPrfsChatAppearance->bgColor->color() );
	p->setTextColor(  mPrfsChatAppearance->textColor->color() );
	p->setLinkColor( mPrfsChatAppearance->linkColor->color() );
	p->setFontFace( mPrfsChatAppearance->fontFace->font() );
	p->setKindMessagesHtml( mChatStyles[ mPrfsChatAppearance->styleList->selectedItem()->text() ] );


	KopeteAway::getInstance()->setAutoAwayTimeout(mAwayConfigUI->mAwayTimeout->value()*60);
	KopeteAway::getInstance()->setGoAvailable(mAwayConfigUI->mGoAvailable->isChecked());
	/* Tells KopeteAway to save it's messages */
	KopeteAway::getInstance()->save();

	// disconnect or else we will end up in an endless loop
	disconnect(KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()));
	p->save();
	connect(KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()));
}

void AppearanceConfig::slotConfigChanged()
{
//	kdDebug(14000) << k_funcinfo << "calling reopen() now..." << endl;
	reopen(); // I am lazy :P
}

void AppearanceConfig::reopen()
{
//	kdDebug(14000) << k_funcinfo << "called" << endl;
	KopetePrefs *p = KopetePrefs::prefs();

	// "General" TAB
	mPrfsGeneral->mShowTrayChk->setChecked( p->showTray() );
	mPrfsGeneral->mStartDockedChk->setChecked( p->startDocked() );
	mPrfsGeneral->mUseQueueChk->setChecked( p->useQueue() );
	mPrfsGeneral->mTrayflashNotifyChk->setChecked ( p->trayflashNotify() );
	mPrfsGeneral->mBalloonNotifyChk->setChecked ( p->balloonNotify() );
	mPrfsGeneral->mSoundIfAwayChk->setChecked( p->soundIfAway( )  ) ;

	// "Contact List" TAB
	mPrfsContactlist->mTreeContactList->setChecked( p->treeView() );
	mPrfsContactlist->mSortByGroup->setChecked( p->sortByGroup() );
	mPrfsContactlist->mShowOfflineUsers->setChecked( p->showOffline() );
	mPrfsContactlist->mGreyIdleMetaContacts->setChecked( p->greyIdleMetaContacts() );

	// "Emoticons" TAB
	KStandardDirs dir;
	icon_theme_list->clear(); // Wipe out old list
	// Get a list of directories in our icon theme dir
	QStringList themeDirs = KGlobal::dirs()->findDirs("data", "kopete/pics/emoticons");
	// loop adding themes from all dirs into theme-list
	for(unsigned int x = 0;x < themeDirs.count();x++)
	{
		QDir themeQDir(themeDirs[x]);
		themeQDir.setFilter( QDir::Dirs ); // only scan for subdirs
		themeQDir.setSorting( QDir::Name ); // I guess name is as good as any
		for(unsigned int y = 0; y < themeQDir.count(); y++)
		{
			QStringList themes = themeQDir.entryList(QDir::Dirs, QDir::Name);
			// We don't care for '.' and '..'
			if ( themeQDir[y] != "." && themeQDir[y] != ".." )
			{
				// Add ourselves to the list, using our directory name
				QPixmap previewPixmap = QPixmap( locate("data","kopete/pics/emoticons/"+themeQDir[y]+"/smile.png") );
				icon_theme_list->insertItem ( previewPixmap,themeQDir[y] );
			}
		}
	}
	// Where is that theme in our big-list-o-themes?
	QListBoxItem *item = icon_theme_list->findItem( p->iconTheme() );

	if (item) // found it... make it the currently selected theme
		icon_theme_list->setCurrentItem( item );
	else // Er, it's not there... select the current item
		icon_theme_list->setCurrentItem( 0 );

	mUseEmoticonsChk->setChecked( p->useEmoticons() );
	icon_theme_list->setEnabled ( p->useEmoticons() );

	// "Chat Window" TAB
	mPrfsChatWindow->cb_RaiseMsgWindowChk->setChecked( p->raiseMsgWindow() );
	mPrfsChatWindow->cb_ShowEventsChk->setChecked( p->showEvents() );
	mPrfsChatWindow->chatWindowGroup->setButton( p->chatWindowPolicy() );
	mPrfsChatWindow->mTransparencyEnabled->setChecked( p->transparencyEnabled() );
	mPrfsChatWindow->mTransparencyTintColor->setColor( p->transparencyColor() );
	mPrfsChatWindow->mTransparencyValue->setValue( p->transparencyValue() );
	mPrfsChatWindow->mTransparencyBgOverride->setChecked( p->bgOverride() );
	mPrfsChatWindow->interfaceGroup->setButton( p->interfacePreference() );
	mPrfsChatWindow->mChatViewBufferSize->setValue( p->chatViewBufferSize() );

	// "Chat Appearance" TAB
	mPrfsChatAppearance->highlightEnabled->setChecked( p->highlightEnabled() );
	mPrfsChatAppearance->foregroundColor->setColor( p->highlightForeground() );
	mPrfsChatAppearance->backgroundColor->setColor( p->highlightBackground() );

	mPrfsChatAppearance->textColor->setColor( p->textColor() );
	mPrfsChatAppearance->linkColor->setColor( p->linkColor() );
	mPrfsChatAppearance->bgColor->setColor( p->bgColor() );
	mPrfsChatAppearance->fontFace->setFont( p->fontFace() );
	mPrfsChatAppearance->fontFace->setText( p->fontFace().family() );

	mChatStyles = p->chatStyles();
	mPrfsChatAppearance->styleList->clear();
	for( KopeteChatStyleMap::Iterator it = mChatStyles.begin(); it != mChatStyles.end(); ++it)
	{
		mPrfsChatAppearance->styleList->insertItem( it.key() );
		if( it.data() == p->kindMessagesHtml() )
			mPrfsChatAppearance->styleList->setSelected( mPrfsChatAppearance->styleList->findItem(it.key()), true );
	}
	mPrfsChatAppearance->styleList->sort();

  	const QString wthis = i18n("Code:                                <br>\
		<b>%M</b> : insert the Message                                   <br>\
		<b>%T</b> : insert Timestamp                                     <br>\
		<b>%F</b> : insert Fonts (the first open and the second close)   <br>\
		<b>%b</b> : background color in html format ( #ABABAB )          <br>\
		<b>%f</b> : insert the displayName of the sender                 <br>\
		<b>%t</b> : insert the displayName of the reciever               <br>\
									<br>\
		<b>%i ... %i </b> : code between is parsed only if message is inbound <br> \
		<b>%o ... %o </b> : code between is parsed only if message is outbound <br> \
		<b>%a ... %a </b> : code between is parsed only if message is an action <br> \
		<b>%s ... %s </b>: code between is parsed only if message is internal <br>\
		<b>%e ... %e </b>: code between is parsed only if message is inbound or outbound <br>\
									<br>\
		<b>%%</b> : insert a '%'");

	//mPrfsChatAppearance->mle_codehtml->setText( p->kindMessagesHtml() );
  	//QWhatsThis::add( mPrfsChatAppearance->mle_codehtml, wthis );
	mAwayConfigUI->updateView();
}

void AppearanceConfig::slotConfigSound()
{
	KNotifyDialog::configure(this);
}

void AppearanceConfig::slotUseEmoticonsChanged ( bool checked )
{
	icon_theme_list->setEnabled( checked );
}

void AppearanceConfig::slotTransparencyChanged ( bool checked )
{
	mPrfsChatWindow->mTransparencyTintColor->setEnabled( checked );
	mPrfsChatWindow->mTransparencyValue->setEnabled( checked );
	mPrfsChatWindow->mTransparencyBgOverride->setEnabled( checked );
}

void AppearanceConfig::slotHighlightChanged()
{
	bool value = mPrfsChatAppearance->highlightEnabled->isChecked();
	mPrfsChatAppearance->foregroundColor->setEnabled ( value );
	mPrfsChatAppearance->backgroundColor->setEnabled ( value );
	slotUpdatePreview();
}

void AppearanceConfig::slotShowTrayChanged()
{
	bool check = mPrfsGeneral->mShowTrayChk->isChecked();

	mPrfsGeneral->mStartDockedChk->setEnabled(check);
	mPrfsGeneral->mTrayflashNotifyChk->setEnabled(check);
	mPrfsGeneral->mBalloonNotifyChk->setEnabled(check);
}

void AppearanceConfig::slotChangeFont()
{
	QFont mFont = KopetePrefs::prefs()->fontFace();
	KFontDialog::getFont( mFont );
	KopetePrefs::prefs()->setFontFace( mFont );
	mPrfsChatAppearance->fontFace->setFont( mFont );
	mPrfsChatAppearance->fontFace->setText( mFont.family() );
	slotUpdatePreview();
}

void AppearanceConfig::slotAddStyle()
{
	editedItem = 0L;
	styleEditor = new StyleEditDialog(0L,"style", true);
	(new QHBoxLayout( styleEditor->editFrame ))->setAutoAdd( true );
	KTrader::OfferList offers = KTrader::self()->query( "KTextEditor/Document" );
	KService::Ptr service = *offers.begin();
	KLibFactory *factory = KLibLoader::self()->factory( service->library() );
	editDocument = static_cast<KTextEditor::Document *>( factory->create( styleEditor->editFrame, 0, "KTextEditor::Document" ) );
	editDocument->createView( styleEditor->editFrame, 0 )->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding) );
	KTextEditor::editInterface( editDocument )->setText( QString::fromLatin1("<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">\n\n\n\n</td></tr></table>") );
	updateHighlight();
	styleEditor->show();
	connect( styleEditor->buttonOk, SIGNAL(clicked()), this, SLOT(slotStyleSaved()) );
	connect( styleEditor->buttonCancel, SIGNAL(clicked()), styleEditor, SLOT(deleteLater()) );
}

void AppearanceConfig::updateHighlight()
{
	KTextEditor::HighlightingInterface *hi = KTextEditor::highlightingInterface( editDocument );
	int count = hi->hlModeCount();
	for( int i=0; i < count; i++ )
	{
		if( hi->hlModeName(i) == QString::fromLatin1("HTML") )
		{
			hi->setHlMode(i);
			break;
		}
	}
}

void AppearanceConfig::slotEditStyle()
{
	slotAddStyle();
	editedItem = mPrfsChatAppearance->styleList->selectedItem();
	KTextEditor::editInterface( editDocument )->setText( mChatStyles[ editedItem->text() ] );
	updateHighlight();
	styleEditor->styleName->setText( editedItem->text() );
}

void AppearanceConfig::slotDeleteStyle()
{
	if( KMessageBox::warningContinueCancel( this, i18n("Are you sure you want to delete the style \"%1\"?")
		.arg( mPrfsChatAppearance->styleList->selectedItem()->text() ),
		i18n("Delete Style"), i18n("Delete Style")) == KMessageBox::Continue )
	{
		QListBoxItem *style = mPrfsChatAppearance->styleList->selectedItem();
		mChatStyles.remove( style->text() );
		if( style->next() )
			mPrfsChatAppearance->styleList->setSelected( style->next(), true );
		else
			mPrfsChatAppearance->styleList->setSelected( style->prev(), true );
		delete style;
	}
}

void AppearanceConfig::slotStyleSaved()
{
	delete editedItem;
	mChatStyles[ styleEditor->styleName->text() ] = KTextEditor::editInterface( editDocument )->text();

	mPrfsChatAppearance->styleList->insertItem( styleEditor->styleName->text() );
	mPrfsChatAppearance->styleList->sort();
	mPrfsChatAppearance->styleList->setSelected( mPrfsChatAppearance->styleList->findItem( styleEditor->styleName->text() ), true );

	styleEditor->deleteLater();
}

void AppearanceConfig::slotUpdatePreview()
{
	KopeteContact *cFrom = new KopeteContact((KopeteAccount*)0L, QString::fromLatin1("UserFrom"), 0L);
	KopeteContact *cTo = new KopeteContact((KopeteAccount*)0L, QString::fromLatin1("UserTo"), 0L);

	KopeteContactPtrList toList = KopeteContactPtrList();
	toList.append( cTo );

	KopeteMessage *msgIn = new KopeteMessage( cFrom, toList, QString::fromLatin1("This is an incoming message"),KopeteMessage::Inbound );
	KopeteMessage *msgOut = new KopeteMessage( cFrom, toList, QString::fromLatin1("This is an outgoing message"),KopeteMessage::Outbound );
	KopeteMessage *msgInt = new KopeteMessage( cFrom, toList, QString::fromLatin1("This is an internal message"),KopeteMessage::Internal );
	KopeteMessage *msgHigh = new KopeteMessage( cFrom, toList, QString::fromLatin1("This is a highlighted message"),KopeteMessage::Inbound );
	KopeteMessage *msgAct = new KopeteMessage( cFrom, toList, QString::fromLatin1("This is an action message"),KopeteMessage::Action );

	QString model;
	QListBoxItem *style = mPrfsChatAppearance->styleList->selectedItem();
	if( style )
	{
		if( mChatStyles.contains( style->text() ) )
			model = mChatStyles[ style->text() ];
	}

	preview->begin();
	preview->write( QString::fromLatin1( "<html><head><style>body{font-family:%1;color:%2;}td{font-family:%3;color:%4}</style></head><body bgcolor=\"%5\" vlink=\"%6\" link=\"%7\">" )
		.arg( mPrfsChatAppearance->fontFace->font().family() )
		.arg( mPrfsChatAppearance->textColor->color().name() )
		.arg( mPrfsChatAppearance->fontFace->font().family() )
		.arg( mPrfsChatAppearance->textColor->color().name() )
		.arg( mPrfsChatAppearance->bgColor->color().name() )
		.arg( mPrfsChatAppearance->linkColor->color().name() )
		.arg( mPrfsChatAppearance->linkColor->color().name() ) );


	// incoming messages
	preview->write( msgIn->transformMessage( model ) );
	msgIn->setFg(Qt::white);
	msgIn->setBg(Qt::blue);
	msgIn->setBody( QString::fromLatin1("This is a colored incoming message (random color)") );
	preview->write( msgIn->transformMessage( model ) );
	// -------------------

	// internal message
	preview->write( msgInt->transformMessage( model ) );

	//highlighted message
	if( mPrfsChatAppearance->highlightEnabled->isChecked() )
	{
		msgHigh->setFg( mPrfsChatAppearance->foregroundColor->color() );
		msgHigh->setBg( mPrfsChatAppearance->backgroundColor->color() );
	}
	preview->write( msgHigh->transformMessage( model ) );

	preview->write( msgAct->transformMessage( model ) );

	preview->write( QString::fromLatin1( "</body></html>" ) );
	preview->end();

	delete msgIn;
	delete msgOut;
	delete msgInt;
	delete msgHigh;
	delete msgAct;
	delete cFrom;
	delete cTo;
}

#include "appearanceconfig.moc"

// vim: set noet ts=4 sts=4 sw=4:
