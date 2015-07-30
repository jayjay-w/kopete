#include "historypreferences.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <kpluginfactory.h>

#include "historyconfig.h"
#include "ui_historypreferencesui.h"

K_PLUGIN_FACTORY( HistoryPreferencesFactory, registerPlugin<HistoryPreferences>(); )
K_EXPORT_PLUGIN( HistoryPreferencesFactory( "kcm_kopete_history" ))

HistoryPreferences::HistoryPreferences(QWidget *parent, const QVariantList &args)
	: KCModule(HistoryPreferencesFactory::componentData(), parent, args)
{
	//Initialize the UI. This is done by creating a layout and adding the UI constructed in
	//the .ui file.
	QVBoxLayout *layout = new QVBoxLayout(this);
	QWidget *widget = new QWidget;
	ui = new Ui::HistoryPreferencesUI;
	ui->setupUi(widget);
	layout->addWidget(widget);
	widget->setWindowTitle("Kopete History Settings");

	//Set up signal-slot connections. These will be used to notify the KCModule that we have
	//changes waiting to be saved.
	connect (ui->chkLoadRecentMessages, SIGNAL(toggled), this, SLOT(slotModified()));
	connect (ui->spinNoOfMessagesToLoad, SIGNAL(valueChanged(int)), this, SLOT(slotModified()));
	connect (ui->cboDbSystem, SIGNAL(currentIndexChanged(int)), this, SLOT(slotModified()));

}

HistoryPreferences::~HistoryPreferences()
{
	//Delete the UI objects before exiting.
	delete ui;
}

void HistoryPreferences::save()
{
	//If changed has been emitted as true, here we will save all of the changes
	//to the config file.
	HistoryConfig::setLoadOldMessages(ui->chkLoadRecentMessages->isChecked());
	HistoryConfig::setNoOfOldMessages(ui->spinNoOfMessagesToLoad->value());
	HistoryConfig::setDbType(ui->cboDbSystem->currentText());
	HistoryConfig::self()->writeConfig();
	emit KCModule::changed(false);
}

void HistoryPreferences::load()
{
	//Load the settings from the config file.
	ui->chkLoadRecentMessages->setChecked(HistoryConfig::loadOldMessages());
	ui->spinNoOfMessagesToLoad->setValue(HistoryConfig::noOfOldMessages());
	ui->cboDbSystem->setEditText(HistoryConfig::dbType());
	emit KCModule::changed(false);
}

void HistoryPreferences::slotModified()
{
	//Notify the KCModule that we have changes that need saving.
	emit KCModule::changed(true);
}

