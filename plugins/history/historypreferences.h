#ifndef HISTORYPREFERENCES_H
#define HISTORYPREFERENCES_H

#include <kcmodule.h>

namespace Ui { class HistoryPreferencesUI; }

class HistoryPreferences : public KCModule
{
	Q_OBJECT
public:
	/**
	 * @brief The HistoryPreferences class is used to display setting for the history plugin. To access
	 * the settings, open the Kopete settings page and click the config icon next to the plugin name.
	 * @param parent The owner of the dialog
	 */
	HistoryPreferences(QWidget *parent = 0, const QVariantList &args = QVariantList() );

	/**
	 * @brief Class destructor. The ui is deleted here.
	 */
	virtual ~HistoryPreferences();

	/**
	 * @brief Save any changes made to the plugin settings.
	 */
	virtual void save();

	/**
	 * @brief Load the saved settings when starting the dialog.
	 */
	virtual void load();

private slots:
	/**
	 * @brief This is emitted whenever any of the setting items are changed.
	 */
	void slotModified();
private:
	Ui::HistoryPreferencesUI *ui;
};

#endif // HISTORYPREFERENCES_H
