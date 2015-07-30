#include "historybrowser.h"
#include "ui_historybrowser.h"

#include "kopeteaccountmanager.h"
#include "kopeteaccount.h"
#include "kopeteprotocol.h"
#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopetecontactlist.h"
#include "kopeteappearancesettings.h"
#include "kopetemetacontact.h"
#include <QLineEdit>
#include <dom/dom_doc.h>
#include <dom/dom_element.h>
#include <dom/html_document.h>
#include <dom/html_element.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <QTextBrowser>
#include "databasemanager.h"

#include <QTreeWidgetItem>
#include <QSplitter>
#include <QMessageBox>

HistoryBrowser::HistoryBrowser(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::HistoryBrowser)
{
	ui->setupUi(this);

	//Setup the history viewer
	QString fontSize;
	QString htmlCode;
	QString fontStyle;
	mHtmlPart = new KHTMLPart(this);

	mHtmlPart->setJScriptEnabled(false);
	mHtmlPart->setJavaEnabled(false);
	mHtmlPart->setPluginsEnabled(false);
	mHtmlPart->setMetaRefreshEnabled(false);
	mHtmlPart->setOnlyLocalReferences(true);

	mHtmlView = mHtmlPart->view();
	mHtmlView->setMarginWidth(4);
	mHtmlView->setMarginHeight(4);
	mHtmlView->setFocusPolicy(Qt::ClickFocus);
	mHtmlView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	ui->splitter->addWidget(mHtmlView);

	QTextStream( &fontSize ) << Kopete::AppearanceSettings::self()->chatFont().pointSize();
	fontStyle = "<style>.hf { font-size:" + fontSize + ".0pt; font-family:" + Kopete::AppearanceSettings::self()->chatFont().family() + "; color: " + Kopete::AppearanceSettings::self()->chatTextColor().name() + "; }</style>";

	mHtmlPart->begin();
	htmlCode = "<html><head>" + fontStyle + "</head><body class=\"hf\"></body></html>";
	mHtmlPart->write( QString::fromLatin1( htmlCode.toLatin1() ) );
	mHtmlPart->end();

	//Then we display the list of accounts
	loadAccounts();
	connect (ui->trvContacts, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(on_trvContacts_itemClicked(QTreeWidgetItem*,int)));
}

HistoryBrowser::~HistoryBrowser()
{
	delete ui;
}

void HistoryBrowser::loadAccounts()
{
	foreach (Kopete::Account *account, Kopete::AccountManager::self()->accounts()) {
		QTreeWidgetItem *accountItem = new QTreeWidgetItem(ui->trvContacts);
		accountItem->setText(0, account->accountLabel());
		accountItem->setIcon(0, account->accountIcon());
		accountItem->setText(99, account->accountId());
		accountItem->setText(100, "Account");
		loadContacts(account, accountItem);
	}
}

void HistoryBrowser::loadContacts(Kopete::Account *account, QTreeWidgetItem *parentItem)
{
	QList<Kopete::Contact *> contacts = DatabaseManager::instance()->getContactList(account);

	foreach (Kopete::Contact *contact, contacts) {
		QTreeWidgetItem *contactItem = new QTreeWidgetItem(parentItem);
		contactItem->setText(0, contact->displayName());
		contactItem->setText(99, contact->contactId());
		contactItem->setText(100, "Contact");
	}
}

void HistoryBrowser::on_trvContacts_itemClicked(QTreeWidgetItem *item, int)
{
	if (item->text(100) != "Contact")
		return; //This is not a contact


	QString contactId = item->text(99);
	QList<Kopete::Message> msgs = DatabaseManager::instance()->getMessages(contactId);
	// Clear View
	DOM::HTMLElement htmlBody = mHtmlPart->htmlDocument().body();
	while(htmlBody.hasChildNodes())
		htmlBody.removeChild(htmlBody.childNodes().item(htmlBody.childNodes().length() - 1));
	// ----

	QString dir = (QApplication::isRightToLeft() ? QString::fromLatin1("rtl") :
						       QString::fromLatin1("ltr"));
	QString accountLabel;
	QString date = msgs.isEmpty() ? "" : msgs.front().timestamp().date().toString();
	QString resultHTML = "<b><font color=\"red\">" + date + "</font></b><br/>";
	DOM::HTMLElement newNode = mHtmlPart->document().createElement(QString::fromLatin1("span"));
	newNode.setAttribute(QString::fromLatin1("dir"), dir);
	newNode.setInnerHTML(resultHTML);
	mHtmlPart->htmlDocument().body().appendChild(newNode);
	//const QString searchForEscaped = Qt::escape(mMainWidget->searchLine->text());

	// Populating HTML Part with messages
	foreach(const Kopete::Message& msg, msgs)
	{
		if ( ui->txtSearch->text().isEmpty() )
		{
			resultHTML.clear();
			if (accountLabel.isEmpty() || accountLabel != item->parent()->text(0))
				// If the message's account is new, just specify it to the user
			{
				if (!accountLabel.isEmpty())
					resultHTML += "<br/><br/><br/>";

				resultHTML += "<b><font color=\"blue\">" + item->parent()->text(0) + "</font></b><br/>";
			}
			accountLabel = item->parent()->text(0);

			QString body = msg.parsedBody();
			QMessageBox::information(this, "Debug", "6c");
			// If there is a search, then we highlight the keywords
			//			if (!searchForEscaped.isEmpty() && body.contains(searchForEscaped, Qt::CaseInsensitive))
			//				body = highlight( body, searchForEscaped );

			QString name;
			if ( msg.from()->metaContact() && msg.from()->metaContact() != Kopete::ContactList::self()->myself() )
			{
				name = msg.from()->metaContact()->displayName();
			}
			else
			{
				name = msg.from()->displayName();
			}
			QMessageBox::information(this, "Debug", "7");
			QString fontColor;
			if (msg.direction() == Kopete::Message::Outbound)
			{
				fontColor = Kopete::AppearanceSettings::self()->chatTextColor().dark().name();
			}
			else
			{
				fontColor = Kopete::AppearanceSettings::self()->chatTextColor().light(200).name();
			}

			QMessageBox::information(this, "Debug", "8");
			QString messageTemplate = "<b>%1&nbsp;<font color=\"%2\">%3</font></b>&nbsp;%4";
			resultHTML += messageTemplate.arg( msg.timestamp().time().toString(),
							   fontColor, name, body );

			newNode = mHtmlPart->document().createElement(QString::fromLatin1("span"));
			newNode.setAttribute(QString::fromLatin1("dir"), dir);
			newNode.setInnerHTML(resultHTML);

			mHtmlPart->htmlDocument().body().appendChild(newNode);
		}
		QMessageBox::information(this, "Debug", "9");
	}
	QMessageBox::information(this, "Debug", "10");
}
