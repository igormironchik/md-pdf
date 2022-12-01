
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
#include "renderer.hpp"
#include "syntax.hpp"
#include "const.hpp"

#ifdef MD_PDF_TESTING
#include <test_const.hpp>
#include <QtTest/QtTest>
#endif // MD_PDF_TESTING

// Qt include.
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QThread>
#include <QBuffer>
#include <QTemporaryFile>
#include <QPainter>

// Magick++ include.
#include <Magick++.h>

// JKQtPlotter include.
#include <jkqtmathtext/jkqtmathtext.h>


//
// PdfRendererError
//

//! Internal exception.
class PdfRendererError {
public:
	explicit PdfRendererError( const QString & reason )
		:	m_what( reason )
	{
	}

	const QString & what() const noexcept
	{
		return m_what;
	}

private:
	QString m_what;
}; // class PdfRendererError


//
// PdfAuxData
//

double
PdfAuxData::topY( int page ) const
{
	if( !drawFootnotes )
		return coords.pageHeight - coords.margins.top;
	else
		return topFootnoteY( page );
}

int
PdfAuxData::currentPageIndex() const
{
	if( !drawFootnotes )
		return currentPageIdx;
	else
		return footnotePageIdx;
}

double
PdfAuxData::topFootnoteY( int page ) const
{
	if( reserved.contains( page ) )
		return reserved[ page ];
	else
		return 0.0;
}

double
PdfAuxData::currentPageAllowedY() const
{
	return allowedY( currentPageIdx );
}

double
PdfAuxData::allowedY( int page ) const
{
	if( !drawFootnotes )
	{
		if( reserved.contains( page ) )
			return reserved[ page ];
		else
			return coords.margins.bottom;
	}
	else
		return coords.margins.bottom;
}

void
PdfAuxData::freeSpaceOn( int page )
{
	if( !drawFootnotes )
	{
		if( reserved.contains( page ) )
		{
			double r = reserved[ page ];
			reserved.remove( page );

			if( page == footnotePageIdx )
				footnotePageIdx = page + 1;

			while( reserved.contains( ++page ) )
			{
				const double tmp = reserved[ page ];
				reserved[ page ] = r;
				r = tmp;
			}

			reserved[ page ] = r;
		}
	}
}

void
PdfAuxData::drawText( double x, double y, const PdfString & text )
{
	firstOnPage = false;

#ifndef MD_PDF_TESTING
	painter->DrawText( x, y, text );
#else
	if( printDrawings )
	{
		const auto s = PdfRenderer::createQString( text );

		(*drawingsStream) << QStringLiteral(
			"Text %1 \"%2\" %3 %4 0.0 0.0 0.0 0.0 0.0 0.0\n" )
				.arg( QString::number( s.length() ), s,
					QString::number( x, 'f', 16 ),
					QString::number( y, 'f', 16 ) );
	}
	else
	{
		painter->DrawText( x, y, text );

		if( QTest::currentTestFailed() )
			self->terminate();

		int pos = testPos++;
		QCOMPARE( DrawPrimitive::Type::Text, testData.at( pos ).type );
		QCOMPARE( PdfRenderer::createQString( text ), testData.at( pos ).text );
		QCOMPARE( x, testData.at( pos ).x );
		QCOMPARE( y, testData.at( pos ).y );
	}
#endif // MD_PDF_TESTING
}

void
PdfAuxData::drawMultiLineText( double x, double y, double width, double height,
	const PdfString & text )
{
	firstOnPage = false;

#ifndef MD_PDF_TESTING
	painter->DrawMultiLineText( x, y, width, height, text );
#else
	if( printDrawings )
	{
		const auto s = PdfRenderer::createQString( text );

		(*drawingsStream) << QStringLiteral(
			"MultilineText %1 \"%2\" %3 %4 0.0 0.0 %5 %6 0.0 0.0\n" )
				.arg( QString::number( s.length() ), s,
					QString::number( x, 'f', 16 ),
					QString::number( y, 'f', 16 ), QString::number( width, 'f', 16 ),
					QString::number( height, 'f', 16 ) );
	}
	else
	{
		painter->DrawMultiLineText( x, y, width, height, text );

		if( QTest::currentTestFailed() )
			self->terminate();

		int pos = testPos++;
		QCOMPARE( DrawPrimitive::Type::MultilineText, testData.at( pos ).type );
		QCOMPARE( PdfRenderer::createQString( text ), testData.at( pos ).text );
		QCOMPARE( x, testData.at( pos ).x );
		QCOMPARE( y, testData.at( pos ).y );
		QCOMPARE( width, testData.at( pos ).width );
		QCOMPARE( height, testData.at( pos ).height );
	}
#endif // MD_PDF_TESTING
}

void
PdfAuxData::drawImage( double x, double y, PdfImage * img, double xScale, double yScale )
{
	firstOnPage = false;

#ifndef MD_PDF_TESTING
	painter->DrawImage( x, y, img, xScale, yScale );
#else
	if( printDrawings )
		(*drawingsStream) << QStringLiteral(
			"Image 0 \"\" %2 %3 0.0 0.0 0.0 0.0 %4 %5\n" )
				.arg( QString::number( x, 'f', 16 ), QString::number( y, 'f', 16 ),
					QString::number( xScale, 'f', 16 ), QString::number( yScale, 'f', 16 ) );
	else
	{
		painter->DrawImage( x, y, img, xScale, yScale );

		if( QTest::currentTestFailed() )
			self->terminate();

		int pos = testPos++;
		QCOMPARE( x, testData.at( pos ).x );
		QCOMPARE( y, testData.at( pos ).y );
		QCOMPARE( xScale, testData.at( pos ).xScale );
		QCOMPARE( yScale, testData.at( pos ).yScale );
	}
#endif // MD_PDF_TESTING
}

void
PdfAuxData::drawLine( double x1, double y1, double x2, double y2 )
{
#ifndef MD_PDF_TESTING
	painter->DrawLine( x1, y1, x2, y2 );
#else
	if( printDrawings )
		(*drawingsStream) << QStringLiteral(
			"Line 0 \"\" %1 %2 %3 %4 0.0 0.0 0.0 0.0\n" )
				.arg( QString::number( x1, 'f', 16 ), QString::number( y1, 'f', 16 ),
					QString::number( x2, 'f', 16 ), QString::number( y2, 'f', 16 ) );
	else
	{
		painter->DrawLine( x1, y1, x2, y2 );

		if( QTest::currentTestFailed() )
			self->terminate();

		int pos = testPos++;
		QCOMPARE( x1, testData.at( pos ).x );
		QCOMPARE( y1, testData.at( pos ).y );
		QCOMPARE( x2, testData.at( pos ).x2 );
		QCOMPARE( y2, testData.at( pos ).y2 );
	}
#endif // MD_PDF_TESTING
}

void
PdfAuxData::save( const QString & fileName )
{
#ifndef MD_PDF_TESTING
	doc->Write( fileName.toLocal8Bit().data() );
#else
	if( !printDrawings )
		doc->Write( fileName.toLocal8Bit().data() );
#endif // MD_PDF_TESTING
}

void
PdfAuxData::drawRectangle( double x, double y, double width, double height )
{
#ifndef MD_PDF_TESTING
	painter->Rectangle( x, y, width, height );
#else
	if( printDrawings )
		(*drawingsStream) << QStringLiteral(
			"Rectangle 0 \"\" %1 %2 0.0 0.0 %3 %4 0.0 0.0\n" )
				.arg( QString::number( x, 'f', 16 ), QString::number( y, 'f', 16 ),
					QString::number( width, 'f', 16 ), QString::number( height, 'f', 16 ) );
	else
	{
		painter->Rectangle( x, y, width, height );

		if( QTest::currentTestFailed() )
			self->terminate();

		int pos = testPos++;
		QCOMPARE( x, testData.at( pos ).x );
		QCOMPARE( y, testData.at( pos ).y );
		QCOMPARE( width, testData.at( pos ).width );
		QCOMPARE( height, testData.at( pos ).height );
	}
#endif // MD_PDF_TESTING
}

void
PdfAuxData::setColor( const QColor & c )
{
	m_colorsStack.push( c );

	painter->SetColor( c.redF(), c.greenF(), c.blueF() );
}

void
PdfAuxData::restoreColor()
{
	if( m_colorsStack.size() > 1 )
		m_colorsStack.pop();

	repeatColor();
}

void
PdfAuxData::repeatColor()
{
	const auto & c = m_colorsStack.top();

	painter->SetColor( c.redF(), c.greenF(), c.blueF() );
}


//
// PdfRenderer::CustomWidth
//

double
PdfRenderer::CustomWidth::firstItemHeight() const
{
	if( !m_width.isEmpty() )
		return m_width.constFirst().height;
	else
		return 0.0;
}

void
PdfRenderer::CustomWidth::calcScale( double lineWidth )
{
	double w = 0.0;
	double sw = 0.0;
	double ww = 0.0;
	double h = 0.0;
	double d = 0.0;

	for( int i = 0, last = m_width.size(); i < last; ++i )
	{
		if( m_width.at( i ).height > h )
		{
			h = m_width.at( i ).height;
			d = m_width.at( i ).descent;
		}

		w += m_width.at( i ).width;

		if( m_width.at( i ).isSpace )
			sw += m_width.at( i ).width;
		else
			ww += m_width.at( i ).width;

		if( m_width.at( i ).isNewLine )
		{
			if( m_width.at( i ).shrink )
			{
				auto ss = ( lineWidth - ww ) / sw;

				while( ww + sw * ss > lineWidth )
					ss -= 0.001;

				m_scale.append( 100.0 * ss );
			}
			else
				m_scale.append( 100.0 );

			m_height.append( h );
			m_descent.append( d );

			w = 0.0;
			sw = 0.0;
			ww = 0.0;
			h = 0.0;
			d = 0.0;
		}
	}
}

double
PdfRenderer::CustomWidth::totalHeight() const
{
	double h = 0.0;
	double max = 0.0;

	for( int i = 0, last = m_width.size(); i < last; ++i )
	{
		if( m_width.at( i ).height > max )
			max = m_width.at( i ).height;

		if( m_width.at( i ).isNewLine )
		{
			h += max;
			max = 0.0;
		}
	}

	return h;
}


//
// PdfRenderer::CellItem
//

double
PdfRenderer::CellItem::width( PdfAuxData & pdfData, PdfRenderer * render, float scale ) const
{
	auto * f = render->createFont( font.family, font.bold, font.italic,
		font.size, pdfData.doc, scale, pdfData );
	f->SetStrikeOut( font.strikethrough );

	if( !word.isEmpty() )
		return f->GetFontMetrics()->StringWidth( createPdfString( word ) );
	else if( !image.isNull() )
	{
		PdfImage pdfImg( pdfData.doc );
		pdfImg.SetDpi( pdfData.m_dpi );
		pdfImg.LoadFromData( reinterpret_cast< const unsigned char * >( image.data() ),
			image.size() );

		return pdfImg.GetWidth();
	}
	else if( !url.isEmpty() )
		return f->GetFontMetrics()->StringWidth( createPdfString( url ) );
	else if( !footnote.isEmpty() )
	{
		const auto old = f->GetFontSize();
		f->SetFontSize( old * c_footnoteScale );
		const auto w = f->GetFontMetrics()->StringWidth( createPdfString(
			footnote ) );
		f->SetFontSize( old );

		return w;
	}
	else
		return 0.0;
}


//
// PdfRenderer::CellData
//

void
PdfRenderer::CellData::heightToWidth( double lineHeight, double spaceWidth, float scale,
	PdfAuxData & pdfData, PdfRenderer * render )
{
	height = 0.0;

	bool newLine = true;

	double w = 0.0;

	for( auto it = items.cbegin(), last = items.cend(); it != last; ++it )
	{
		if( it->image.isNull() )
		{
			if( newLine )
			{
				height += lineHeight;
				newLine = false;
				w = 0.0;
			}

			w += it->width( pdfData, render, scale );

			if( w >= width )
			{
				newLine = true;
				continue;
			}

			double sw = spaceWidth;

			if( it != items.cbegin() && it->font != ( it - 1 )->font )
			{
				auto * f = render->createFont( it->font.family, it->font.bold, it->font.italic,
					it->font.size, pdfData.doc, scale, pdfData );
				f->SetStrikeOut( it->font.strikethrough );
				sw = f->GetFontMetrics()->StringWidth( PdfString( " " ) );
			}

			if( it + 1 != last && !( it + 1 )->footnote.isEmpty() )
				sw = 0.0;

			if( it + 1 != last )
			{
				if( w + sw + ( it + 1 )->width( pdfData, render, scale ) > width )
					newLine = true;
				else
				{
					w += sw;
					newLine = false;
				}
			}
		}
		else
		{
			PdfImage pdfImg( pdfData.doc );
			pdfImg.SetDpi( pdfData.m_dpi );
			pdfImg.LoadFromData( reinterpret_cast< const unsigned char * >( it->image.data() ),
				it->image.size() );

			height += pdfImg.GetHeight() / ( pdfImg.GetWidth() / width ) * scale;
			newLine = true;
		}
	}
}


//
// PdfRenderer
//

PdfRenderer::PdfRenderer()
	:	m_terminate( false )
	,	m_footnoteNum( 1 )
#ifdef MD_PDF_TESTING
	,	m_isError( false )
#endif
{
	connect( this, &PdfRenderer::start, this, &PdfRenderer::renderImpl,
		Qt::QueuedConnection );
}

void
PdfRenderer::render( const QString & fileName, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
	const RenderOpts & opts, bool testing )
{
	m_fileName = fileName;
	m_doc = doc;
	m_opts = opts;

	if( !testing )
		emit start();
}

void
PdfRenderer::terminate()
{
	QMutexLocker lock( &m_mutex );

	m_terminate = true;

#ifdef MD_PDF_TESTING
	QFAIL( "Test terminated." );
#endif
}

#ifdef MD_PDF_TESTING
bool
PdfRenderer::isError() const
{
	return m_isError;
}
#endif

void
PdfRenderer::renderImpl()
{
	{
		const int itemsCount = m_doc->items().size();

		emit progress( 0 );

		PdfMemDocument document;

		PdfPainter painter;

		PdfAuxData pdfData;

		pdfData.doc = &document;
		pdfData.painter = &painter;

		pdfData.coords.margins.left = m_opts.m_left;
		pdfData.coords.margins.right = m_opts.m_right;
		pdfData.coords.margins.top = m_opts.m_top;
		pdfData.coords.margins.bottom = m_opts.m_bottom;
		pdfData.m_dpi = m_opts.m_dpi;

		pdfData.m_colorsStack.push( Qt::black );

#ifdef MD_PDF_TESTING
		pdfData.fonts[ QStringLiteral( "Droid Serif" ) ] = c_font;
		pdfData.fonts[ QStringLiteral( "Droid Serif Bold" ) ] = c_boldFont;
		pdfData.fonts[ QStringLiteral( "Droid Serif Italic" ) ] = c_italicFont;
		pdfData.fonts[ QStringLiteral( "Droid Serif Bold Italic" ) ] = c_boldItalicFont;
		pdfData.fonts[ QStringLiteral( "Courier New" ) ] = c_monoFont;

		pdfData.self = this;

		if( m_opts.printDrawings )
		{
			pdfData.printDrawings = true;

			pdfData.drawingsFile.reset( new QFile( m_opts.testDataFileName ) );
			if( !pdfData.drawingsFile->open( QIODevice::WriteOnly ) )
				QFAIL( "Unable to open file for dump drawings." );

			pdfData.drawingsStream.reset( new QTextStream( pdfData.drawingsFile.get() ) );
		}
		else
			pdfData.testData = m_opts.testData;
#endif // MD_PDF_TESTING

		try {
			int itemIdx = 0;

			{
				auto * font = createFont( m_opts.m_textFont, false, false, m_opts.m_textFontSize,
					pdfData.doc, 1.0, pdfData );
				pdfData.extraInFootnote = font->GetFontMetrics()->GetLineSpacing() / 3.0;
			}

			createPage( pdfData );

			for( auto it = m_doc->items().cbegin(), last = m_doc->items().cend(); it != last; ++it )
			{
				++itemIdx;

				{
					QMutexLocker lock( &m_mutex );

					if( m_terminate )
						break;
				}

				switch( (*it)->type() )
				{
					case MD::ItemType::Heading :
						drawHeading( pdfData, m_opts, static_cast< MD::Heading< MD::QStringTrait >* > ( it->get() ),
							m_doc, 0.0,
							// If there is another item after heading we need to know its min
							// height to glue heading with it.
							( it + 1 != last ?
								minNecessaryHeight( pdfData, m_opts, *( it + 1 ), m_doc, 0.0,
									1.0, false ) :
								0.0 ), CalcHeightOpt::Unknown, 1.0 );
						break;

					case MD::ItemType::Paragraph :
						drawParagraph( pdfData, m_opts, static_cast< MD::Paragraph< MD::QStringTrait >* > ( it->get() ),
							m_doc, 0.0, true, CalcHeightOpt::Unknown, 1.0, false );
						break;

					case MD::ItemType::Code :
						drawCode( pdfData, m_opts, static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
							m_doc, 0.0, CalcHeightOpt::Unknown, 1.0 );
						break;

					case MD::ItemType::Blockquote :
						drawBlockquote( pdfData, m_opts,
							static_cast< MD::Blockquote< MD::QStringTrait >* > ( it->get() ),
							m_doc, 0.0, CalcHeightOpt::Unknown, 1.0, false );
						break;

					case MD::ItemType::List :
					{
						auto * list = static_cast< MD::List< MD::QStringTrait >* > ( it->get() );
						const auto bulletWidth = maxListNumberWidth( list );

						drawList( pdfData, m_opts, list, m_doc, bulletWidth );
					}
						break;

					case MD::ItemType::Table :
						drawTable( pdfData, m_opts,
							static_cast< MD::Table< MD::QStringTrait >* > ( it->get() ),
							m_doc, 0.0, CalcHeightOpt::Unknown, 1.0, false );
						break;

					case MD::ItemType::PageBreak :
					{
						if( itemIdx < itemsCount )
							createPage( pdfData );
					}
						break;

					case MD::ItemType::Anchor :
					{
						auto * a = static_cast< MD::Anchor< MD::QStringTrait >* > ( it->get() );
						m_dests.insert( a->label(), PdfDestination( pdfData.page ) );
					}
						break;

					default :
						break;
				}

				emit progress( static_cast< int > ( static_cast< double > (itemIdx) /
					static_cast< double > (itemsCount) * 100.0 ) );
			}

			if( !m_footnotes.isEmpty() )
			{
				pdfData.drawFootnotes = true;
				pdfData.coords.x = pdfData.coords.margins.left;
				pdfData.coords.y = pdfData.topFootnoteY( pdfData.reserved.firstKey() ) -
					pdfData.extraInFootnote;

				pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.reserved.firstKey() ) );

				pdfData.footnotePageIdx = pdfData.reserved.firstKey();

				drawHorizontalLine( pdfData, m_opts );

				for( const auto & f : qAsConst( m_footnotes ) )
					drawFootnote( pdfData, m_opts, m_doc, f.get(), CalcHeightOpt::Unknown );
			}

			resolveLinks( pdfData );

			finishPages( pdfData );

			pdfData.save( m_fileName );

			emit done( m_terminate );
		}
		catch( const PdfError & e )
		{
			try {
				finishPages( pdfData );
				pdfData.save( m_fileName );
			}
			catch( ... )
			{
			}

#ifdef MD_PDF_TESTING
			m_isError = true;
#endif
			emit error( QString::fromLatin1( PdfError::ErrorMessage( e.GetError() ) ) );
		}
		catch( const PdfRendererError & e )
		{
			try {
				finishPages( pdfData );
				pdfData.save( m_fileName );
			}
			catch( ... )
			{
			}

#ifdef MD_PDF_TESTING
			m_isError = true;
#endif

			emit error( e.what() );
		}

#ifdef MD_PDF_TESTING
		if( m_opts.printDrawings )
			pdfData.drawingsFile->close();

		if( pdfData.testPos != pdfData.testData.size() )
			m_isError = true;
#endif // MD_PDF_TESTING
	}

	try {
		clean();
	}
	catch( const PdfError & e )
	{
#ifdef MD_PDF_TESTING
		m_isError = true;
#endif
		emit error( QString::fromLatin1( PdfError::ErrorMessage( e.GetError() ) ) );
	}

	deleteLater();
}

void
PdfRenderer::finishPages( PdfAuxData & pdfData )
{
	for( int i = 0; i <= pdfData.currentPageIdx; ++i )
	{
		pdfData.painter->SetPage( pdfData.doc->GetPage( i ) );
		pdfData.painter->FinishPage();
	}
}

void
PdfRenderer::clean()
{
	m_dests.clear();
	m_unresolvedLinks.clear();
	PdfEncodingFactory::FreeGlobalEncodingInstances();
}

double
PdfRenderer::rowHeight( const QVector< QVector< CellData > > & table, int row )
{
	double h = 0.0;

	for( auto it = table.cbegin(), last = table.cend(); it != last; ++it )
	{
		if( (*it)[ row ].height > h )
			h = (*it)[ row ].height;
	}

	return  h;
}

void
PdfRenderer::resolveLinks( PdfAuxData & pdfData )
{
	for( auto it = m_unresolvedLinks.cbegin(), last = m_unresolvedLinks.cend(); it != last; ++it )
	{
		if( m_dests.contains( it.key() ) )
		{
			for( const auto & r : qAsConst( it.value() ) )
			{
				auto * page = pdfData.doc->GetPage( r.second );
				auto * annot = page->CreateAnnotation( ePdfAnnotation_Link,
					PdfRect( r.first.x(), r.first.y(), r.first.width(), r.first.height() ) );
				annot->SetBorderStyle( 0.0, 0.0, 0.0 );
				annot->SetDestination( m_dests.value( it.key(), PdfDestination( pdfData.page ) ) );
			}
		}
#ifdef MD_PDF_TESTING
		else
		{
			terminate();

			QFAIL( "Unresolved link." );
		}
#endif // MD_PDF_TESTING
	}
}

PdfFont *
PdfRenderer::createFont( const QString & name, bool bold, bool italic, float size,
	PdfMemDocument * doc, float scale, const PdfAuxData & pdfData )
{
#ifdef MD_PDF_TESTING
	const QString internalName = name + ( bold ? QStringLiteral( " Bold" ) : QString() ) +
		( italic ? QStringLiteral( " Italic" ) : QString() );

	auto * font = doc->CreateFont( name.toLocal8Bit().data(), bold, italic , false,
		PdfEncodingFactory::GlobalIdentityEncodingInstance(),
		PdfFontCache::eFontCreationFlags_None, true,
		pdfData.fonts[ internalName ].toLocal8Bit().data() );
#else
	Q_UNUSED( pdfData )

	auto * font = doc->CreateFont( name.toLocal8Bit().data(), bold, italic , false,
		PdfEncodingFactory::GlobalIdentityEncodingInstance(),
		PdfFontCache::eFontCreationFlags_None );
#endif // MD_PDF_TESTING

	if( !font )
		throw PdfRendererError( tr( "Unable to create font: %1. Please choose another one.\n\n"
			"This application uses PoDoFo C++ library to create PDF. And not all fonts supported by Qt "
			"are supported by PoDoFo. I'm sorry for the inconvenience." )
				.arg( name ) );

	font->SetFontSize( size * scale );

	return font;
}

bool
PdfRenderer::isFontCreatable( const QString & name )
{
	PdfMemDocument doc;

	auto * font = doc.CreateFont( name.toLocal8Bit().data(), false, false , false,
		PdfEncodingFactory::GlobalIdentityEncodingInstance(),
		PdfFontCache::eFontCreationFlags_None );

	return ( font != nullptr );
}

void
PdfRenderer::createPage( PdfAuxData & pdfData )
{
	auto create = [] ( PdfAuxData & pdfData )
	{
		pdfData.page = pdfData.doc->CreatePage(
			PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );

		if( !pdfData.page )
			throw PdfRendererError( QLatin1String( "Oops, can't create empty page in PDF.\n\n"
				"This is very strange, it should not appear ever, but it is. "
				"I'm sorry for the inconvenience." ) );

		pdfData.firstOnPage = true;

		pdfData.painter->SetPage( pdfData.page );

		pdfData.coords = { { pdfData.coords.margins.left, pdfData.coords.margins.right,
				pdfData.coords.margins.top, pdfData.coords.margins.bottom },
			pdfData.page->GetPageSize().GetWidth(),
			pdfData.page->GetPageSize().GetHeight(),
			pdfData.coords.margins.left, pdfData.page->GetPageSize().GetHeight() -
				pdfData.coords.margins.top };

		++pdfData.currentPageIdx;
	};

	if( !pdfData.drawFootnotes )
		create( pdfData );
	else
	{
		auto it = pdfData.reserved.find( ++pdfData.footnotePageIdx );

		if( it == pdfData.reserved.cend() )
			it = pdfData.reserved.upperBound( pdfData.footnotePageIdx );

		if( it != pdfData.reserved.cend() )
			pdfData.footnotePageIdx = it.key();
		else
			pdfData.footnotePageIdx = pdfData.currentPageIdx + 1;

		if( pdfData.footnotePageIdx <= pdfData.currentPageIdx )
		{
			pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.footnotePageIdx ) );
			pdfData.coords.x = pdfData.coords.margins.left;
			pdfData.coords.y = pdfData.topFootnoteY( pdfData.footnotePageIdx );
		}
		else
		{
			create( pdfData );

			pdfData.coords.y = pdfData.topFootnoteY( pdfData.footnotePageIdx );
		}

		pdfData.coords.y -= pdfData.extraInFootnote;

		drawHorizontalLine( pdfData, m_opts );

		if( pdfData.continueParagraph )
			pdfData.coords.y -= pdfData.lineHeight;
	}

	if( pdfData.m_colorsStack.size() > 1 )
		pdfData.repeatColor();
}

void
PdfRenderer::drawHorizontalLine( PdfAuxData & pdfData, const RenderOpts & renderOpts )
{
	pdfData.setColor( renderOpts.m_borderColor );
	pdfData.drawLine( pdfData.coords.margins.left, pdfData.coords.y,
		pdfData.coords.pageWidth - pdfData.coords.margins.right,
		pdfData.coords.y );
	pdfData.restoreColor();
}

PdfString
PdfRenderer::createPdfString( const QString & text )
{
	return PdfString( reinterpret_cast< pdf_utf8* > ( text.toUtf8().data() ) );
}

QString
PdfRenderer::createQString( const PdfString & str )
{
	return QString::fromUtf8( str.GetStringUtf8().c_str(),
		static_cast< int > ( str.GetCharacterLength() ) );
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Heading< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
	double nextItemMinHeight, CalcHeightOpt heightCalcOpt, float scale, bool withNewLine )
{
	if( item && item->text().get() )
	{
		const auto where = drawParagraph( pdfData, renderOpts, item->text().get(), doc,
			offset, withNewLine, heightCalcOpt,
			scale * ( 1.0 + ( 7 - item->level() ) * 0.25 ),
			pdfData.drawFootnotes );

		if( heightCalcOpt == CalcHeightOpt::Unknown && !item->label().isEmpty() &&
				!where.first.isEmpty() )
		{
			m_dests.insert( item->label(),
				PdfDestination( pdfData.doc->GetPage( where.first.front().pageIdx ),
					PdfRect( pdfData.coords.margins.left + offset, where.first.front().y,
						 pdfData.coords.pageWidth - pdfData.coords.margins.left -
						 pdfData.coords.margins.right - offset, where.first.front().height ) ) );
		}

		return where;
	}
	else
		return {};
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Text< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine,
	PdfFont * footnoteFont, float footnoteFontScale, MD::Item< MD::QStringTrait > * nextItem, int footnoteNum,
	double offset, bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote )
{
	auto * spaceFont = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

	auto * font = createFont( renderOpts.m_textFont, item->opts() & MD::TextOption::BoldText,
		item->opts() & MD::TextOption::ItalicText,
		renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

	if( item->opts() & MD::TextOption::StrikethroughText )
		font->SetStrikeOut( true );
	else
		font->SetStrikeOut( false );

	return drawString( pdfData, renderOpts, item->text(),
		spaceFont, font, font->GetFontMetrics()->GetLineSpacing(),
		doc, newLine, footnoteFont, footnoteFontScale, nextItem, footnoteNum, offset,
		firstInParagraph, cw, QColor(), inFootnote );
}

namespace /* anonymous */ {

//! Combine smaller rectangles standing next each other to bigger one.
QVector< QPair< QRectF, int > >
normalizeRects( const QVector< QPair< QRectF, int > > & rects )
{
	QVector< QPair< QRectF, int > > ret;

	if( !rects.isEmpty() )
	{
		QPair< QRectF, int > to( rects.first() );

		auto it = rects.cbegin();
		++it;

		for( auto last = rects.cend(); it != last; ++it )
		{
			if( qAbs( it->first.y() - to.first.y() ) < 0.001 )
				to.first.setWidth( to.first.width() + it->first.width() );
			else
			{
				ret.append( to );

				to = *it;
			}
		}

		ret.append( to );
	}

	return ret;
}

} /* namespace anonymous */

QVector< QPair< QRectF, int > >
PdfRenderer::drawLink( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Link< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine,
	PdfFont * footnoteFont, float footnoteFontScale, MD::Item< MD::QStringTrait > * nextItem, int footnoteNum,
	double offset, bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote )
{
	QVector< QPair< QRectF, int > > rects;

	QString url = item->url();

	const auto lit = doc->labeledLinks().find( url );

	if( lit != doc->labeledLinks().cend() )
		url = lit->second->url();

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

	// If text link.
	if( item->img()->isEmpty() )
	{
		pdfData.setColor( renderOpts.m_linkColor );

		auto * font = createFont( renderOpts.m_textFont, item->opts() & MD::TextOption::BoldText,
			item->opts() & MD::TextOption::ItalicText, renderOpts.m_textFontSize,
			pdfData.doc, scale, pdfData );

		if( item->opts() & MD::TextOption::StrikethroughText )
			font->SetStrikeOut( true );
		else
			font->SetStrikeOut( false );

		if( !item->p()->isEmpty() )
		{
			for( auto it = item->p()->items().begin(), last = item->p()->items().end();
				it != last; ++it )
			{
				switch( (*it)->type() )
				{
					case MD::ItemType::Text :
					{
						auto * text = std::static_pointer_cast< MD::Text< MD::QStringTrait > >( *it ).get();

						auto * spaceFont = createFont( renderOpts.m_textFont, false, false,
							renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

						auto * font = createFont( renderOpts.m_textFont,
							text->opts() & MD::BoldText || item->opts() & MD::BoldText,
							text->opts() & MD::ItalicText || item->opts() & MD::ItalicText,
							renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

						if( text->opts() & MD::StrikethroughText ||
							item->opts() & MD::StrikethroughText )
								font->SetStrikeOut( true );
						else
							font->SetStrikeOut( false );

						rects.append( drawString( pdfData, renderOpts,
							text->text(), spaceFont, font, font->GetFontMetrics()->GetLineSpacing(),
							doc, newLine, footnoteFont, footnoteFontScale,
							( it == std::prev( last ) ? nextItem : nullptr ), footnoteNum, offset,
							( it == item->p()->items().begin() && firstInParagraph ),
							cw, QColor(), inFootnote ) );
					}
						break;

					case MD::ItemType::Code :
						rects.append( drawInlinedCode( pdfData, renderOpts,
							static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
							doc, newLine, offset,
							( it == item->p()->items().begin() && firstInParagraph ), cw, scale,
							inFootnote ) );
						break;

					default :
						break;
				}
			}
		}
		else
			rects = drawString( pdfData, renderOpts,
				url, createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
					pdfData.doc, scale, pdfData ),
				font, font->GetFontMetrics()->GetLineSpacing(),
				doc, newLine, footnoteFont, footnoteFontScale, nextItem, footnoteNum, offset,
				firstInParagraph, cw, QColor(), inFootnote );

		pdfData.restoreColor();
	}
	// Otherwise image link.
	else
		rects.append( drawImage( pdfData, renderOpts, item->img().get(), doc, newLine, offset,
			firstInParagraph, cw, scale ) );

	rects = normalizeRects( rects );

	if( draw )
	{
		// If Web URL.
		if( !QUrl( url ).isRelative() )
		{
			for( const auto & r : qAsConst( rects ) )
			{
				auto * annot = pdfData.doc->GetPage( r.second )->CreateAnnotation( ePdfAnnotation_Link,
					PdfRect( r.first.x(), r.first.y(), r.first.width(), r.first.height() ) );
				annot->SetBorderStyle( 0.0, 0.0, 0.0 );

				PdfAction action( ePdfAction_URI, pdfData.doc );
				action.SetURI( PdfString( url.toLatin1().data() ) );

				annot->SetAction( action );
			}
		}
		// Otherwise internal link.
		else
			m_unresolvedLinks.insert( url, rects );
	}

	return rects;
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawString( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	const QString & str, PdfFont * spaceFont, PdfFont * font, double lineHeight,
	std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine, PdfFont * footnoteFont,
	float footnoteFontScale, MD::Item< MD::QStringTrait > * nextItem, int footnoteNum, double offset,
	bool firstInParagraph, CustomWidth * cw, const QColor & background, bool inFootnote )
{
	Q_UNUSED( doc )
	Q_UNUSED( renderOpts )

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

	bool footnoteAtEnd = false;
	double footnoteWidth = 0.0;

	if( nextItem && nextItem->type() == MD::ItemType::FootnoteRef &&
		doc->footnotesMap().find( static_cast< MD::FootnoteRef< MD::QStringTrait >* >(
			nextItem )->id() ) != doc->footnotesMap().cend() && !inFootnote )
				footnoteAtEnd = true;

	if( footnoteAtEnd )
	{
		const auto old = footnoteFont->GetFontSize();
		footnoteFont->SetFontSize( old * footnoteFontScale );
		footnoteWidth = footnoteFont->GetFontMetrics()->StringWidth( createPdfString(
			QString::number( footnoteNum ) ) );
		footnoteFont->SetFontSize( old );
	}

	double h = lineHeight;
	double descent = 0.0;
	double ascent = font->GetFontMetrics()->GetAscent();
	double d = 0.0;

	if( cw && cw->isDrawing() )
	{
		h = cw->height();
		descent = cw->descent();
		d = ( h - ascent ) / 2.0;
	}

	auto newLineFn = [&] ()
	{
		newLine = true;

		if( draw )
		{
			if( cw )
				cw->moveToNextLine();

			moveToNewLine( pdfData, offset, cw->height(), 1.0 );

			h = cw->height();
			descent = cw->descent();
			d = ( h - ascent ) / 2.0;
		}
		else if( cw )
		{
			cw->append( { 0.0, lineHeight, 0.0, false, true, true, "" } );
			pdfData.coords.x = pdfData.coords.margins.left + offset;
		}
	};

	QVector< QPair< QRectF, int > > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	static const QString charsWithoutSpaceBefore = QLatin1String( ".,;" );

	const auto words = str.split( QLatin1Char( ' ' ), Qt::SkipEmptyParts );

	const auto wv = pdfData.coords.pageWidth - pdfData.coords.margins.right;

	// We need to draw space char if first word is a word.
	if( !firstInParagraph && !newLine && !words.isEmpty() &&
		!charsWithoutSpaceBefore.contains( words.first() ) )
	{
		const auto strike = spaceFont->IsStrikeOut();
		spaceFont->SetStrikeOut( false );
		pdfData.painter->SetFont( spaceFont );

		const auto w = spaceFont->GetFontMetrics()->StringWidth( " " );

		auto scale = 100.0;

		if( draw && cw )
			scale = cw->scale();

		const auto xv = pdfData.coords.x + w * scale / 100.0 + font->GetFontMetrics()->StringWidth(
				createPdfString( words.first() ) ) +
			( words.size() == 1 && footnoteAtEnd ? footnoteWidth : 0.0 );

		if( xv < wv || qAbs( xv - wv ) < 0.01 )
		{
			if( draw )
			{
				spaceFont->SetFontScale( scale );

				pdfData.drawText( pdfData.coords.x,
					pdfData.coords.y + d, " " );

				spaceFont->SetFontScale( 100.0 );
			}
			else if( cw )
				cw->append( { w, lineHeight, 0.0, true, false, true, " " } );

			ret.append( qMakePair( QRectF( pdfData.coords.x,
				pdfData.coords.y + d,
				w * scale / 100.0, lineHeight ), pdfData.currentPageIndex() ) );

			pdfData.coords.x += w * scale / 100.0;
		}
		else
			newLineFn();

		spaceFont->SetStrikeOut( strike );
	}

	pdfData.painter->SetFont( font );

	// Draw words.
	for( auto it = words.begin(), last = words.end(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return ret;
		}

		const auto str = createPdfString( *it );

		const auto length = font->GetFontMetrics()->StringWidth( str );

		const auto xv = pdfData.coords.x + length +
			( it + 1 == last && footnoteAtEnd ? footnoteWidth : 0.0 );

		if( xv < wv || qAbs( xv - wv ) < 0.01 )
		{
			newLine = false;

			if( draw )
			{
				if( background.isValid() )
				{
					pdfData.setColor( background );
					pdfData.drawRectangle( pdfData.coords.x, pdfData.coords.y +
						font->GetFontMetrics()->GetDescent() + d, length,
						font->GetFontMetrics()->GetLineSpacing() );
					pdfData.painter->Fill();
					pdfData.restoreColor();
				}

				pdfData.drawText( pdfData.coords.x,
					pdfData.coords.y + d, str );
				ret.append( qMakePair( QRectF( pdfData.coords.x,
					pdfData.coords.y + d,
					length, lineHeight ), pdfData.currentPageIndex() ) );
			}
			else if( cw )
				cw->append( { length + ( it + 1 == last && footnoteAtEnd ? footnoteWidth : 0.0 ),
					lineHeight, 0.0, false, false, true, *it } );

			pdfData.coords.x += length;

			// Draw space if needed.
			if( it + 1 != last )
			{
				const auto spaceWidth = font->GetFontMetrics()->StringWidth( " " );
				const auto nextLength = font->GetFontMetrics()->StringWidth( createPdfString(
					*( it + 1 ) ) ) + ( it + 2 == last && footnoteAtEnd ? footnoteWidth : 0.0 );

				auto scale = 100.0;

				if( draw && cw )
					scale = cw->scale();

				const auto xv = pdfData.coords.x + spaceWidth * scale / 100.0 + nextLength;

				if( xv < wv || qAbs( xv - wv ) < 0.01 )
				{
					if( draw )
					{
						font->SetFontScale( scale );

						ret.append( qMakePair( QRectF( pdfData.coords.x,
							pdfData.coords.y + d,
							spaceWidth * scale / 100.0, lineHeight ), pdfData.currentPageIndex() ) );

						if( background.isValid() )
						{
							pdfData.setColor( background );
							pdfData.drawRectangle( pdfData.coords.x, pdfData.coords.y +
								font->GetFontMetrics()->GetDescent() +
								d, spaceWidth * scale / 100.0,
								font->GetFontMetrics()->GetLineSpacing() );
							pdfData.painter->Fill();
							pdfData.restoreColor();
						}

						pdfData.drawText( pdfData.coords.x,
							pdfData.coords.y + d, " " );

						font->SetFontScale( 100.0 );
					}
					else if( cw )
						cw->append( { spaceWidth, lineHeight, 0.0, true, false, true, " " } );

					pdfData.coords.x += spaceWidth * scale / 100.0;
				}
				else
					newLineFn();
			}
		}
		// Need to move to new line.
		else
		{
			const auto xv = pdfData.coords.margins.left + offset + length +
				( it + 1 == last && footnoteAtEnd ? footnoteWidth : 0.0 );

			if( xv < wv || qAbs( xv - wv ) < 0.01 )
			{
				newLineFn();

				--it;
			}
			else
			{
				newLineFn();

				if( draw )
				{
					pdfData.drawText( pdfData.coords.x,
						pdfData.coords.y + d, str );
					ret.append( qMakePair( QRectF( pdfData.coords.x,
							pdfData.coords.y + d,
							font->GetFontMetrics()->StringWidth( str ), lineHeight ),
						pdfData.currentPageIndex() ) );
				}
				else if( cw )
					cw->append( { font->GetFontMetrics()->StringWidth( str ),
						lineHeight, 0.0, false, false, true, *it } );

				newLineFn();

				if( cw && it + 1 == last && footnoteAtEnd )
					cw->append( { footnoteWidth, lineHeight, 0.0, false, false, true,
						QString::number( footnoteNum ) } );
			}
		}
	}

	return ret;
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote )
{
	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );

	auto * font = createFont( renderOpts.m_codeFont, false, false, renderOpts.m_codeFontSize,
		pdfData.doc, scale, pdfData );

	return drawString( pdfData, renderOpts, item->text(), font, font,
		textFont->GetFontMetrics()->GetLineSpacing(),
		doc, newLine, nullptr, 1.0, nullptr, m_footnoteNum,
		offset, firstInParagraph, cw, renderOpts.m_codeBackground, inFootnote );
}

void
PdfRenderer::moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
	double yOffsetMultiplier )
{
	pdfData.coords.x = pdfData.coords.margins.left + xOffset;
	pdfData.coords.y -= yOffset * yOffsetMultiplier;

	if( pdfData.coords.y < pdfData.currentPageAllowedY() &&
		qAbs( pdfData.coords.y - pdfData.currentPageAllowedY() ) > 0.1 )
	{
		createPage( pdfData );

		pdfData.coords.x = pdfData.coords.margins.left + xOffset;
		pdfData.coords.y -= yOffset;
	}
}

namespace /* anonymous */ {

QVector< WhereDrawn > toWhereDrawn( const QVector< QPair< QRectF, int > > & rects,
	double pageHeight )
{
	struct AuxData{
		double minY = 0.0;
		double maxY = 0.0;
	}; // struct AuxData

	QMap< int, AuxData > map;

	for( const auto & r : rects )
	{
		if( !map.contains( r.second ) )
			map[ r.second ] = { pageHeight, 0.0 };

		if( r.first.y() < map[ r.second ].minY )
			map[ r.second ].minY = r.first.y();

		if( r.first.height() + r.first.y() > map[ r.second ].maxY )
			map[ r.second ].maxY = r.first.height() + r.first.y();
	}

	QVector< WhereDrawn > ret;

	for( auto it = map.cbegin(), last = map.cend(); it != last; ++it )
		ret.append( { it.key(), it.value().minY, it.value().maxY - it.value().minY } );

	return ret;
}

double totalHeight( const QVector< WhereDrawn > & where )
{
	return std::accumulate( where.cbegin(), where.cend(), 0.0,
		[] ( const double & val, const WhereDrawn & cur ) -> double
			{ return ( val + cur.height ); } );
}

} /* namespace anonymous */

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Paragraph< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset, bool withNewLine,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote )
{
	QVector< QPair< QRectF, int > > rects;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return {};
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing paragraph." ) );

	auto * font = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

	auto * footnoteFont = font;

	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	pdfData.lineHeight = lineHeight;

	pdfData.coords.x = pdfData.coords.margins.left + offset;

	bool newLine = false;
	CustomWidth cw;

	auto footnoteNum = m_footnoteNum;

	bool lineBreak = false;

	// Calculate words/lines/spaces widthes.
	for( auto it = item->items().begin(), last = item->items().end(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return {};
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Text :
				drawText( pdfData, renderOpts, static_cast< MD::Text< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, footnoteFont, c_footnoteScale,
					( it + 1 != last ? ( it + 1 )->get() : nullptr ),
					footnoteNum, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Code :
				drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Link :
				drawLink( pdfData, renderOpts, static_cast< MD::Link< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, footnoteFont, c_footnoteScale,
					( it + 1 != last ? ( it + 1 )->get() : nullptr ),
					footnoteNum, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Image :
				drawImage( pdfData, renderOpts, static_cast< MD::Image< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale );
				lineBreak = false;
				break;

			case MD::ItemType::Math :
				drawMathExpr( pdfData, renderOpts, static_cast< MD::Math< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( std::next( it ) != last),
					( it == item->items().begin() || lineBreak ),
					&cw, scale );
				lineBreak = false;
				break;

			case MD::ItemType::LineBreak :
			{
				lineBreak = true;
				cw.append( { 0.0, lineHeight, 0.0, false, true, false, "" } );
				pdfData.coords.x = pdfData.coords.margins.left + offset;
			}
				break;

			case MD::ItemType::FootnoteRef :
				++footnoteNum;
				lineBreak = false;
				break;

			default :
				break;
		}
	}

	cw.append( { 0.0, lineHeight, 0.0, false, true, false, "" } );

	cw.calcScale( pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset );

	cw.setDrawing();

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		{
			QVector< WhereDrawn > r;
			r.append( { -1, 0.0,
				( ( withNewLine && !pdfData.firstOnPage ) ||
					( withNewLine && pdfData.drawFootnotes ) ?
						lineHeight + cw.firstItemHeight() :
						cw.firstItemHeight() ) } );

			return { r, {} };
		}

		case CalcHeightOpt::Full :
		{
			QVector< WhereDrawn > r;

			double h = 0.0;
			double max = 0.0;

			for( auto it = cw.cbegin(), last = cw.cend(); it != last; ++it )
			{
				if( it == cw.cbegin() && ( ( withNewLine && !pdfData.firstOnPage ) ||
					( withNewLine && pdfData.drawFootnotes ) ) )
						h += lineHeight;

				if( h + it->height > max )
					max = h + it->height;

				if( it->isNewLine )
				{
					r.append( { -1, 0.0, max } );
					max = 0.0;
					h = 0.0;
				}
			}

			return { r, {} };
		}

		default :
			break;
	}

	if( ( withNewLine && !pdfData.firstOnPage && heightCalcOpt == CalcHeightOpt::Unknown ) ||
		( withNewLine && pdfData.drawFootnotes && heightCalcOpt == CalcHeightOpt::Unknown ) )
			moveToNewLine( pdfData, offset, lineHeight + cw.height(), 1.0 );
	else
		moveToNewLine( pdfData, offset, cw.height(), 1.0 );

	pdfData.coords.x = pdfData.coords.margins.left + offset;

	bool extraOnFirstLine = true;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		if( pdfData.coords.y < pdfData.coords.margins.bottom )
		{
			createPage( pdfData );
			extraOnFirstLine = false;
		}
	}

	newLine = false;

	const auto firstLineY = pdfData.coords.y;
	const auto firstLinePageIdx = pdfData.currentPageIndex();
	const auto firstLineHeight = cw.height();

	pdfData.continueParagraph = true;

	lineBreak = false;

	// Actual drawing.
	for( auto it = item->items().begin(), last = item->items().end(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return {};
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Text :
				rects.append( drawText( pdfData, renderOpts, static_cast< MD::Text< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, nullptr, 1.0, nullptr, m_footnoteNum,
					offset, ( it == item->items().begin() || lineBreak ), &cw, scale, inFootnote ) );
				lineBreak = false;
				break;

			case MD::ItemType::Code :
				rects.append( drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote ) );
				lineBreak = false;
				break;

			case MD::ItemType::Link :
			{
				auto link = static_cast< MD::Link< MD::QStringTrait >* > ( it->get() );

				if( !link->img()->isEmpty() && extraOnFirstLine )
					pdfData.coords.y += cw.height();

				rects.append( drawLink( pdfData, renderOpts, link,
					doc, newLine, nullptr, 1.0, nullptr, m_footnoteNum,
					offset, ( it == item->items().begin() || lineBreak ), &cw, scale, inFootnote ) );
				lineBreak = false;
			}
				break;

			case MD::ItemType::Image :
			{
				if( extraOnFirstLine )
					pdfData.coords.y += cw.height();

				rects.append( drawImage( pdfData, renderOpts, static_cast< MD::Image< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale ) );
				lineBreak = false;
			}
				break;

			case MD::ItemType::Math :
				rects.append( drawMathExpr( pdfData, renderOpts, static_cast< MD::Math< MD::QStringTrait >* > ( it->get() ),
					doc, newLine, offset, ( std::next( it ) != last ),
					( it == item->items().begin() || lineBreak ),
					&cw, scale ) );
				lineBreak = false;
				break;

			case MD::ItemType::LineBreak :
				lineBreak = true;
				moveToNewLine( pdfData, offset, lineHeight, 1.0 );
				break;

			case MD::ItemType::FootnoteRef :
			{
				lineBreak = false;
				if( !inFootnote )
				{
					auto * ref = static_cast< MD::FootnoteRef< MD::QStringTrait >* > ( it->get() );

					const auto fit = doc->footnotesMap().find( ref->id() );

					if( fit != doc->footnotesMap().cend() )
					{
						const auto str = createPdfString( QString::number( m_footnoteNum ) );

						const auto old = footnoteFont->GetFontSize();
						footnoteFont->SetFontSize( old * c_footnoteScale );

						const auto w = footnoteFont->GetFontMetrics()->StringWidth( str );

						rects.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
								w, lineHeight ),
							pdfData.currentPageIndex() ) );

						++m_footnoteNum;

						pdfData.drawText( pdfData.coords.x, pdfData.coords.y + lineHeight -
							footnoteFont->GetFontMetrics()->GetLineSpacing(), str );
						footnoteFont->SetFontSize( old );

						pdfData.coords.x += w;

						addFootnote( fit->second, pdfData, renderOpts, doc );
					}
				}
			}
				break;

			default :
				break;
		}

		extraOnFirstLine = false;
	}

	pdfData.continueParagraph = false;

	return { toWhereDrawn( normalizeRects( rects ), pdfData.coords.pageHeight ),
		{ firstLinePageIdx, firstLineY, firstLineHeight } };
}

QPair< QRectF, int >
PdfRenderer::drawMathExpr( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Math< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
	bool & newLine, double offset, bool hasNext,
	bool firstInParagraph, CustomWidth * cw, float scale )
{
	static const double dpi = 200.0;

	JKQTMathText mt;
	mt.useAnyUnicode( renderOpts.m_mathFont, renderOpts.m_mathFont );

	const double fontSizePx = ( renderOpts.m_mathFontSize / 72.0 ) * dpi;

	mt.setFontSizePixels( qRound( fontSizePx * scale ) );
	mt.parse( item->expr() );

	QPainter tmpP;
	const QSizeF size = mt.getSize( tmpP );
	auto descent = mt.getDescent( tmpP );

	QPixmap px( qRound( size.width() ), qRound( size.height() ) );
	px.fill();
	QPainter p( &px );

	mt.draw( p, 0, QRectF( QPointF( 0.0, 0.0 ), size ) );

	const auto img = px.toImage();

	QByteArray data;
	QBuffer buf( &data );

	img.save( &buf, "png" );

	PdfImage pdfImg( pdfData.doc );
	pdfImg.SetDpi( qRound( dpi ) );
	pdfImg.LoadFromData( reinterpret_cast< const unsigned char * > ( data.data() ), data.size() );

	descent = ( pdfImg.GetHeight() / size.height() ) * descent;

	auto * font = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	newLine = false;

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

	double h = ( cw && cw->isDrawing() ? cw->height() : 0.0 );

	if( draw )
	{
		if( !item->isInline() )
		{
			newLine = true;

			if( !firstInParagraph )
			{
				moveToNewLine( pdfData, offset, 0.0, 1.0 );

				if( cw )
				{
					cw->moveToNextLine();
					h = cw->height();
				}
			}

			double x = 0.0;
			double imgScale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;
			double availableHeight = pdfData.coords.y - pdfData.currentPageAllowedY();

			if( pdfImg.GetWidth() - availableWidth > 0.01 )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale - pageHeight > 0.01 )
			{
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}
			else if( pdfImg.GetHeight() * imgScale - availableHeight > 0.01 )
			{
				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}

			if( availableWidth - pdfImg.GetWidth() * imgScale > 0.01 )
				x = ( availableWidth - pdfImg.GetWidth() * imgScale ) / 2.0;

			pdfData.drawImage( pdfData.coords.x + x,
				pdfData.coords.y - pdfImg.GetHeight() * imgScale -
					( h - pdfImg.GetHeight() * imgScale ) / 2.0,
				&pdfImg, imgScale, imgScale );

			pdfData.coords.y -= pdfImg.GetHeight() * imgScale;

			const QRectF r = { pdfData.coords.x + x,
				pdfData.coords.y - pdfImg.GetHeight() * imgScale -
				( h - pdfImg.GetHeight() * imgScale ) / 2.0,
				pdfImg.GetWidth() * imgScale, pdfImg.GetHeight() * imgScale };
			const auto idx = pdfData.currentPageIndex();

			if( hasNext )
			{
				moveToNewLine( pdfData, offset, lineHeight, 1.0 );

				if( cw )
					cw->moveToNextLine();
			}

			return { r, idx };
		}
		else
		{
			auto sscale = 100.0;

			if( cw )
				sscale = cw->scale();

			const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

			pdfData.coords.x += spaceWidth * sscale / 100.0;

			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.x -
				pdfData.coords.margins.right;

			const double availableTotalWidth = pdfData.coords.pageWidth -
				pdfData.coords.margins.left - pdfData.coords.margins.right - offset;

			if( pdfImg.GetWidth() - availableWidth > 0.01 )
			{
				if( cw )
				{
					cw->moveToNextLine();
					h = cw->height();
				}

				moveToNewLine( pdfData, offset, h, 1.0 );
			}

			double imgScale = 1.0;

			if( pdfImg.GetWidth() - availableTotalWidth > 0.01 )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			double availableHeight = pdfData.coords.y - pdfData.currentPageAllowedY();

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale - pageHeight > 0.01 )
			{
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}
			else if( pdfImg.GetHeight() * imgScale - availableHeight > 0.01 )
			{
				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}

			pdfData.drawImage( pdfData.coords.x,
				pdfData.coords.y + ( h - pdfImg.GetHeight() * imgScale ) / 2.0,
				&pdfImg, imgScale, imgScale );

			pdfData.coords.x += pdfImg.GetWidth() * imgScale;

			const QRectF r = { pdfData.coords.x,
				pdfData.coords.y + ( h - pdfImg.GetHeight() * imgScale ) / 2.0,
				pdfImg.GetWidth() * imgScale, pdfImg.GetHeight() * imgScale };
			const auto idx = pdfData.currentPageIndex();

			return { r, idx };
		}
	}
	else
	{
		if( !item->isInline() )
		{
			double height = 0.0;

			newLine = true;

			const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

			if( !firstInParagraph )
				height += lineHeight;

			double imgScale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;

			if( pdfImg.GetWidth() - availableWidth > 0.01 )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale - pageHeight > 0.01 )
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

			height += pdfImg.GetHeight() * imgScale - font->GetFontMetrics()->GetDescent();

			pdfData.coords.x = pdfData.coords.margins.left + offset;

			cw->append( { 0.0, 0.0, descent, false, true, false, "" } );
			cw->append( { 0.0, height, descent, false, true, false, "" } );

			if( hasNext )
				cw->append( { 0.0, 0.0, descent, false, true, false, "" } );
		}
		else
		{
			const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.x -
				pdfData.coords.margins.right - spaceWidth;

			const double availableTotalWidth = pdfData.coords.pageWidth -
				pdfData.coords.margins.left - pdfData.coords.margins.right - offset;

			pdfData.coords.x += spaceWidth;

			if( pdfImg.GetWidth() - availableWidth > 0.01 )
			{
				cw->append( { 0.0, lineHeight, descent, false, true, true, "" } );
				pdfData.coords.x = pdfData.coords.margins.left + offset;
			}
			else
				cw->append( { spaceWidth, lineHeight, descent, true, false, true, " " } );

			double imgScale = 1.0;

			if( pdfImg.GetWidth() - availableTotalWidth > 0.01 )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			double availableHeight = pdfData.coords.y - pdfData.currentPageAllowedY();

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale - pageHeight > 0.01 )
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

			pdfData.coords.x += pdfImg.GetWidth() * imgScale;

			cw->append( { pdfImg.GetWidth() * imgScale,
				pdfImg.GetHeight() * imgScale - font->GetFontMetrics()->GetDescent(),
				descent, false, false, hasNext, "" } );
		}
	}

	return {};
}

void
PdfRenderer::reserveSpaceForFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	const QVector< WhereDrawn > & h, const double & currentY, int currentPage,
	double lineHeight, bool addExtraLine )
{
	const auto topY = pdfData.topFootnoteY( currentPage );
	const auto available = currentY - topY -
		( qAbs( topY ) < 0.01 ? pdfData.coords.margins.bottom : 0.0 );

	auto height = totalHeight( h ) + ( addExtraLine ? lineHeight : 0.0 );
	auto extra = 0.0;

	if( !pdfData.reserved.contains( currentPage ) )
		extra = pdfData.extraInFootnote;

	auto add = [&pdfData] ( const double & height, int currentPage )
	{
		if( pdfData.reserved.contains( currentPage ) )
			pdfData.reserved[ currentPage ] += height;
		else
			pdfData.reserved.insert( currentPage,
				height + pdfData.coords.margins.bottom );
	};

	if( height + extra < available )
		add( height + extra, currentPage );
	else
	{
		height = extra + ( addExtraLine ? lineHeight : 0.0 );

		for( int i = 0; i < h.size(); ++i )
		{
			const auto tmp = h[ i ].height;

			if( height + tmp < available )
				height += tmp;
			else
			{
				if( qAbs( height - extra ) > 0.01 )
					add( height, currentPage );

				reserveSpaceForFootnote( pdfData, renderOpts, h.mid( i ),
					pdfData.coords.pageHeight - pdfData.coords.margins.top,
					currentPage + 1, lineHeight, true );

				break;
			}
		}
	}
}

QVector< WhereDrawn >
PdfRenderer::drawFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
	MD::Footnote< MD::QStringTrait > * note, CalcHeightOpt heightCalcOpt,
	double * lineHeight )
{
	QVector< WhereDrawn > ret;

	static const double c_offset = 2.0;

	auto * font = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, c_footnoteScale, pdfData );
	auto footnoteOffset = c_offset * 2.0 / c_mmInPt +
		font->GetFontMetrics()->StringWidth( createPdfString(
			QString::number( doc->footnotesMap().size() ) ) );

	if( lineHeight )
		*lineHeight = font->GetFontMetrics()->GetLineSpacing();

	for( auto it = note->items().cbegin(), last = note->items().cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				break;
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Heading :
				ret.append( drawHeading( pdfData, renderOpts,
					static_cast< MD::Heading< MD::QStringTrait >* > ( it->get() ),
					doc, footnoteOffset,
					// If there is another item after heading we need to know its min
					// height to glue heading with it.
					( it + 1 != last ?
						minNecessaryHeight( pdfData, renderOpts, *( it + 1 ), doc, 0.0,
							c_footnoteScale, true ) :
						0.0 ), heightCalcOpt, c_footnoteScale ).first );
				pdfData.continueParagraph = true;
				break;

			case MD::ItemType::Paragraph :
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph< MD::QStringTrait >* > ( it->get() ), doc, footnoteOffset,
					true, heightCalcOpt, c_footnoteScale, true ).first );
				pdfData.continueParagraph = true;
				break;

			case MD::ItemType::Code :
				ret.append( drawCode( pdfData, renderOpts, static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale ).first );
				pdfData.continueParagraph = true;
				break;

			case MD::ItemType::Blockquote :
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote< MD::QStringTrait >* > ( it->get() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale, true ).first );
				pdfData.continueParagraph = true;
				break;

			case MD::ItemType::List :
			{
				auto * list = static_cast< MD::List< MD::QStringTrait >* > ( it->get() );
				const auto bulletWidth = maxListNumberWidth( list );

				ret.append( drawList( pdfData, renderOpts, list, doc, bulletWidth, footnoteOffset,
					heightCalcOpt, c_footnoteScale, true ).first );

				pdfData.continueParagraph = true;
			}
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table< MD::QStringTrait >* > ( it->get() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale, true ).first );
				pdfData.continueParagraph = true;
				break;

			default :
				break;
		}

		// Draw footnote number.
		if( it == note->items().cbegin() && heightCalcOpt == CalcHeightOpt::Unknown )
		{
			const auto str = createPdfString( QString::number( pdfData.currentFootnote ) );
			const auto w = font->GetFontMetrics()->StringWidth( str );
			const auto y = ret.constFirst().y + ret.constFirst().height -
				font->GetFontMetrics()->GetLineSpacing();
			const auto x = pdfData.coords.margins.left + footnoteOffset -
				c_offset - w;
			const auto p = ret.constFirst().pageIdx;

			pdfData.painter->SetPage( pdfData.doc->GetPage( p ) );
			pdfData.painter->SetFont( font );
			pdfData.drawText( x, y, str );
			pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.footnotePageIdx ) );

			++pdfData.currentFootnote;
		}
	}

	pdfData.continueParagraph = false;

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::footnoteHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	std::shared_ptr< MD::Document< MD::QStringTrait > > doc, MD::Footnote< MD::QStringTrait > * note,
	double * lineHeight )
{
	return drawFootnote( pdfData, renderOpts, doc, note, CalcHeightOpt::Full, lineHeight );
}

QPair< QRectF, int >
PdfRenderer::drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Image< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float scale )
{
	Q_UNUSED( doc )

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

	if( draw )
	{
		emit status( tr( "Loading image." ) );

		const auto img = loadImage( item );

		if( !img.isNull() )
		{
			PdfImage pdfImg( pdfData.doc );
			pdfImg.SetDpi( pdfData.m_dpi );
			pdfImg.LoadFromData( reinterpret_cast< const unsigned char * >( img.data() ), img.size() );

			newLine = true;

			auto * font = createFont( renderOpts.m_textFont, false, false,
				renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

			const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

			if( !firstInParagraph )
				moveToNewLine( pdfData, offset, lineHeight, 1.0 );
			else
				pdfData.coords.x += offset;

			double x = 0.0;
			double imgScale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;
			double availableHeight = pdfData.coords.y - pdfData.currentPageAllowedY();

			if( pdfImg.GetWidth() > availableWidth )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale > pageHeight )
			{
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}
			else if( pdfImg.GetHeight() * imgScale > availableHeight )
			{
				createPage( pdfData );

				pdfData.freeSpaceOn( pdfData.currentPageIndex() );

				pdfData.coords.x += offset;
			}

			if( pdfImg.GetWidth() * imgScale < availableWidth )
				x = ( availableWidth - pdfImg.GetWidth() * imgScale ) / 2.0;

			pdfData.drawImage( pdfData.coords.x + x,
				pdfData.coords.y - pdfImg.GetHeight() * imgScale,
				&pdfImg, imgScale, imgScale );

			pdfData.coords.y -= pdfImg.GetHeight() * imgScale;

			QRectF r( pdfData.coords.x + x, pdfData.coords.y,
				pdfImg.GetWidth() * imgScale, pdfImg.GetHeight() * imgScale );

			moveToNewLine( pdfData, offset, lineHeight, 1.0 );

			return qMakePair( r, pdfData.currentPageIndex() );
		}
		else
			throw PdfRendererError( tr( "Unable to load image: %1.\n\n"
				"If this image is in Web, please be sure you are connected to the Internet. I'm "
				"sorry for the inconvenience." )
					.arg( item->url() ) );
	}
	else
	{
		emit status( tr( "Loading image." ) );

		const auto img = loadImage( item );

		auto height = 0.0;

		if( !img.isNull() )
		{
			PdfImage pdfImg( pdfData.doc );
			pdfImg.SetDpi( pdfData.m_dpi );
			pdfImg.LoadFromData( reinterpret_cast< const unsigned char * >( img.data() ), img.size() );

			newLine = true;

			auto * font = createFont( renderOpts.m_textFont, false, false,
				renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

			const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

			if( !firstInParagraph )
				height += lineHeight;

			double imgScale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;

			if( pdfImg.GetWidth() > availableWidth )
				imgScale = ( availableWidth / pdfImg.GetWidth() ) * scale;

			const double pageHeight = pdfData.topY( pdfData.currentPageIndex() ) -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * imgScale > pageHeight )
				imgScale = ( pageHeight / ( pdfImg.GetHeight() * imgScale ) ) * scale;

			height += pdfImg.GetHeight() * imgScale;
		}

		pdfData.coords.x = pdfData.coords.margins.left + offset;

		if( !cw->isEmpty() )
			cw->append( { 0.0, 0.0, 0.0, false, true, false, "" } );

		cw->append( { 0.0, height, 0.0, false, true, false, "" } );

		return qMakePair( QRectF(), pdfData.currentPageIndex() );
	}
}

//
// convert
//

QImage
convert( const Magick::Image & img )
{
	QImage qimg( static_cast< int > ( img.columns() ),
		static_cast< int > ( img.rows() ), QImage::Format_RGB888 );
	const Magick::PixelPacket * pixels;
	Magick::ColorRGB rgb;

	for( int y = 0; y < qimg.height(); ++y)
	{
		pixels = img.getConstPixels( 0, y, static_cast< std::size_t > ( qimg.width() ), 1 );

		for( int x = 0; x < qimg.width(); ++x )
		{
			rgb = ( *( pixels + x ) );

			qimg.setPixel( x, y, QColor( static_cast< int> ( 255 * rgb.red() ),
				static_cast< int > ( 255 * rgb.green() ),
				static_cast< int > ( 255 * rgb.blue() ) ).rgb());
		}
	}

	return qimg;
}


//
// LoadImageFromNetwork
//

LoadImageFromNetwork::LoadImageFromNetwork( const QUrl & url, QThread * thread )
	:	m_thread( thread )
	,	m_reply( nullptr )
	,	m_url( url )
{
	connect( this, &LoadImageFromNetwork::start, this, &LoadImageFromNetwork::loadImpl,
		Qt::QueuedConnection );
}

const QImage &
LoadImageFromNetwork::image() const
{
	return m_img;
}

void
LoadImageFromNetwork::load()
{
	emit start();
}

void
LoadImageFromNetwork::loadImpl()
{
	QNetworkAccessManager * m = new QNetworkAccessManager( this );
	QNetworkRequest r( m_url );
	r.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy );
	m_reply = m->get( r );

	connect( m_reply, &QNetworkReply::finished, this, &LoadImageFromNetwork::loadFinished );
	connect( m_reply,
		static_cast< void(QNetworkReply::*)(QNetworkReply::NetworkError) >(
			&QNetworkReply::errorOccurred ),
		this, &LoadImageFromNetwork::loadError );
}

void
LoadImageFromNetwork::loadFinished()
{
	const auto data = m_reply->readAll();
	const auto svg = QString( data.mid( 0, 4 ).toLower() );

	if( svg == QStringLiteral( "<svg" ) )
	{
		try {
			Magick::Image img;
			Magick::Color c( 0x00, 0x00, 0x00, 0xFFFF );
			img.backgroundColor( c );
			QTemporaryFile file( QStringLiteral( "XXXXXX.svg" ) );
			if( file.open() )
			{
				file.write( data );
				file.close();

				img.read( file.fileName().toStdString() );

				img.magick( "png" );

				m_img = convert( img );
			}
		}
		catch( const Magick::Exception & )
		{
		}
	}
	else
		m_img.loadFromData( data );

	m_reply->deleteLater();

	m_thread->quit();
}

void
LoadImageFromNetwork::loadError( QNetworkReply::NetworkError )
{
	m_reply->deleteLater();
	m_thread->quit();
}

QByteArray
PdfRenderer::loadImage( MD::Image< MD::QStringTrait > * item )
{
	if( m_imageCache.contains( item->url() ) )
		return m_imageCache[ item->url() ];

	QImage img;

	if( QFileInfo::exists( item->url() ) )
	{
		if( item->url().toLower().endsWith( QStringLiteral( "svg" ) ) )
		{
			try {
				Magick::Image mimg;
				Magick::Color c(0x0, 0x0, 0x0, 0xFFFF);
				mimg.backgroundColor( c );
				mimg.read( item->url().toStdString() );
				mimg.magick( "png" );
				img = convert( mimg );
			}
			catch( const Magick::Exception & )
			{
			}
		}
		else
			img = QImage( item->url() );
	}
	else if( !QUrl( item->url() ).isRelative() )
	{
		QThread thread;

		LoadImageFromNetwork load( QUrl( item->url() ), &thread );

		load.moveToThread( &thread );
		thread.start();
		load.load();
		thread.wait();

		img = load.image();

#ifdef MD_PDF_TESTING
	if( img.isNull() )
	{
		terminate();

		QWARN( "Got empty image from network." );
	}
#endif
	}
	else
		throw PdfRendererError(
			tr( "Hmm, I don't know how to load this image: %1.\n\n"
				"This image is not a local existing file, and not in the Web. Check your Markdown." )
					.arg( item->url() ) );

	if( img.isNull() )
		throw PdfRendererError( tr( "Unable to load image: %1.\n\n"
			"If this image is in Web, please be sure you are connected to the Internet. I'm "
			"sorry for the inconvenience." ).arg( item->url() ) );

	QByteArray data;
	QBuffer buf( &data );

	QString fmt = QStringLiteral( "png" );

	if( item->url().endsWith( QStringLiteral( "jpg" ) ) ||
		item->url().endsWith( QStringLiteral( "jpeg" ) ))
			fmt = QStringLiteral( "jpg" );

	img.save( &buf, fmt.toLatin1().constData() );

	m_imageCache.insert( item->url(), data );

	return data;
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset, CalcHeightOpt heightCalcOpt,
	float scale )
{
	Q_UNUSED( doc )

	if( item->text().isEmpty() )
		return {};

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing code." ) );

	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );
	const auto textLHeight = textFont->GetFontMetrics()->GetLineSpacing();

	QStringList lines;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		if( pdfData.coords.y - ( textLHeight * 2.0 ) < pdfData.currentPageAllowedY() &&
			qAbs( pdfData.coords.y - ( textLHeight * 2.0 ) - pdfData.currentPageAllowedY() ) > 0.1 )
				createPage( pdfData );
		else
			pdfData.coords.y -= textLHeight * 2.0;

		pdfData.coords.x = pdfData.coords.margins.left + offset;
	}

	lines = item->text().split( QLatin1Char( '\n' ), Qt::KeepEmptyParts );

	for( auto it = lines.begin(), last = lines.end(); it != last; ++it )
		it->replace( QStringLiteral( "\t" ), QStringLiteral( "    " ) );

	auto * font = createFont( renderOpts.m_codeFont, false, false, renderOpts.m_codeFontSize,
		pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		{
			QVector< WhereDrawn > r;
			r.append( { -1, 0.0, textLHeight * 2.0 + lineHeight } );

			return { r, {} };
		}

		case CalcHeightOpt::Full :
		{
			QVector< WhereDrawn > r;
			r.append( { -1, 0.0, textLHeight * 2.0 + lineHeight } );

			auto i = 1;

			while( i < lines.size() )
			{
				r.append( { -1, 0.0, lineHeight } );
				++i;
			}

			return { r, {} };
		}

		default :
			break;
	}

	pdfData.painter->SetFont( font );

	int i = 0;

	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return {};
	}

	const auto colored = Syntax::createSyntaxHighlighter( item->syntax() )->prepare( lines );
	int currentWord = 0;
	const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

	const auto firstLinePageIdx = pdfData.currentPageIndex();
	const auto firstLineY = pdfData.coords.y;
	const auto firstLineHeight = lineHeight;

	while( i < lines.size() )
	{
		auto y = pdfData.coords.y;
		int j = i + 1;
		double h = 0.0;

		while( ( y - lineHeight > pdfData.currentPageAllowedY() ||
			qAbs( y - lineHeight - pdfData.currentPageAllowedY() ) < 0.1 ) && j < lines.size() )
		{
			h += lineHeight;
			y -= lineHeight;
			++j;
		}

		if( i < j )
		{
			pdfData.setColor( renderOpts.m_codeBackground );
			pdfData.drawRectangle( pdfData.coords.x, y + font->GetFontMetrics()->GetDescent(),
				pdfData.coords.pageWidth - pdfData.coords.x - pdfData.coords.margins.right,
				 h + lineHeight );
			pdfData.painter->Fill();
			pdfData.restoreColor();

			ret.append( { pdfData.currentPageIndex(), y, h + lineHeight } );
		}

		for( ; i < j; ++i )
		{
			pdfData.coords.x = pdfData.coords.margins.left + offset;

			while( true )
			{
				if( currentWord == colored.size() || colored[ currentWord ].line != i )
					break;

				switch( colored[ currentWord ].color )
				{
					case Syntax::ColorRole::Keyword :
						pdfData.setColor( renderOpts.m_keywordColor );
						break;

					case Syntax::ColorRole::Regular :
						pdfData.setColor( renderOpts.m_codeColor );
						break;

					case Syntax::ColorRole::Comment :
						pdfData.setColor( renderOpts.m_commentColor );
						break;
				}

				const auto length = colored[ currentWord ].endPos -
					colored[ currentWord ].startPos + 1;

				pdfData.drawText( pdfData.coords.x, pdfData.coords.y,
					createPdfString( lines.at( i ).mid( colored[ currentWord ].startPos, length ) ) );

				pdfData.coords.x += spaceWidth * length;

				pdfData.restoreColor();

				++currentWord;
			}

			pdfData.coords.y -= lineHeight;
		}

		if( i < lines.size() )
		{
			createPage( pdfData );
			pdfData.coords.x = pdfData.coords.margins.left + offset;
			pdfData.coords.y -= lineHeight;
		}
	}

	return { ret, { firstLinePageIdx, firstLineY, firstLineHeight } };
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawBlockquote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Blockquote< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing blockquote." ) );

	bool first  = true;
	WhereDrawn firstLine = {};

	// Draw items.
	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return {};
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Heading :
			{
				if( heightCalcOpt != CalcHeightOpt::Minimum )
				{
					const auto where = drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading< MD::QStringTrait >* > ( it->get() ),
						doc, offset + c_blockquoteBaseOffset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ), doc,
								offset + c_blockquoteBaseOffset, scale, inFootnote  ) : 0.0 ),
						heightCalcOpt, scale );

					ret.append( where.first );

					if( first )
					{
						firstLine = where.second;
						first = false;
					}
				}
				else
				{
					ret.append( { -1, 0.0, 0.0 } );

					return { ret, {} };
				}
			}
				break;

			case MD::ItemType::Paragraph :
			{
				const auto where = drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph< MD::QStringTrait >* > ( it->get() ),
					doc, offset + c_blockquoteBaseOffset, true, heightCalcOpt,
					scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			case MD::ItemType::Code :
			{
				const auto where = drawCode( pdfData, renderOpts,
					static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			case MD::ItemType::Blockquote :
			{
				const auto where = drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote< MD::QStringTrait >* > ( it->get() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			case MD::ItemType::List :
			{
				auto * list = static_cast< MD::List< MD::QStringTrait >* > ( it->get() );
				const auto bulletWidth = maxListNumberWidth( list );

				const auto where = drawList( pdfData, renderOpts,
					list,
					doc, bulletWidth, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			case MD::ItemType::Table :
			{
				const auto where = drawTable( pdfData, renderOpts,
					static_cast< MD::Table< MD::QStringTrait >* > ( it->get() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			default :
				break;
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			return { ret, {} };
	}

	if( heightCalcOpt == CalcHeightOpt::Full )
		return { ret, {} };

	struct AuxData {
		double y = 0.0;
		double height = 0.0;
	}; // struct AuxData

	QMap< int, AuxData > map;

	for( const auto & where : qAsConst( ret ) )
	{
		if( !map.contains( where.pageIdx ) )
			map.insert( where.pageIdx, { where.y, where.height } );

		if( map[ where.pageIdx ].y > where.y )
		{
			map[ where.pageIdx ].height = map[ where.pageIdx ].y +
				map[ where.pageIdx ].height - where.y;
			map[ where.pageIdx ].y = where.y;
		}
	}

	// Draw blockquote left vertival bar.
	for( auto it = map.cbegin(), last = map.cend(); it != last; ++it )
	{
		pdfData.painter->SetPage( pdfData.doc->GetPage( it.key() ) );
		pdfData.setColor( renderOpts.m_borderColor );
		pdfData.drawRectangle( pdfData.coords.margins.left + offset, it.value().y,
			c_blockquoteMarkWidth, it.value().height );
		pdfData.painter->Fill();
		pdfData.restoreColor();
	}

	pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIndex() ) );

	return { ret, firstLine };
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::List< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, int bulletWidth, double offset,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote, bool nested )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return {};
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing list." ) );

	int idx = 1;
	ListItemType prevListItemType = ListItemType::Unknown;
	bool first = true;
	WhereDrawn firstLine;

	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
		{
			const auto where = drawListItem( pdfData, renderOpts,
				static_cast< MD::ListItem< MD::QStringTrait >* > ( it->get() ), doc, idx,
				prevListItemType, bulletWidth, offset, heightCalcOpt,
				scale, inFootnote, first && !nested );

			ret.append( where.first );

			if( first )
			{
				firstLine = where.second;
				first = false;
			}
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			break;
	}

	if( !ret.isEmpty() )
	{
		auto * font = createFont( m_opts.m_textFont, false, false,
			m_opts.m_textFontSize, pdfData.doc, scale, pdfData );

		ret.front().height += font->GetFontMetrics()->GetLineSpacing();
	}

	return { ret, firstLine };
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::ListItem< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, int & idx,
	ListItemType & prevListItemType, int bulletWidth, double offset, CalcHeightOpt heightCalcOpt,
	float scale, bool inFootnote, bool firstInList )
{
	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();
	const auto orderedListNumberWidth =
		font->GetFontMetrics()->StringWidth( PdfString( "9" ) ) * bulletWidth +
		font->GetFontMetrics()->StringWidth( PdfString( "." ) );
	const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );
	const auto unorderedMarkWidth = spaceWidth * 0.75;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		pdfData.painter->SetFont( font );

		offset += orderedListNumberWidth + spaceWidth;
	}

	QVector< WhereDrawn > ret;

	bool addExtraSpace = false;
	bool first = true;
	WhereDrawn firstLine;

	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return {};
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Heading :
			{
				if( heightCalcOpt != CalcHeightOpt::Minimum )
				{
					const auto where = drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading< MD::QStringTrait >* > ( it->get() ),
						doc, offset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ),  doc, offset,
								scale, inFootnote ) :
							0.0 ),
						heightCalcOpt, scale, ( it == item->items().cbegin() && firstInList ) );

					ret.append( where.first );

					if( first )
					{
						firstLine = where.second;
						first = false;
					}
				}
				else
					ret.append( { -1, 0.0, 0.0 } );
			}
				break;

			case MD::ItemType::Paragraph :
			{
				const auto where = drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph< MD::QStringTrait >* > ( it->get() ),
					doc, offset,
					( it == item->items().cbegin() && firstInList ) || it != item->items().cbegin(),
					heightCalcOpt,
					scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::Code :
			{
				const auto where = drawCode( pdfData, renderOpts,
					static_cast< MD::Code< MD::QStringTrait >* > ( it->get() ),
					doc, offset, heightCalcOpt, scale );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}

				addExtraSpace = false;
			}
				break;

			case MD::ItemType::Blockquote :
			{
				const auto where = drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote< MD::QStringTrait >* > ( it->get() ),
					doc, offset, heightCalcOpt, scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::List :
			{
				const auto where = drawList( pdfData, renderOpts,
					static_cast< MD::List< MD::QStringTrait >* > ( it->get() ),
					doc, bulletWidth, offset, heightCalcOpt,
					scale, inFootnote, true );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			case MD::ItemType::Table :
			{
				const auto where = drawTable( pdfData, renderOpts,
					static_cast< MD::Table< MD::QStringTrait >* > ( it->get() ),
					doc, offset, heightCalcOpt, scale, inFootnote );

				ret.append( where.first );

				if( first )
				{
					firstLine = where.second;
					first = false;
				}
			}
				break;

			default :
				break;
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			break;
	}

	if( addExtraSpace )
	{
		ret.append( { pdfData.currentPageIndex(), pdfData.coords.y, lineHeight } );

		if( heightCalcOpt != CalcHeightOpt::Full )
			moveToNewLine( pdfData, offset, lineHeight, 1.0 );
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		pdfData.painter->SetFont( font );
		pdfData.painter->SetPage( pdfData.doc->GetPage( firstLine.pageIdx ) );

		if( item->isTaskList() )
		{
			pdfData.setColor( Qt::black );
			pdfData.drawRectangle(
				pdfData.coords.margins.left + offset - ( orderedListNumberWidth + spaceWidth ),
				firstLine.y + qAbs( firstLine.height - orderedListNumberWidth ) / 2.0,
				orderedListNumberWidth, orderedListNumberWidth );
			pdfData.painter->Stroke();

			if( item->isChecked() )
			{
				const auto d = orderedListNumberWidth * 0.2;

				pdfData.drawRectangle(
					pdfData.coords.margins.left + offset + d - ( orderedListNumberWidth + spaceWidth ),
					firstLine.y + qAbs( firstLine.height - orderedListNumberWidth ) / 2.0 + d,
					orderedListNumberWidth - 2.0 * d, orderedListNumberWidth - 2.0 * d );
				pdfData.painter->Fill();
			}

			pdfData.restoreColor();
		}
		else if( item->listType() == MD::ListItem< MD::QStringTrait >::Ordered )
		{
			if( prevListItemType == ListItemType::Unordered )
				idx = 1;
			else if( prevListItemType == ListItemType::Ordered )
				++idx;

			prevListItemType = ListItemType::Ordered;

			const QString idxText = QString::number( idx ) + QLatin1Char( '.' );

			pdfData.drawText(
				pdfData.coords.margins.left + offset - ( orderedListNumberWidth + spaceWidth ),
				firstLine.y + qAbs( firstLine.height - font->GetFontMetrics()->GetAscent() ) / 2.0,
				createPdfString( idxText ) );
		}
		else
		{
			prevListItemType = ListItemType::Unordered;

			pdfData.setColor( Qt::black );
			const auto r = unorderedMarkWidth / 2.0;
			pdfData.painter->Circle(
				pdfData.coords.margins.left + offset + r - ( orderedListNumberWidth + spaceWidth ),
				firstLine.y + qAbs( firstLine.height - unorderedMarkWidth ) / 2.0, r );
			pdfData.painter->Fill();
			pdfData.restoreColor();
		}

		pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIndex() ) );
	}

	return { ret, firstLine };
}

int
PdfRenderer::maxListNumberWidth( MD::List< MD::QStringTrait > * list ) const
{
	int counter = 0;

	for( auto it = list->items().cbegin(), last = list->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
		{
			auto * item = static_cast< MD::ListItem< MD::QStringTrait >* > ( it->get() );

			if( item->listType() == MD::ListItem< MD::QStringTrait >::Ordered )
				++counter;
		}
	}

	for( auto it = list->items().cbegin(), last = list->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
		{
			auto * item = static_cast< MD::ListItem< MD::QStringTrait >* > ( it->get() );

			for( auto lit = item->items().cbegin(), llast = item->items().cend(); lit != llast; ++lit )
			{
				if( (*lit)->type() == MD::ItemType::List )
				{
					auto i = maxListNumberWidth( static_cast< MD::List< MD::QStringTrait >* > ( lit->get() ) );

					if( i > counter )
						counter = i;
				}
			}
		}
	}

	return ( counter / 10 + 1 );
}

bool
operator != ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 )
{
	return ( f1.family != f2.family || f1.bold != f2.bold ||
		f1.italic != f2.italic || f1.strikethrough != f2.strikethrough ||
		f1.size != f2.size );
}

bool
operator == ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 )
{
	return ( f1.family == f2.family && f1.bold == f2.bold &&
		f1.italic == f2.italic && f1.strikethrough == f2.strikethrough &&
		f1.size == f2.size );
}

QVector< QVector< PdfRenderer::CellData > >
PdfRenderer::createAuxTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Table< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, float scale, bool inFootnote )
{
	Q_UNUSED( pdfData )
	Q_UNUSED( scale )

	const auto columnsCount = item->columnsCount();

	QVector< QVector< CellData > > auxTable;
	auxTable.resize( columnsCount );

	for( auto rit = item->rows().cbegin(), rlast = item->rows().cend(); rit != rlast; ++rit )
	{
		int i = 0;

		for( auto cit = (*rit)->cells().cbegin(), clast = (*rit)->cells().cend(); cit != clast; ++cit )
		{
			if( i == columnsCount )
				break;

			CellData data;
			data.alignment = item->columnAlignment( i );

			for( auto it = (*cit)->items().cbegin(), last = (*cit)->items().cend(); it != last; ++it )
			{
				switch( (*it)->type() )
				{
					case MD::ItemType::Text :
					{
						auto * t = static_cast< MD::Text< MD::QStringTrait >* > ( it->get() );

						const auto words = t->text().split( QLatin1Char( ' ' ),
							Qt::SkipEmptyParts );

						for( const auto & w : words )
						{
							CellItem item;
							item.word = w;
							item.font = { renderOpts.m_textFont,
								(bool) ( t->opts() & MD::TextOption::BoldText ),
								(bool) ( t->opts() & MD::TextOption::ItalicText ),
								(bool) ( t->opts() & MD::TextOption::StrikethroughText ),
								renderOpts.m_textFontSize };

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Code :
					{
						auto * c = static_cast< MD::Code< MD::QStringTrait >* > ( it->get() );

						const auto words = c->text().split( QLatin1Char( ' ' ),
							Qt::SkipEmptyParts );

						for( const auto & w : words )
						{
							CellItem item;
							item.word = w;
							item.font = { renderOpts.m_codeFont, false, false, false,
								renderOpts.m_codeFontSize };
							item.background = renderOpts.m_codeBackground;

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Link :
					{
						auto * l = static_cast< MD::Link< MD::QStringTrait >* > ( it->get() );

						QString url = l->url();

						const auto lit = doc->labeledLinks().find( url );

						if( lit != doc->labeledLinks().cend() )
							url = lit->second->url();

						if( !l->img()->isEmpty() )
						{
							CellItem item;
							item.image = loadImage( l->img().get() );
							item.url = url;
							item.font = { renderOpts.m_textFont,
								(bool) ( l->opts() & MD::TextOption::BoldText ),
								(bool) ( l->opts() & MD::TextOption::ItalicText ),
								(bool) ( l->opts() & MD::TextOption::StrikethroughText ),
								renderOpts.m_textFontSize };

							data.items.append( item );
						}
						else if( !l->text().isEmpty() )
						{
							const auto words = l->text().split( QLatin1Char( ' ' ),
								Qt::SkipEmptyParts );

							for( const auto & w : words )
							{
								CellItem item;
								item.word = w;
								item.font = { renderOpts.m_textFont,
									(bool) ( l->opts() & MD::TextOption::BoldText ),
									(bool) ( l->opts() & MD::TextOption::ItalicText ),
									(bool) ( l->opts() & MD::TextOption::StrikethroughText ),
									renderOpts.m_textFontSize };
								item.url = url;
								item.color = renderOpts.m_linkColor;

								data.items.append( item );
							}
						}
						else
						{
							CellItem item;
							item.font = { renderOpts.m_textFont,
								(bool) ( l->opts() & MD::TextOption::BoldText ),
								(bool) ( l->opts() & MD::TextOption::ItalicText ),
								(bool) ( l->opts() & MD::TextOption::StrikethroughText ),
								renderOpts.m_textFontSize };
							item.url = url;
							item.color = renderOpts.m_linkColor;

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Image :
					{
						auto * i = static_cast< MD::Image< MD::QStringTrait >* > ( it->get() );

						CellItem item;

						emit status( tr( "Loading image." ) );

						item.image = loadImage( i );
						item.font = { renderOpts.m_textFont,
							false, false, false, renderOpts.m_textFontSize };

						data.items.append( item );
					}
						break;

					case MD::ItemType::FootnoteRef :
					{
						if( !inFootnote )
						{
							auto * ref = static_cast< MD::FootnoteRef< MD::QStringTrait >* > ( it->get() );

							const auto fit = doc->footnotesMap().find( ref->id() );

							if( fit != doc->footnotesMap().cend() )
							{
								CellItem item;
								item.font = { renderOpts.m_textFont,
									false, false, false,
									renderOpts.m_textFontSize };
								item.footnote = QString::number( m_footnoteNum++ );
								item.footnoteObj = fit->second;

								data.items.append( item );
							}
						}
					}
						break;

					default :
						break;
				}
			}

			auxTable[ i ].append( data );

			++i;
		}

		for( ; i < columnsCount; ++i )
			auxTable[ i ].append( CellData() );
	}

	return auxTable;
}

void
PdfRenderer::calculateCellsSize( PdfAuxData & pdfData, QVector< QVector< CellData > > & auxTable,
	double spaceWidth, double offset, double lineHeight, float scale )
{
	QVector< double > columnWidthes;
	columnWidthes.resize( auxTable.size() );

	const auto availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset;

	const auto width = availableWidth / auxTable.size();

	for( auto it = auxTable.begin(), last = auxTable.end(); it != last; ++it )
	{
		for( auto cit = it->begin(), clast = it->end(); cit != clast; ++cit )
			cit->setWidth( width - c_tableMargin * 2.0 );
	}

	for( auto it = auxTable.begin(), last = auxTable.end(); it != last; ++it )
		for( auto cit = it->begin(), clast = it->end(); cit != clast; ++cit )
			cit->heightToWidth( lineHeight, spaceWidth, scale, pdfData, this );
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Table< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
	double offset, CalcHeightOpt heightCalcOpt,
	float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return {};
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing table." ) );

	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();
	const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

	auto auxTable = createAuxTable( pdfData, renderOpts, item, doc, scale, inFootnote );

	calculateCellsSize( pdfData, auxTable, spaceWidth, offset, lineHeight, scale );

	const auto r0h = rowHeight( auxTable, 0 );
	const bool justHeader = auxTable.at( 0 ).size() == 1;
	const auto r1h = ( !justHeader ? rowHeight( auxTable, 1 ) : 0 );

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		{
			ret.append( { -1, 0.0, r0h + r1h + ( c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) +
				lineHeight - ( font->GetFontMetrics()->GetDescent() * ( justHeader ? 1.0 : 2.0 ) ) } );

			return { ret, {} };
		}

		case CalcHeightOpt::Full :
		{
			ret.append( { -1, 0.0, r0h + r1h + ( c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) +
				lineHeight - ( font->GetFontMetrics()->GetDescent() * ( justHeader ? 1.0 : 2.0 ) ) } );

			for( int i = 2; i < auxTable.at( 0 ).size(); ++i )
				ret.append( { -1, 0.0, rowHeight( auxTable, i ) + c_tableMargin * 2.0 -
					font->GetFontMetrics()->GetDescent() } );

			return { ret, {} };
		}

		default :
			break;
	}

	const auto nonSplittableHeight = ( r0h + r1h + ( c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) -
		( font->GetFontMetrics()->GetDescent() * ( justHeader ? 1.0 : 2.0 ) ) );

	if( pdfData.coords.y - nonSplittableHeight < pdfData.currentPageAllowedY() &&
		qAbs( pdfData.coords.y - nonSplittableHeight - pdfData.currentPageAllowedY() ) > 0.1 )
	{
		createPage( pdfData );

		pdfData.freeSpaceOn( pdfData.currentPageIndex() );
	}

	moveToNewLine( pdfData, offset, lineHeight, 1.0 );

	QVector< std::shared_ptr< MD::Footnote< MD::QStringTrait > > > footnotes;
	bool first = true;
	WhereDrawn firstLine;

	for( int row = 0; row < auxTable[ 0 ].size(); ++row )
	{
		const auto where = drawTableRow( auxTable, row, pdfData, offset, lineHeight, renderOpts,
			doc, footnotes, scale, inFootnote );

		ret.append( where.first );

		if( first )
		{
			firstLine = where.second;
			first = false;
		}
	}

	for( const auto & f : qAsConst( footnotes ) )
		addFootnote( f, pdfData, renderOpts, doc );

	return { ret, firstLine };
}

void
PdfRenderer::addFootnote( std::shared_ptr< MD::Footnote< MD::QStringTrait > > f, PdfAuxData & pdfData,
	const RenderOpts & renderOpts, std::shared_ptr< MD::Document< MD::QStringTrait > > doc )
{
	PdfAuxData tmpData = pdfData;
	tmpData.coords = { { pdfData.coords.margins.left, pdfData.coords.margins.right,
			pdfData.coords.margins.top, pdfData.coords.margins.bottom },
		pdfData.page->GetPageSize().GetWidth(),
		pdfData.page->GetPageSize().GetHeight(),
		pdfData.coords.margins.left, pdfData.page->GetPageSize().GetHeight() -
			pdfData.coords.margins.top };

	double lineHeight = 0.0;
	auto h = footnoteHeight( tmpData, renderOpts,
		doc, f.get(), &lineHeight );

	reserveSpaceForFootnote( pdfData, renderOpts, h, pdfData.coords.y,
		pdfData.currentPageIdx, lineHeight );

	m_footnotes.append( f );
}

QPair< QVector< WhereDrawn >, WhereDrawn >
PdfRenderer::drawTableRow( QVector< QVector< CellData > > & table, int row, PdfAuxData & pdfData,
	double offset, double lineHeight, const RenderOpts & renderOpts,
	std::shared_ptr< MD::Document< MD::QStringTrait > > doc, QVector< std::shared_ptr< MD::Footnote< MD::QStringTrait > > > & footnotes,
	float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return {};
	}

	emit status( tr( "Drawing table row." ) );

	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );

	const auto startPage = pdfData.currentPageIndex();
	const auto startY = pdfData.coords.y;
	auto endPage = startPage;
	auto endY = startY;
	int currentPage = startPage;
	const auto firstLinePageIdx = startPage;
	const auto firstLineHeight = lineHeight;
	const auto firstLineY = startY - firstLineHeight - c_tableMargin;

	TextToDraw text;
	QMap< QString, QVector< QPair< QRectF, int > > > links;

	int column = 0;

	// Draw cells.
	for( auto it = table.cbegin(), last = table.cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return {};
		}

		emit status( tr( "Drawing table cell." ) );

		text.alignment = it->at( 0 ).alignment;
		text.availableWidth = it->at( 0 ).width;
		text.lineHeight = lineHeight;

		pdfData.painter->SetPage( pdfData.doc->GetPage( startPage ) );

		currentPage = startPage;

		auto startX = pdfData.coords.margins.left + offset;

		for( int i = 0; i < column; ++i )
			startX += table[ i ][ 0 ].width + c_tableMargin * 2.0;

		startX += c_tableMargin;

		double x = startX;
		double y = startY - c_tableMargin;

		if( y < pdfData.currentPageAllowedY() && qAbs( y - pdfData.currentPageAllowedY() ) > 0.1 )
		{
			newPageInTable( pdfData, currentPage, endPage, endY );

			y = pdfData.topY( currentPage );

			if( pdfData.drawFootnotes )
				y -= pdfData.extraInFootnote;
		}

		bool textBefore = false;

		for( auto c = it->at( row ).items.cbegin(), clast = it->at( row ).items.cend(); c != clast; ++c )
		{
			if( !c->image.isNull() && !text.text.isEmpty() )
				drawTextLineInTable( x, y, text, lineHeight, pdfData, links, textFont, currentPage,
					endPage, endY, footnotes, inFootnote, scale );

			if( !c->image.isNull() )
			{
				if( textBefore )
					y -= lineHeight;

				PdfImage img( pdfData.doc );
				img.SetDpi( pdfData.m_dpi );
				img.LoadFromData( reinterpret_cast< const unsigned char * >( c->image.data() ),
					c->image.size() );

				auto ratio = it->at( 0 ).width / img.GetWidth() * scale;

				auto h = img.GetHeight() * ratio;

				if(  y - h < pdfData.currentPageAllowedY() &&
					qAbs( y - h - pdfData.currentPageAllowedY() ) > 0.1 )
				{
					newPageInTable( pdfData, currentPage, endPage, endY );

					y = pdfData.topY( currentPage );

					if( pdfData.drawFootnotes )
						y -= pdfData.extraInFootnote;
				}

				const auto availableHeight = pdfData.topY( currentPage ) -
					pdfData.currentPageAllowedY();

				if( h > availableHeight )
					ratio = availableHeight / img.GetHeight();

				const auto w = img.GetWidth() * ratio;
				auto o = 0.0;

				if( w < table[ column ][ 0 ].width )
					o = ( table[ column ][ 0 ].width - w ) / 2.0;

				y -= img.GetHeight() * ratio;

				pdfData.drawImage( x + o, y, &img, ratio, ratio );

				if( !c->url.isEmpty() )
					links[ c->url ].append( qMakePair( QRectF( x, y,
							c->width( pdfData, this, scale ),
							img.GetHeight() * ratio ),
						currentPage ) );

				textBefore = false;
			}
			else
			{
				auto * font = createFont( c->font.family, c->font.bold, c->font.italic,
					c->font.size, pdfData.doc, scale, pdfData );
				font->SetStrikeOut( c->font.strikethrough );

				auto w = font->GetFontMetrics()->StringWidth(
					createPdfString( c->word.isEmpty() ? c->url : c->word ) );
				double s = 0.0;

				if( !text.text.isEmpty() )
				{
					if( text.text.last().font == c->font )
						s = font->GetFontMetrics()->StringWidth( PdfString( " " ) );
					else
						s = textFont->GetFontMetrics()->StringWidth( PdfString( " " ) );
				}

				double fw = 0.0;

				if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
				{
					auto * f1 = createFont( ( c + 1 )->font.family, ( c + 1 )->font.bold,
						( c + 1 )->font.italic, ( c + 1 )->font.size, pdfData.doc, scale, pdfData );
					f1->SetStrikeOut( ( c + 1 )->font.strikethrough );

					const auto old = f1->GetFontSize();
					f1->SetFontSize( old * scale );
					fw = f1->GetFontMetrics()->StringWidth( createPdfString(
						( c + 1 )->footnote ) );
					w += fw;
					f1->SetFontSize( old );
				}

				if( text.width + s + w < it->at( 0 ).width ||
					qAbs( text.width + s + w - it->at( 0 ).width ) < 0.01 )
				{
					text.text.append( *c );

					if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
						text.text.append( *( c + 1 ) );

					text.width += s + w;
				}
				else
				{
					if( !text.text.isEmpty() )
					{
						drawTextLineInTable( x, y, text, lineHeight, pdfData, links,
							textFont, currentPage, endPage, endY, footnotes, inFootnote, scale );
						text.text.append( *c );

						if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
							text.text.append( *( c + 1 ) );

						text.width += w;
					}
					else
					{
						text.text.append( *c );
						text.width += w;
						drawTextLineInTable( x, y, text, lineHeight, pdfData, links,
							textFont, currentPage, endPage, endY, footnotes, inFootnote, scale );

						if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
						{
							text.text.append( *( c + 1 ) );
							text.width += fw;
						}
					}
				}

				if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
					++c;

				textBefore = true;
			}
		}

		if( !text.text.isEmpty() )
			drawTextLineInTable( x, y, text, lineHeight, pdfData, links, textFont, currentPage,
				endPage, endY, footnotes, inFootnote, scale );

		y -= c_tableMargin - textFont->GetFontMetrics()->GetDescent();

		if( y < endY  && currentPage == pdfData.currentPageIndex() )
			endY = y;

		++column;
	}

	drawRowBorder( pdfData, startPage, ret, renderOpts, offset, table, startY, endY );

	pdfData.coords.y = endY;
	pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIndex() ) );

	processLinksInTable( pdfData, links, doc );

	return { ret, { firstLinePageIdx, firstLineY, firstLineHeight } };
}

void
PdfRenderer::drawRowBorder( PdfAuxData & pdfData, int startPage, QVector< WhereDrawn > & ret,
	const RenderOpts & renderOpts, double offset, const QVector< QVector< CellData > > & table,
	double startY, double endY )
{
	for( int i = startPage; i <= pdfData.currentPageIndex(); ++i )
	{
		pdfData.painter->SetPage( pdfData.doc->GetPage( i ) );

		pdfData.setColor( renderOpts.m_borderColor );

		const auto startX = pdfData.coords.margins.left + offset;
		auto endX = startX;

		for( int c = 0; c < table.size(); ++ c )
			endX += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

		if( i == startPage )
		{
			pdfData.drawLine( startX, startY, endX, startY );

			auto x = startX;
			auto y = endY;

			if( i == pdfData.currentPageIndex() )
			{
				pdfData.drawLine( startX, endY, endX, endY );
				pdfData.drawLine( x, startY, x, endY );
			}
			else
			{
				pdfData.drawLine( x, startY, x, pdfData.allowedY( i ) );
				y = pdfData.allowedY( i );
			}

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.drawLine( x, startY, x, y );
			}

			ret.append( { i, ( i < pdfData.currentPageIndex() ? pdfData.allowedY( i ) : endY ),
				( i < pdfData.currentPageIndex() ? startY - pdfData.allowedY( i ) : startY - endY  ) } );
		}
		else if( i < pdfData.currentPageIndex() )
		{
			auto x = startX;
			auto y = pdfData.allowedY( i );
			auto sy = pdfData.topY( i );

			if( pdfData.drawFootnotes )
				sy -= pdfData.extraInFootnote + c_tableMargin;

			pdfData.drawLine( x, sy, x, y );

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.drawLine( x, sy, x, y );
			}

			ret.append( { i, pdfData.allowedY( i ),
				sy - pdfData.allowedY( i ) } );
		}
		else
		{
			auto x = startX;
			auto y = endY;
			auto sy = pdfData.topY( i );

			if( pdfData.drawFootnotes )
				sy -= pdfData.extraInFootnote + c_tableMargin;

			pdfData.drawLine( x, sy, x, y );

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.drawLine( x, sy, x, y );
			}

			pdfData.drawLine( startX, y, endX, y );

			ret.append( { pdfData.currentPageIndex(), endY,
				sy - endY } );
		}

		pdfData.restoreColor();
	}
}

void
PdfRenderer::drawTextLineInTable( double x, double & y, TextToDraw & text, double lineHeight,
	PdfAuxData & pdfData, QMap< QString, QVector< QPair< QRectF, int > > > & links,
	PdfFont * font, int & currentPage, int & endPage, double & endY,
	QVector< std::shared_ptr< MD::Footnote< MD::QStringTrait > > > & footnotes, bool inFootnote, float scale )
{
	y -= lineHeight;

	if( y < pdfData.allowedY( currentPage ) )
	{
		newPageInTable( pdfData, currentPage, endPage, endY );

		y = pdfData.topY( currentPage ) - lineHeight;

		if( pdfData.drawFootnotes )
			y -= pdfData.extraInFootnote;
	}

	if( text.width < text.availableWidth ||
		qAbs( text.width - text.availableWidth ) < 0.01 )
	{
		switch( text.alignment )
		{
			case MD::Table< MD::QStringTrait >::AlignRight :
				x = x + text.availableWidth - text.width;
				break;

			case MD::Table< MD::QStringTrait >::AlignCenter :
				x = x + ( text.availableWidth - text.width ) / 2.0;
				break;

			default :
				break;
		}
	}
	else
	{
		const auto str = ( text.text.first().word.isEmpty() ? text.text.first().url :
			text.text.first().word );

		QString res;

		double w = 0.0;

		auto * f = createFont( text.text.first().font.family, text.text.first().font.bold,
			text.text.first().font.italic, text.text.first().font.size, pdfData.doc, scale, pdfData );
		f->SetStrikeOut( text.text.first().font.strikethrough );
		auto * fm = f->GetFontMetrics();

		for( const auto & ch : str )
		{
			w += fm->UnicodeCharWidth( ch.unicode() );

			if( w >= text.availableWidth )
				break;
			else
				res.append( ch );
		}

		text.text.first().word = res;
	}

	for( auto it = text.text.cbegin(), last = text.text.cend(); it != last; ++it )
	{
		auto * f = createFont( it->font.family, it->font.bold,
			it->font.italic, it->font.size, pdfData.doc, scale, pdfData );
		f->SetStrikeOut( it->font.strikethrough );

		if( it->background.isValid() )
		{
			pdfData.setColor( it->background );

			pdfData.drawRectangle( x, y + f->GetFontMetrics()->GetDescent(),
				it->width( pdfData, this, scale ), f->GetFontMetrics()->GetLineSpacing() );

			pdfData.painter->Fill();

			pdfData.restoreColor();
		}

		if( it->color.isValid() )
			pdfData.setColor( it->color );

		pdfData.painter->SetFont( f );
		pdfData.drawText( x, y, createPdfString( it->word.isEmpty() ?
			it->url : it->word ) );

		pdfData.restoreColor();

		if( !it->url.isEmpty() )
			links[ it->url ].append( qMakePair( QRectF( x, y, it->width( pdfData, this, scale ),
				lineHeight ), currentPage ) );

		x += it->width( pdfData, this, scale );

		if( !inFootnote )
		{
			if( it + 1 != last && !( it + 1 )->footnote.isEmpty() )
			{
				++it;

				const auto str = createPdfString( it->footnote );

				const auto old = f->GetFontSize();
				f->SetFontSize( old * c_footnoteScale );

				const auto w = f->GetFontMetrics()->StringWidth( str );

				pdfData.drawText( x, y + lineHeight -
					f->GetFontMetrics()->GetLineSpacing(), str );
				f->SetFontSize( old );

				x += w;

				footnotes.append( it->footnoteObj );
			}
		}

		if( it + 1 != last )
		{
			auto tmpX = x;

			if( it->background.isValid() && it->font == ( it + 1 )->font )
			{
				pdfData.setColor( it->background );

				const auto sw = f->GetFontMetrics()->StringWidth( PdfString( " " ) );

				pdfData.drawRectangle( x, y + f->GetFontMetrics()->GetDescent(),
					sw, f->GetFontMetrics()->GetLineSpacing() );

				x += sw;

				pdfData.painter->Fill();

				pdfData.restoreColor();
			}
			else
				x += font->GetFontMetrics()->StringWidth( PdfString( " " ) );

			if( !( it + 1 )->url.isEmpty() && it->url == ( it + 1 )->url )
				links[ it->url ].append( qMakePair( QRectF( tmpX, y, x - tmpX, lineHeight ),
					currentPage ) );
		}
	}

	text.clear();
}

void
PdfRenderer::newPageInTable( PdfAuxData & pdfData, int & currentPage, int & endPage,
	double & endY )
{
	if( currentPage + 1 > pdfData.currentPageIndex() )
	{
		createPage( pdfData );

		if( pdfData.currentPageIndex() > endPage )
		{
			endPage = pdfData.currentPageIndex();
			endY = pdfData.coords.y;
		}

		++currentPage;
	}
	else
	{
		++currentPage;

		pdfData.painter->SetPage( pdfData.doc->GetPage( currentPage ) );
	}
}

void
PdfRenderer::processLinksInTable( PdfAuxData & pdfData,
	const QMap< QString, QVector< QPair< QRectF, int > > > & links,
	std::shared_ptr< MD::Document< MD::QStringTrait > > doc )
{
	for( auto it = links.cbegin(), last = links.cend(); it != last; ++it )
	{
		QString url = it.key();

		const auto lit = doc->labeledLinks().find( url );

		if( lit != doc->labeledLinks().cend() )
			url = lit->second->url();

		auto tmp = it.value();

		if( !tmp.isEmpty() )
		{
			QVector< QPair< QRectF, int > > rects;
			QPair< QRectF, int > r = tmp.first();

			for( auto rit = tmp.cbegin() + 1, rlast = tmp.cend(); rit != rlast; ++rit )
			{
				if( r.second == rit->second &&
					qAbs( r.first.x() + r.first.width() - rit->first.x() ) < 0.001 &&
					qAbs( r.first.y() - rit->first.y() ) < 0.001 )
				{
					r.first.setWidth( r.first.width() + rit->first.width() );
				}
				else
				{
					rects.append( r );
					r = *rit;
				}
			}

			rects.append( r );

			if( !QUrl( url ).isRelative() )
			{
				for( const auto & r : qAsConst( rects ) )
				{
					auto * annot = pdfData.doc->GetPage( r.second )->CreateAnnotation( ePdfAnnotation_Link,
						PdfRect( r.first.x(), r.first.y(), r.first.width(), r.first.height() ) );
					annot->SetBorderStyle( 0.0, 0.0, 0.0 );

					PdfAction action( ePdfAction_URI, pdfData.doc );
					action.SetURI( PdfString( url.toLatin1().data() ) );

					annot->SetAction( action );
				}
			}
			else
				m_unresolvedLinks.insert( url, rects );
		}
	}
}

double
PdfRenderer::minNecessaryHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	std::shared_ptr< MD::Item< MD::QStringTrait > > item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
	double offset, float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	PdfAuxData tmp = pdfData;
	tmp.coords.y = tmp.coords.pageHeight - tmp.coords.margins.top;
	tmp.coords.x = tmp.coords.margins.left + offset;

	switch( item->type() )
	{
		case MD::ItemType::Heading :
			return 0.0;

		case MD::ItemType::Paragraph :
		{
			ret = drawParagraph( tmp, renderOpts, static_cast< MD::Paragraph< MD::QStringTrait >* > ( item.get() ),
				doc, offset, true, CalcHeightOpt::Minimum, scale, inFootnote ).first;
		}
			break;

		case MD::ItemType::Code :
		{
			ret = drawCode( tmp, renderOpts, static_cast< MD::Code< MD::QStringTrait >* > ( item.get() ),
				doc, offset, CalcHeightOpt::Minimum, scale ).first;
		}
			break;

		case MD::ItemType::Blockquote :
		{
			ret = drawBlockquote( tmp, renderOpts,
				static_cast< MD::Blockquote< MD::QStringTrait >* > ( item.get() ),
				doc, offset, CalcHeightOpt::Minimum, scale, inFootnote ).first;
		}
			break;

		case MD::ItemType::List :
		{
			auto * list = static_cast< MD::List< MD::QStringTrait >* > ( item.get() );
			const auto bulletWidth = maxListNumberWidth( list );

			ret = drawList( tmp, m_opts, list, m_doc, bulletWidth, offset,
				CalcHeightOpt::Minimum, scale, inFootnote ).first;
		}
			break;

		case MD::ItemType::Table :
		{
			ret = drawTable( tmp, renderOpts,
				static_cast< MD::Table< MD::QStringTrait >* > ( item.get() ),
				doc, offset, CalcHeightOpt::Minimum, scale, inFootnote ).first;
		}
			break;

		default :
			break;
	}

	if( !ret.isEmpty() )
		return ret.constFirst().height;
	else
		return 0.0;
}
