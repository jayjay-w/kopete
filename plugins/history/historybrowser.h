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

private:
	Ui::HistoryBrowser *ui;
};

#endif // HISTORYBROWSER_H
