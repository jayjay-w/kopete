#include "historybrowser.h"
#include "ui_historybrowser.h"

#include "kopeteaccountmanager.h"
#include "kopeteaccount.h"
#include "kopeteprotocol.h"

#include <QTreeWidgetItem>

HistoryBrowser::HistoryBrowser(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::HistoryBrowser)
{
	ui->setupUi(this);
	loadAccounts();
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
	}
}
