/*
    Kopete View Item Delegate

    Copyright (c) 2007 by Matt Rogers <mattr@kde.org>
    Copyright (c) 2009 by Roman Jarosz <kedgedev@gmail.com>

    Kopete    (c) 2002-2009 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopeteitemdelegate.h"
#include "kopeteitembase.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QAbstractItemView>
#include <QApplication>
#include <QVector>

#include <qimageblitz.h>

#include <KIconLoader>

#include "kopetemetacontact.h"
#include "kopetecontact.h"
#include "kopeteappearancesettings.h"
#include "contactlistlayoutmanager.h"
#include "contactlistproxymodel.h"

const qreal MARGIN = 2.0;
const qreal MARGINH = 6.0;
const qreal PADDING = 1.0;

KopeteItemDelegate::KopeteItemDelegate( QAbstractItemView* parent )
: QStyledItemDelegate( parent )
{
}

KopeteItemDelegate::~KopeteItemDelegate()
{
}

QFont KopeteItemDelegate::normalFont( const QFont& naturalFont )
{
	if ( Kopete::AppearanceSettings::self()->contactListUseCustomFont() )
		return Kopete::AppearanceSettings::self()->contactListNormalFont();
	else
		return naturalFont;
}

QFont KopeteItemDelegate::smallFont( const QFont& naturalFont )
{
	if ( Kopete::AppearanceSettings::self()->contactListUseCustomFont() )
		return Kopete::AppearanceSettings::self()->contactListSmallFont();

	QFont font( naturalFont );
	if ( font.pixelSize() != -1 )
		font.setPixelSize( (font.pixelSize() * 3) / 4 );
	else
		font.setPointSizeF( font.pointSizeF() * 0.75 );
	return font;
}

QSize KopeteItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if ( index.data( Kopete::Items::TypeRole ) == Kopete::Items::MetaContact )
	{
		QFont normal = normalFont( option.font );
		QFont small = smallFont( option.font );

		qreal height = 0;

		ContactList::ContactListLayout layout = ContactList::LayoutManager::instance()->activeLayout();
		int rowCount = layout.layout().rows();
		for ( int i = 0; i < rowCount; i++ )
			height += calculateRowHeight( layout.layout().row( i ), normal, small );

		height += MARGIN * 2 + ( rowCount - 1 ) * PADDING;
		return QSize( 120, height );
		
	}
	else
		return QStyledItemDelegate::sizeHint( option, index );
}

Kopete::Contact* KopeteItemDelegate::contactAt( const QStyleOptionViewItem& option, const QModelIndex& index, const QPoint& point ) const
{
	if ( index.data( Kopete::Items::TypeRole ) == Kopete::Items::MetaContact )
	{
		ContactList::ContactListLayout layout = ContactList::LayoutManager::instance()->activeLayout();
		QList< QPair<QRect, Kopete::Contact*> > contactPositionList;
		paintItem( layout.layout(), 0, option, index, &contactPositionList );

		QPoint delegatePoint = point - option.rect.topLeft();
		for ( int i = 0; i < contactPositionList.size(); ++i )
		{
			if ( contactPositionList.at(i).first.contains( delegatePoint ) )
				return contactPositionList.at(i).second;
		}
	}
	return 0;
}

void KopeteItemDelegate::paint( QPainter* painter, 
								const QStyleOptionViewItem& option,
								const QModelIndex& index ) const
{
	//pull in contact settings: idleContactColor, greyIdleMetaContacts
	//pull in contact list settings: contactListDisplayMode
	QStyleOptionViewItem opt = option;
	
	if ( index.data( Kopete::Items::TypeRole ) == Kopete::Items::MetaContact )
	{
		ContactList::ContactListLayout layout = ContactList::LayoutManager::instance()->activeLayout();

		painter->save();

		// Draw background
		QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

		painter->translate( option.rect.topLeft() );

		if ( Kopete::AppearanceSettings::self()->greyIdleMetaContacts() && index.data( Kopete::Items::IdleTimeRole ).toInt() > 0 )
			painter->setPen( Kopete::AppearanceSettings::self()->idleContactColor() ); //apply the appropriate idle color
		else if ( option.state & QStyle::State_Selected )
			painter->setPen( option.palette.color( QPalette::Normal, QPalette::HighlightedText ) );
		else
			painter->setPen( option.palette.color( QPalette::Normal, QPalette::Text ) );

		paintItem( layout.layout(), painter, option, index, 0 );

		painter->restore();
	}
	else if (  index.data( Kopete::Items::TypeRole ) == Kopete::Items::Group )
	{
		QColor gc( Kopete::AppearanceSettings::self()->groupNameColor() );
		opt.palette.setColor( QPalette::Text, gc );
		QStyledItemDelegate::paint( painter, opt, index );
	}
	else
	{
		QStyledItemDelegate::paint( painter, opt, index );
	}
}

void KopeteItemDelegate::paintItem( ContactList::LayoutItemConfig config, QPainter* painter,
                                    const QStyleOptionViewItem& option, const QModelIndex& index,
                                    QList<QPair<QRect, Kopete::Contact*> >* contactPositionList ) const
{
	int rowCount = config.rows();
	if ( rowCount == 0 )
		return;

	const int hBorderMargin = MARGIN * 2;
	//const int hMargins = hBorderMargin + ( rowCount - 1 ) * PADDING;

	int rowOffsetX = MARGIN;
	int rowOffsetY = MARGIN;

	if ( config.showIcon() )
	{
		int imageSize = option.rect.height() - hBorderMargin;

		if ( painter )
		{
			QRectF nominalImageRect( rowOffsetX, rowOffsetY, imageSize, imageSize );

			QVariant metaContactPicture;
			if ( index.data( Kopete::Items::HasNewMessageRole ).toBool() )
				metaContactPicture = QString::fromUtf8( "mail-unread" );
			else
				metaContactPicture = index.data( Kopete::Items::MetaContactImageRole );

			if ( metaContactPicture.type() == QVariant::Image )
			{
				// We have contact photo
				QImage metaContactImage = metaContactPicture.value<QImage>();
				if ( !metaContactImage.isNull() )
				{
					metaContactImage = metaContactImage.scaled( imageSize, imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );

					int metaContactStatus = index.data( Kopete::Items::OnlineStatusRole ).toInt();
					if ( metaContactStatus == Kopete::OnlineStatus::Offline )
						Blitz::grayscale( metaContactImage );

					switch ( metaContactStatus )
					{
					case Kopete::OnlineStatus::Online:
						break;
					case Kopete::OnlineStatus::Away:
						Blitz::fade( metaContactImage, 0.5, Qt::white );
						break;
					case Kopete::OnlineStatus::Offline:
						Blitz::fade( metaContactImage, 0.4, Qt::white );
						break;
					case Kopete::OnlineStatus::Unknown:
					default:
						Blitz::fade( metaContactImage, 0.8, Qt::white );
					}

					QPixmap photoPixmap;
					bool roundedIcons = Kopete::AppearanceSettings::self()->contactListIconRounded();
					if ( roundedIcons )
					{
						photoPixmap = QPixmap( metaContactImage.width(), metaContactImage.height() );
						photoPixmap.fill( Qt::transparent );
						QPainter p( &photoPixmap );
						p.setRenderHint( QPainter::Antialiasing );
						p.setPen( Qt::NoPen );
						p.setBrush( QBrush( metaContactImage ) );
						QRectF rectangle( 0.5, 0.5, photoPixmap.width()-1, photoPixmap.height()-1 );
						p.drawRoundedRect( rectangle, 25, 25, Qt::RelativeSize );
					}
					else
					{
						photoPixmap = QPixmap::fromImage( metaContactImage );
					}

					if ( Kopete::AppearanceSettings::self()->contactListIconBorders() )
					{
						QPainter p( &photoPixmap );
						p.setPen( Qt::black );

						if ( roundedIcons )
						{
							p.setRenderHint( QPainter::Antialiasing );
							QRectF rectangle( 0.5, 0.5, photoPixmap.width()-1, photoPixmap.height()-1 );
							p.drawRoundedRect( rectangle, 25, 25, Qt::RelativeSize );
						}
						else
						{
							p.drawRect( 0, 0, photoPixmap.width()-1, photoPixmap.height()-1 );
						}
					}
					//offset cover if non square
					QPointF offset = centerImage( photoPixmap, nominalImageRect );
					QRectF imageRect( nominalImageRect.x() + offset.x(),
									nominalImageRect.y() + offset.y(),
									nominalImageRect.width() - offset.x() * 2,
									nominalImageRect.height() - offset.y() * 2 );

					painter->drawPixmap( imageRect, photoPixmap, QRectF( photoPixmap.rect() ) );
				}
			}
			else
			{
				// We have icon
				QString metaContactImageName = metaContactPicture.value<QString>();
				QPixmap metaContactImage = SmallIcon( metaContactImageName, imageSize );
				if ( !metaContactImage.isNull() )
				{
					//offset cover if non square
					QPointF offset = centerImage( metaContactImage, nominalImageRect );
					QRectF imageRect( nominalImageRect.x() + offset.x(),
									nominalImageRect.y() + offset.y(),
									nominalImageRect.width() - offset.x() * 2,
									nominalImageRect.height() - offset.y() * 2 );

					painter->drawPixmap( imageRect, metaContactImage, QRectF( metaContactImage.rect() ) );
				}
			}
		}

		rowOffsetX += imageSize + MARGINH;
	}

	QFont normal = normalFont( option.font );
	QFont small = smallFont( option.font );

	for ( int i = 0; i < rowCount; i++ )
	{
		ContactList::LayoutItemConfigRow row = config.row( i );
		const int rowHeight = calculateRowHeight( row, normal, small );
		qreal itemOffsetX = rowOffsetX;
		const int elementCount = row.count();
		qreal rowWidth = option.rect.width() - ( rowOffsetX + MARGINH );

		QRectF rowBox( itemOffsetX, rowOffsetY, rowWidth, rowHeight );
		int currentItemX = itemOffsetX;

		const qreal IconMarginH = 2.0;
		const qreal IconMarginV = 1.0;
		const qreal IconSize = rowHeight - 2 * IconMarginV;

		QVector<DynamicLayoutItem> dynamicLayoutData( elementCount );
		bool hasFixedTypeItem = false;

		// Figure out width of items
		for ( int j = 0; j < elementCount; ++j )
		{
			ContactList::LayoutItemConfigRowElement element = row.element( j );
			const int value = element.value();
			DynamicLayoutItem& dlItem = dynamicLayoutData[j];

			dlItem.dirty = true;
			if ( value != ContactList::LayoutManager::ContactIcons && value != ContactList::LayoutManager::PlaceHolder )
			{
				dlItem.font = QFont( ( element.small() ) ? small : normal );
				dlItem.font.setBold( element.bold() );
				dlItem.font.setItalic( element.italic() );

				const int role = ContactList::LayoutManager::instance()->token( value ).mModelRole;
				QString text = ( role > -1 ) ? index.data( role ).toString() : QString();
				dlItem.text = element.prefix() + text + element.suffix();
			}

			if ( element.optimalSize() )
			{
				qreal idealWidth = 0;
				if ( value == ContactList::LayoutManager::ContactIcons )
				{
					QObject* metaContactObject = qVariantValue<QObject*>( index.data( Kopete::Items::ObjectRole ) );
					Kopete::MetaContact* metaContact = qobject_cast<Kopete::MetaContact*>(metaContactObject);
					const int contactListSize = metaContact->contacts().size();
					idealWidth = contactListSize * IconSize;
					if ( contactListSize > 1 )
						idealWidth += (contactListSize - 1) * IconMarginH;

					hasFixedTypeItem = true;
					dlItem.type = LayoutFixed;
				}
				else if ( ContactList::LayoutManager::PlaceHolder )
				{
					dlItem.type = LayoutNormal;
				}
				else
				{
					QFontMetricsF fm( dlItem.font );
					idealWidth = fm.width( dlItem.text );
					dlItem.type = LayoutNormal;
				}

				if ( element.size() >= 0.001 )
				{
					const qreal maxWidth = rowWidth * element.size();
					if ( maxWidth < idealWidth)
						idealWidth = maxWidth;
				}
				dlItem.width = idealWidth;
			}
			else
			{
				if ( element.size() >= 0.001 )
				{
					dlItem.type = LayoutNormal;
					dlItem.width = rowWidth * element.size();
				}
				else
				{
					dlItem.type = LayoutAuto;
					dlItem.width = 0;
				}
			}
		}

		// Check width of fixed items
		qreal availableWidth = rowWidth;
		if ( hasFixedTypeItem )
		{
			for ( int j = 0; j < elementCount; ++j )
			{
				DynamicLayoutItem& dlItem = dynamicLayoutData[j];
				if ( dlItem.type == LayoutFixed )
				{
					availableWidth -= dlItem.width;
					dlItem.dirty = false;
					if ( availableWidth < 0 )
					{
						dlItem.width += availableWidth;
						availableWidth = 0;
						break;
					}
				}
			}
		}

		// Check width of normal items and count auto items
		int layoutAutoItemCount = 0;
		if ( availableWidth > 0 )
		{
			for ( int j = 0; j < elementCount; ++j )
			{
				DynamicLayoutItem& dlItem = dynamicLayoutData[j];
				if ( dlItem.type == LayoutAuto )
				{
					layoutAutoItemCount++;
				}
				else if ( dlItem.dirty )
				{
					dlItem.dirty = false;
					availableWidth -= dlItem.width;
					if ( availableWidth < 0 )
					{
						dlItem.width += availableWidth;
						availableWidth = 0;
						break;
					}
				}
			}
		}

		const qreal layoutAutoItemWidth = ( layoutAutoItemCount > 0 ) ? (availableWidth / (qreal)layoutAutoItemCount) : 0;
		for ( int j = 0; j < elementCount; ++j )
		{
			// Set auto items width
			DynamicLayoutItem& dlItem = dynamicLayoutData[j];
			if ( dlItem.dirty )
			{
				if ( availableWidth > 0 )
					dlItem.width = layoutAutoItemWidth;
				else
					dlItem.width = 0;
			}

			qreal itemWidth = dlItem.width;
			if ( itemWidth > 0 )
			{
				ContactList::LayoutItemConfigRowElement element = row.element( j );

				const int value = element.value();
				//const int role = ContactList::LayoutManager::instance()->token( value ).mModelRole;
				const int alignment = element.alignment();

				if ( painter )
					painter->setFont( dlItem.font );

				//special case for painting the ContactIcons...
				if ( value == ContactList::LayoutManager::ContactIcons )
				{
					QObject* metaContactObject = qVariantValue<QObject*>( index.data( Kopete::Items::ObjectRole ) );
					Kopete::MetaContact* metaContact = qobject_cast<Kopete::MetaContact*>(metaContactObject);

					QList<Kopete::Contact*> contactList = metaContact->contacts();
					if ( contactList.size() > 0 )
					{
						const qreal IconMarginH = 2.0;
						const qreal IconMarginV = 1.0;
						const qreal IconSize = rowHeight - 2 * IconMarginV;
						qreal iconsWidth = contactList.size() * IconSize;
						if ( contactList.size() > 1 )
							iconsWidth += (contactList.size() - 1) * IconMarginH;

						if (iconsWidth > itemWidth)
							iconsWidth = itemWidth;

						QRectF drawingRect( currentItemX, rowOffsetY + IconMarginV, iconsWidth, IconSize );
						if ( (alignment & Qt::AlignRight) == Qt::AlignRight )
							drawingRect.moveRight( currentItemX + itemWidth );
						else if ( (alignment & Qt::AlignHCenter) == Qt::AlignHCenter )
							drawingRect.moveLeft( currentItemX + (itemWidth - iconsWidth) / 2.0 );

						double offsetX = 0;
						foreach ( Kopete::Contact *contact, contactList )
						{
							QIcon contactIcon = contact->onlineStatus().iconFor( contact );
							if ( contactIcon.isNull() )
								continue;

							QRectF pixmapRect( drawingRect.x() + offsetX, drawingRect.y(),
							                   IconSize, IconSize );

							if ( contactPositionList )
								contactPositionList->append( QPair<QRect, Kopete::Contact*>( pixmapRect.toRect(), contact ) );

							if ( painter )
							{
								QPixmap contactPixmap = contactIcon.pixmap( IconSize, IconSize );
								painter->setClipRect( pixmapRect.intersected( drawingRect ) );
								painter->drawPixmap( pixmapRect, contactPixmap, QRectF( contactPixmap.rect() ) );
							}

							offsetX += IconSize + IconMarginH;
						}
					}
				}
				else if ( ContactList::LayoutManager::PlaceHolder )
				{
					// Do nothing
				}
				else
				{
					if ( painter )
					{
						QString text = QFontMetricsF( dlItem.font ).elidedText( dlItem.text, Qt::ElideRight, itemWidth );
						QRectF drawRect( currentItemX, rowOffsetY, itemWidth, rowHeight );
						painter->setClipRect( drawRect );
						painter->drawText( drawRect, alignment, text );
					}
				}

				currentItemX += itemWidth;
			}
		}
		rowOffsetY += rowHeight + PADDING;
	}
}

QPointF KopeteItemDelegate::centerImage( const QImage& image, const QRectF& rect ) const
{
	qreal imageRatio = ( qreal )image.width() / ( qreal )image.height();

	qreal moveByX = 0.0;
	qreal moveByY = 0.0;

	if ( imageRatio >= 1 )
		moveByY = ( rect.height() - ( rect.width() / imageRatio ) ) / 2.0;
	else
		moveByX = ( rect.width() - ( rect.height() * imageRatio ) ) / 2.0;

	return QPointF( moveByX, moveByY );
}

QPointF KopeteItemDelegate::centerImage( const QPixmap& pixmap, const QRectF& rect ) const
{
	qreal pixmapRatio = ( qreal )pixmap.width() / ( qreal )pixmap.height();
	
	qreal moveByX = 0.0;
	qreal moveByY = 0.0;
	
	if ( pixmapRatio >= 1 )
		moveByY = ( rect.height() - ( rect.width() / pixmapRatio ) ) / 2.0;
	else
		moveByX = ( rect.width() - ( rect.height() * pixmapRatio ) ) / 2.0;
	
	return QPointF( moveByX, moveByY );
}

qreal KopeteItemDelegate::calculateRowHeight( const ContactList::LayoutItemConfigRow &row, const QFont &normal, const QFont &small ) const
{
	qreal rowHeight = 0;

	const int elementCount = row.count();
	for ( int i = 0; i < elementCount; ++i )
	{
		ContactList::LayoutItemConfigRowElement element = row.element( i );
		QFont elementFont( ( element.small() ) ? small : normal );
		elementFont.setItalic( element.italic() );
		elementFont.setBold( element.bold() );
		rowHeight = qMax( rowHeight, QFontMetricsF( elementFont ).height() );
	}
	return rowHeight;
}
