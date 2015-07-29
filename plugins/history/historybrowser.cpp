#include "historybrowser.h"
#include "ui_historybrowser.h"

HistoryBrowser::HistoryBrowser(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::HistoryBrowser)
{
	ui->setupUi(this);
}

HistoryBrowser::~HistoryBrowser()
{
	delete ui;
}
