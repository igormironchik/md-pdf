
/*!
	\file

	\author Igor Mironchik (igor.mironchik at gmail dot com).

	Copyright (c) 2019-2024 Igor Mironchik

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
#include "podofo_paintdevice.hpp"

//
// PoDoFoPaintDevicePrivate
//

struct PoDoFoPaintDevicePrivate {
	PoDoFoPaintDevicePrivate( PoDoFoPaintDevice * parent )
		:	q( parent )
	{
	}

	//! Parent.
	PoDoFoPaintDevice * q = nullptr;
	//! Paint engine.
	PoDoFoPaintEngine engine;
}; // struct PoDoFoPaintDevicePrivate


//
// PoDoFoPaintDevice
//

PoDoFoPaintDevice::PoDoFoPaintDevice()
	:	d( new PoDoFoPaintDevicePrivate( this ) )
{
}

PoDoFoPaintDevice::~PoDoFoPaintDevice()
{
}

void
PoDoFoPaintDevice::setPage( PoDoFo::PdfPage & p, PoDoFo::PdfDocument & doc )
{
	d->engine.setPage( p, doc );
}

QPaintEngine *
PoDoFoPaintDevice::paintEngine() const
{
	return &d->engine;
}

int
PoDoFoPaintDevice::metric( QPaintDevice::PaintDeviceMetric metric ) const
{
	switch( metric )
	{
		case PdmWidth :
			return qRound( d->engine.page().GetRect().Width / 72.0 * 1200.0 );

        case PdmHeight :
			return qRound( d->engine.page().GetRect().Height / 72.0 * 1200.0 );

        case PdmWidthMM :
			return qRound( d->engine.page().GetRect().Width / 72.0 * 25.4 );

        case PdmHeightMM :
			return qRound( d->engine.page().GetRect().Height / 72.0 * 25.4 );

        case PdmNumColors :
			return INT_MAX;

        case PdmDepth :
			return 32;

        case PdmDpiX :
        case PdmDpiY :
        case PdmPhysicalDpiX :
        case PdmPhysicalDpiY :
			return 1200;

        case PdmDevicePixelRatio :
			return 1;

        case PdmDevicePixelRatioScaled :
			return 1 * QPaintDevice::devicePixelRatioFScale();
	}

	return -1;
}


//
// PoDoFoPaintEnginePrivate
//

struct PoDoFoPaintEnginePrivate {
	PoDoFoPaintEnginePrivate( PoDoFoPaintEngine * parent )
		:	q( parent )
	{
	}

	//! Parent.
	PoDoFoPaintEngine * q = nullptr;
	//! Pdf page.
	PoDoFo::PdfPage * page = nullptr;
	//! Pdf document.
	PoDoFo::PdfDocument * doc = nullptr;
	//! Pdf painter.
	std::shared_ptr< PoDoFo::PdfPainter > painter;
}; // struct PoDoFoPaintEnginePrivate


//
// PoDoFoPaintEngine
//

PoDoFoPaintEngine::PoDoFoPaintEngine()
	:	d( new PoDoFoPaintEnginePrivate( this ) )
{
}

PoDoFoPaintEngine::~PoDoFoPaintEngine()
{
}

void
PoDoFoPaintEngine::setPage( PoDoFo::PdfPage & p, PoDoFo::PdfDocument & doc )
{
	d->page = &p;
	d->doc = &doc;

	d->painter = std::make_shared< PoDoFo::PdfPainter > ();
	d->painter->SetCanvas( *d->page );
}

const PoDoFo::PdfPage &
PoDoFoPaintEngine::page() const
{
	return *d->page;
}

bool
PoDoFoPaintEngine::begin( QPaintDevice * )
{
	return true;
}

void
PoDoFoPaintEngine::drawEllipse( const QRectF & )
{
}

void
PoDoFoPaintEngine::drawEllipse( const QRect & )
{
}

void
PoDoFoPaintEngine::drawImage( const QRectF &, const QImage &,
	const QRectF &, Qt::ImageConversionFlags )
{
}

void
PoDoFoPaintEngine::drawLines( const QLineF * lines, int lineCount )
{
}

void
PoDoFoPaintEngine::drawLines( const QLine * lines, int lineCount )
{
}

void
PoDoFoPaintEngine::drawPath( const QPainterPath & path )
{
}

void
PoDoFoPaintEngine::drawPixmap( const QRectF &, const QPixmap &, const QRectF & )
{
}

void
PoDoFoPaintEngine::drawPoints( const QPointF *, int )
{
}

void
PoDoFoPaintEngine::drawPoints( const QPoint *, int )
{
}

void
PoDoFoPaintEngine::drawPolygon( const QPointF *, int,
	QPaintEngine::PolygonDrawMode )
{
}

void
PoDoFoPaintEngine::drawPolygon( const QPoint *, int,
	QPaintEngine::PolygonDrawMode )
{
}

void
PoDoFoPaintEngine::drawRects( const QRectF * rects, int rectCount )
{
}

void
PoDoFoPaintEngine::drawRects( const QRect * rects, int rectCount )
{
}

void
PoDoFoPaintEngine::drawTextItem( const QPointF & p, const QTextItem & textItem )
{
}

void
PoDoFoPaintEngine::drawTiledPixmap( const QRectF &, const QPixmap &,
	const QPointF & )
{
}

bool
PoDoFoPaintEngine::end()
{
	return true;
}

QPaintEngine::Type
PoDoFoPaintEngine::type() const
{
	return QPaintEngine::Pdf;
}

inline PoDoFo::PdfLineCapStyle
capStyle( Qt::PenCapStyle s )
{
	switch( s )
	{
		case Qt::SquareCap :
			return PoDoFo::PdfLineCapStyle::Square;

		case Qt::FlatCap :
			return PoDoFo::PdfLineCapStyle::Butt;

		case Qt::RoundCap :
		default :
			return PoDoFo::PdfLineCapStyle::Round;
	}
}

inline PoDoFo::PdfLineJoinStyle
joinStyle( Qt::PenJoinStyle s )
{
	switch( s )
	{
		case Qt::MiterJoin :
			return PoDoFo::PdfLineJoinStyle::Miter;

		case Qt::BevelJoin :
			return PoDoFo::PdfLineJoinStyle::Bevel;

		case Qt::RoundJoin :
		default :
			return PoDoFo::PdfLineJoinStyle::Round;
	}
}

inline PoDoFo::PdfColor
color( const QColor & c )
{
	return PoDoFo::PdfColor( c.redF(), c.greenF(), c.blueF() );
}

void
PoDoFoPaintEngine::updateState( const QPaintEngineState & state )
{
	const auto st = state.state();

	if( st & QPaintEngine::DirtyPen )
	{
		const auto p = state.pen();

		d->painter->GraphicsState.SetLineWidth( p.widthF() / paintDevice()->physicalDpiX() * 72.0 );
		d->painter->GraphicsState.SetLineCapStyle( capStyle( p.capStyle() ) );
		d->painter->GraphicsState.SetLineJoinStyle( joinStyle( p.joinStyle() ) );
		d->painter->GraphicsState.SetFillColor( color( p.brush().color() ) );
		d->painter->GraphicsState.SetStrokeColor( color( p.color() ) );
		d->painter->GraphicsState.SetMiterLevel( p.miterLimit() );
	}

	if( st & QPaintEngine::DirtyBrush )
		d->painter->GraphicsState.SetFillColor( color( state.brush().color() ) );

	if( st & QPaintEngine::DirtyFont )
	{
		const auto f = state.font();

		PoDoFo::PdfFontSearchParams params;
		params.Style = PoDoFo::PdfFontStyle::Regular;
		if( f.bold() ) params.Style.value() |= PoDoFo::PdfFontStyle::Bold;
		if( f.italic() ) params.Style.value() |= PoDoFo::PdfFontStyle::Italic;

		auto * font = d->doc->GetFonts().SearchFont( f.family().toLocal8Bit().data(), params );

		const double size = f.pointSizeF() > 0.0 ? f.pointSizeF() :
			f.pixelSize() / paintDevice()->physicalDpiY() * 72.0;

		d->painter->TextState.SetFont( *font, size );
	}

	if( st & QPaintEngine::DirtyTransform )
	{
		const auto t = state.transform();

		d->painter->GraphicsState.SetCurrentMatrix( PoDoFo::Matrix::FromCoefficients(
			t.m11(), t.m12(), t.m21(), t.m22(), t.m31(), t.m32() ) );
	}

	// if( st & QPaintEngine::DirtyClipRegion )
	// {
	// 	const auto r = state.clipRegion().boundingRect();

	// 	d->painter->SetClipRect( r.x() / paintDevice()->physicalDpiX() * 72.0,
	// 		( paintDevice()->height() - r.y() - r.height() ) / paintDevice()->physicalDpiY() * 72.0,
	// 		r.width() / paintDevice()->physicalDpiX() * 72.0,
	// 		r.height() / paintDevice()->physicalDpiY() * 72.0 );
	// }
}
