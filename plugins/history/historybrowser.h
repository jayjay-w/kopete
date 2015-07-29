#ifndef HISTORYBROWSER_H
#define HISTORYBROWSER_H

#include <QWidget>

namespace Ui {
class HistoryBrowser;
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
private:
	Ui::HistoryBrowser *ui;
};

#endif // HISTORYBROWSER_H
