#ifndef HISTORYBROWSER_H
#define HISTORYBROWSER_H

#include <QWidget>

namespace Ui {
class HistoryBrowser;
}

class QTreeWidgetItem;
class KAction;
class KHTMLView;
class KHTMLPart;
namespace Kopete {
	class Account;
	class XSLT;
}

class HistoryBrowser : public QWidget
{
	Q_OBJECT

public:
	explicit HistoryBrowser(QWidget *parent = 0);
	~HistoryBrowser();

private slots:
	/**
	 * Load the account list onto the accounts tree view.
	 */
	void loadAccounts();

	/**
	 * Load the contacts for a particular account that have a chat history onto the contacts window.
	 * @param account The account to look for
	 * @param parentItem The respective QTreeWidhetItem where the contacts will be based.
	 */
	void loadContacts(Kopete::Account *account, QTreeWidgetItem *parentItem);

	void on_trvContacts_itemClicked(QTreeWidgetItem *item, int);
private:
	Ui::HistoryBrowser *ui;
	KHTMLView *mHtmlView;
	KHTMLPart *mHtmlPart;
	Kopete::XSLT *mXsltParser;
};

#endif // HISTORYBROWSER_H
