#include "smssend.h"
#include "smssendprefs.h"
#include "smssendprovider.h"
#include "smsprotocol.h"
#include "kopeteaccount.h"

#include <qcombobox.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kmessagebox.h>
#include <kdebug.h>

SMSSend::SMSSend(KopeteAccount* account)
	: SMSService(account)
{
	prefWidget = 0L;
	m_provider = 0L;
}

SMSSend::~SMSSend()
{
}

void SMSSend::send(const KopeteMessage& msg)
{
	kdWarning( 14160 ) << k_funcinfo << "m_account = " << m_account << " (should be non-zero!!)" << endl;
	QString provider = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("ProviderName"));

	if (provider.length() < 1)
	{
		KMessageBox::error(0L, i18n("No provider configured"), i18n("Could Not Send Message"));
		return;
	}

	QString prefix = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("Prefix"));
	if (prefix.isNull())
	{
		KMessageBox::error(0L, i18n("No prefix set for SMSSend, please change it in the configuration dialog"), i18n("No Prefix"));
		return;
	}

	m_provider = new SMSSendProvider(provider, prefix, m_account, this);

	connect( m_provider, SIGNAL(messageSent(const KopeteMessage &)), this, SIGNAL(messageSent(const KopeteMessage &)));

	m_provider->send(msg);
}

QWidget* SMSSend::configureWidget(QWidget* parent)
{
	if (prefWidget == 0L)
		prefWidget = new SMSSendPrefsUI(parent);

	prefWidget->program->setMode(KFile::Directory);

	QString prefix = QString::null;

	if (m_account)
		prefix = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("Prefix"));
	if (prefix.isNull())
	{
		QDir d("/usr/share/smssend");
		if (d.exists())
		{
			prefix = "/usr";
		}
		d = "/usr/local/share/smssend";
		if (d.exists())
		{
			prefix="/usr/local";
		}
		else
		{
			prefix="/usr";
		}
	}

	connect (prefWidget->program, SIGNAL(textChanged(const QString &)),
		this, SLOT(loadProviders(const QString&)));

	prefWidget->program->setURL(prefix);

	loadProviders(prefix);

	connect(prefWidget->provider, SIGNAL(activated(const QString &)),
		this, SLOT(setOptions(const QString &)));

	return prefWidget;
}

void SMSSend::savePreferences()
{
	if (prefWidget != 0L && m_account != 0L && m_provider != 0L )
	{
		m_account->setPluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("Prefix"), prefWidget->program->url());
		m_account->setPluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("ProviderName"), prefWidget->provider->currentText());
		m_provider->save(args);
	}
}

void SMSSend::loadProviders(const QString &prefix)
{
	kdWarning( 14160 ) << k_funcinfo << "m_account = " << m_account << " (should be ok if zero)" << endl;

	QStringList p;

	prefWidget->provider->clear();

	QDir d(prefix + "/share/smssend");
	if (!d.exists())
	{
		setOptions(QString::null);
		return;
	}

	p = d.entryList("*.sms");

	d = QDir::homeDirPath()+"/.smssend/";

	QStringList tmp(d.entryList("*.sms"));

	for (QStringList::Iterator it = tmp.begin(); it != tmp.end(); ++it)
		p.prepend(*it);

	for (QStringList::iterator it = p.begin(); it != p.end(); ++it)
		(*it).truncate((*it).length()-4);

	prefWidget->provider->insertStringList(p);

	bool found = false;
	if (m_account)
	{	QString pName = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("ProviderName"));
		for (int i=0; i < prefWidget->provider->count(); i++)
		{
			if (prefWidget->provider->text(i) == pName)
			{
				found=true;
				prefWidget->provider->setCurrentItem(i);
				setOptions(pName);
				break;
			}
		}
	}
	if (!found)
		setOptions(prefWidget->provider->currentText());
}

void SMSSend::setOptions(const QString& name)
{
	kdWarning( 14160 ) << k_funcinfo << "m_account = " << m_account << " (should be ok if zero!!)" << endl;

	prefWidget->settingsBox->setTitle(name);

	args.setAutoDelete(true);
	args.clear();

	if (m_provider) delete m_provider;
	m_provider = new SMSSendProvider(name, prefWidget->program->url(), m_account, this);

	for (int i=0; i < m_provider->count(); i++)
	{
		if (!m_provider->name(i).isNull())
		{
			SMSSendArg* a = new SMSSendArg(prefWidget->settingsBox);
			a->argName->setText(m_provider->name(i));
			a->value->setText(m_provider->value(i));
			a->description->setText(m_provider->description(i));
			args.append(a);
			a->show();
		}
	}
}
void SMSSend::setAccount(KopeteAccount* account)
{
	m_provider->setAccount(account);
	SMSService::setAccount(account);
}

int SMSSend::maxSize()
{
	kdWarning( 14160 ) << k_funcinfo << "m_account = " << m_account << " (should be non-zero!!)" << endl;

	QString pName = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("ProviderName"));
	if (pName.length() < 1)
		return 160;
	QString prefix = m_account->pluginData(SMSProtocol::protocol(), QString("%1:%2").arg("SMSSend").arg("Prefix"));
	if (prefix.isNull())
		prefix = "/usr";
	// quick sanity check
	if (!m_provider) return 0;
	return m_provider->maxSize();
}

const QString& SMSSend::description()
{
	QString url = "http://zekiller.skytech.org/smssend_en.php";
	m_description = i18n("<qt>SMSSend is a program for sending SMS through gateways on the web. It can be found on <a href=\"%1\">%2</a></qt>").arg(url).arg(url);
	return m_description;
}


#include "smssend.moc"
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

