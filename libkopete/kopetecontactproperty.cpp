/*
    kopetecontactproperty.cpp

    Kopete::Contact Property class

    Copyright (c) 2004    by Stefan Gehn <metz AT gehn.net>
    Copyright (c) 2006    by Michaël Larouche <michael.larouche@kdemail.net>

    Kopete    (c) 2004-2006    by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontactproperty.h"
#include <kdebug.h>
#include "kopeteglobal.h"

namespace Kopete
{

class ContactPropertyTmpl::Private
{
public:
	QString key;
	QString label;
	QString icon;
	ContactPropertyOptions options;
	unsigned int refCount;
};

ContactPropertyTmpl ContactPropertyTmpl::null;


ContactPropertyTmpl::ContactPropertyTmpl()
{
	d = new Private;
	d->refCount = 1;
	d->options = NoProperty;
	// Don't register empty template
}

ContactPropertyTmpl::ContactPropertyTmpl(const QString &key,
	const QString &label, const QString &icon, ContactPropertyOptions options)
{
	ContactPropertyTmpl other = Kopete::Global::Properties::self()->tmpl(key);
	if(other.isNull())
	{
//		kDebug(14000) << k_funcinfo << "Creating new template for key = '" << key << "'" << endl;

		d = new Private;
		d->refCount = 1;
		d->key = key;
		d->label = label;
		d->icon = icon;
		d->options = options;
		Kopete::Global::Properties::self()->registerTemplate(key, (*this));
	}
	else
	{
//		kDebug(14000) << k_funcinfo << "Using existing template for key = '" << key << "'" << endl;
		d = other.d;
		d->refCount++;
	}
}

ContactPropertyTmpl::ContactPropertyTmpl(const ContactPropertyTmpl &other)
{
	d = other.d;
	d->refCount++;
}

ContactPropertyTmpl &ContactPropertyTmpl::operator=(
	const ContactPropertyTmpl &other)
{
	d->refCount--;
	if(d->refCount == 0)
	{
		if (!d->key.isEmpty()) // null property
			Kopete::Global::Properties::self()->unregisterTemplate(d->key);
		delete d;
	}

	d = other.d;
	d->refCount++;

	return *this;
}

ContactPropertyTmpl::~ContactPropertyTmpl()
{
	d->refCount--;
	if(d->refCount == 0)
	{
		if (!d->key.isEmpty()) // null property
			Kopete::Global::Properties::self()->unregisterTemplate(d->key);
		delete d;
	}
}

bool ContactPropertyTmpl::operator==(const ContactPropertyTmpl &other) const
{
	return (d && other.d &&
		d->key == other.d->key &&
		d->label == other.d->label &&
		d->icon == other.d->key &&
		d->options == other.d->options);
}

bool ContactPropertyTmpl::operator!=(const ContactPropertyTmpl &other) const
{
	return (!d || !other.d ||
		d->key != other.d->key ||
		d->label != other.d->label ||
		d->icon != other.d->key ||
		d->options != other.d->options);
}


const QString &ContactPropertyTmpl::key() const
{
	return d->key;
}

const QString &ContactPropertyTmpl::label() const
{
	return d->label;
}

const QString &ContactPropertyTmpl::icon() const
{
	return d->icon;
}

ContactPropertyTmpl::ContactPropertyOptions ContactPropertyTmpl::options() const
{
	return d->options;
}

bool ContactPropertyTmpl::persistent() const
{
	return d->options & PersistentProperty;
}

bool ContactPropertyTmpl::isRichText() const
{
	return d->options & RichTextProperty;
}

bool ContactPropertyTmpl::isPrivate() const
{
	return d->options & PrivateProperty;
}

bool ContactPropertyTmpl::isNull() const
{
	return (!d || d->key.isNull());
}


// -----------------------------------------------------------------------------


ContactProperty ContactProperty::null;

class ContactProperty::Private
{
public:
	QVariant value;
	ContactPropertyTmpl propertyTemplate;
};

ContactProperty::ContactProperty()
 : d(new Private)
{
}

ContactProperty::ContactProperty(const ContactPropertyTmpl &tmpl,
	const QVariant &val)
 : d(new Private)
{
	d->propertyTemplate = tmpl;
	d->value = val;
}

ContactProperty::~ContactProperty()
{
	//kDebug(14000) << k_funcinfo << "this = " << (void *)this << endl;
}

const QVariant &ContactProperty::value() const
{
	return d->value;
}

const ContactPropertyTmpl &ContactProperty::tmpl() const
{
	return d->propertyTemplate;
}

bool ContactProperty::isNull() const
{
	return d->value.isNull();
}

bool ContactProperty::isRichText() const
{
	return d->propertyTemplate.isRichText();
}

} // END namespace Kopete
