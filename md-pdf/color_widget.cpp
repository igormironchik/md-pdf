
/*!
	\file

	\author Igor Mironchik (igor.mironchik at gmail dot com).

	Copyright (c) 2019 Igor Mironchik

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// md-pdf include.
#include "color_widget.hpp"

// Qt include.
#include <QPainter>


//
// ColorWidget
//

ColorWidget::ColorWidget( QWidget * parent )
	:	QFrame( parent )
{
	setFrameStyle( QFrame::Panel | QFrame::Sunken );
	setColor( Qt::white );
}

const QColor &
ColorWidget::color() const
{
	return m_color;
}

void
ColorWidget::setColor( const QColor & c )
{
	m_color = c;

	update();
}

void
ColorWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setPen( m_color );
	p.setBrush( m_color );
	p.drawRect( frameRect() );
}
