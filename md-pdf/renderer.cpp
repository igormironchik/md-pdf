
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

	for( int i = 0, last = m_width.size(); i < last; ++i )
	{
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

			w = 0.0;
			sw = 0.0;
			ww = 0.0;
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
PdfRenderer::CellItem::width( PdfAuxData & pdfData ) const
{
	if( !word.isEmpty() )
		return font->GetFontMetrics()->StringWidth( createPdfString( word ) );
	else if( !image.isNull() )
	{
		PdfImage pdfImg( pdfData.doc );
		pdfImg.LoadFromData( reinterpret_cast< const unsigned char * >( image.data() ),
			image.size() );

		return pdfImg.GetWidth();
	}
	else if( !url.isEmpty() )
		return font->GetFontMetrics()->StringWidth( createPdfString( url ) );
	else if( !footnote.isEmpty() )
	{
		const auto old = font->GetFontSize();
		font->SetFontSize( old * c_footnoteScale );
		const auto w = font->GetFontMetrics()->StringWidth( createPdfString(
			footnote ) );
		font->SetFontSize( old );

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
	PdfAuxData & pdfData )
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

			w += it->width( pdfData );

			if( w >= width )
			{
				newLine = true;
				continue;
			}

			double sw = spaceWidth;

			if( it != items.cbegin() && it->font != ( it - 1 )->font )
				sw = it->font->GetFontMetrics()->StringWidth( PdfString( " " ) );

			if( it + 1 != last && !( it + 1 )->footnote.isEmpty() )
				sw = 0.0;

			if( it + 1 != last )
			{
				if( w + sw + ( it + 1 )->width( pdfData ) > width )
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
{
	connect( this, &PdfRenderer::start, this, &PdfRenderer::renderImpl,
		Qt::QueuedConnection );
}

void
PdfRenderer::render( const QString & fileName, QSharedPointer< MD::Document > doc,
	const RenderOpts & opts )
{
	m_fileName = fileName;
	m_doc = doc;
	m_opts = opts;

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
			pdfData.drawingsStream->setCodec( QTextCodec::codecForName( "UTF-8" ) );
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
						drawHeading( pdfData, m_opts, static_cast< MD::Heading* > ( it->data() ),
							m_doc, 0.0,
							// If there is another item after heading we need to know its min
							// height to glue heading with it.
							( it + 1 != last ?
								minNecessaryHeight( pdfData, m_opts, *( it + 1 ), m_doc, 0.0,
									1.0, false ) :
								0.0 ), CalcHeightOpt::Unknown, 1.0 );
						break;

					case MD::ItemType::Paragraph :
						drawParagraph( pdfData, m_opts, static_cast< MD::Paragraph* > ( it->data() ),
							m_doc, 0.0, true, CalcHeightOpt::Unknown, 1.0, false );
						break;

					case MD::ItemType::Code :
						drawCode( pdfData, m_opts, static_cast< MD::Code* > ( it->data() ),
							m_doc, 0.0, CalcHeightOpt::Unknown, 1.0 );
						break;

					case MD::ItemType::Blockquote :
						drawBlockquote( pdfData, m_opts,
							static_cast< MD::Blockquote* > ( it->data() ),
							m_doc, 0.0, CalcHeightOpt::Unknown, 1.0, false );
						break;

					case MD::ItemType::List :
					{
						auto * list = static_cast< MD::List* > ( it->data() );
						const auto bulletWidth = maxListNumberWidth( list );

						auto * font = createFont( m_opts.m_textFont, false, false,
							m_opts.m_textFontSize, pdfData.doc, 1.0, pdfData );
						pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

						drawList( pdfData, m_opts, list, m_doc, bulletWidth );
					}
						break;

					case MD::ItemType::Table :
						drawTable( pdfData, m_opts,
							static_cast< MD::Table* > ( it->data() ),
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
						auto * a = static_cast< MD::Anchor* > ( it->data() );
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
				pdfData.coords.y = pdfData.topFootnoteY( pdfData.footnotePageIdx ) -
					pdfData.extraInFootnote;

				pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.footnotePageIdx ) );

				drawHorizontalLine( pdfData, m_opts );

				for( const auto & f : qAsConst( m_footnotes ) )
					drawFootnote( pdfData, m_opts, m_doc, f.data(), CalcHeightOpt::Unknown );
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

			emit error( e.what() );
		}

#ifdef MD_PDF_TESTING
		if( m_opts.printDrawings )
			pdfData.drawingsFile->close();
#endif // MD_PDF_TESTING
	}

	try {
		clean();
	}
	catch( const PdfError & e )
	{
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
		auto it = pdfData.reserved.find( pdfData.footnotePageIdx );

		if( it != pdfData.reserved.end() )
			++it;

		if( it != pdfData.reserved.end() )
			pdfData.footnotePageIdx = it.key();
		else
			++pdfData.footnotePageIdx;


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

QVector< WhereDrawn >
PdfRenderer::drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Heading * item, QSharedPointer< MD::Document > doc, double offset,
	double nextItemMinHeight, CalcHeightOpt heightCalcOpt, float scale )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing heading." ) );

	PdfFont * font = createFont( renderOpts.m_textFont.toLocal8Bit().data(),
		true, false, renderOpts.m_textFontSize + 16 - ( item->level() < 7 ? item->level() * 2 : 12 ),
		pdfData.doc, scale, pdfData );

	pdfData.painter->SetFont( font );

	const double width = pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset;

	const auto lines = pdfData.painter->GetMultiLineTextAsLines(
		width, createPdfString( item->text() ) );

	const double height = lines.size() * font->GetFontMetrics()->GetLineSpacing();
	const double availableHeight = pdfData.coords.pageHeight -
		pdfData.coords.margins.top - pdfData.coords.margins.bottom;

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		case CalcHeightOpt::Full :
		{
			for( std::size_t i = 0; i < lines.size(); ++i )
				ret.append( { -1, 0.0, font->GetFontMetrics()->GetLineSpacing() } );
			return ret;
		}

		default :
			break;
	}

	if( !pdfData.firstOnPage || pdfData.drawFootnotes )
		pdfData.coords.y -= c_beforeHeading;

	// If heading can be placed with next item on current page.
	if( pdfData.coords.y - height - nextItemMinHeight > pdfData.currentPageAllowedY() )
	{
		pdfData.drawMultiLineText( pdfData.coords.margins.left + offset,
			pdfData.coords.y - height,
			width, height, createPdfString( item->text() ) );

		if( !item->label().isEmpty() )
			m_dests.insert( item->label(), PdfDestination( pdfData.page,
				PdfRect( pdfData.coords.margins.left + offset,
					pdfData.coords.y - font->GetFontMetrics()->GetLineSpacing(),
					width, font->GetFontMetrics()->GetLineSpacing() ) ) );

		pdfData.coords.y -= height;

		ret.append( { pdfData.currentPageIndex(), pdfData.coords.y, height } );

		return ret;
	}
	// If heading can be placed with next item on empty page.
	else if( height + nextItemMinHeight <= availableHeight )
	{
		createPage( pdfData );
		pdfData.freeSpaceOn( pdfData.currentPageIdx );
		return drawHeading( pdfData, renderOpts, item, doc, offset, nextItemMinHeight,
			heightCalcOpt, scale );
	}
	// Otherwise we need to split heading to place it on different pages.
	else
	{
		std::vector< PdfString > tmp;
		double h = 0.0;
		std::size_t i = 0;
		double available = pdfData.topY( pdfData.currentPageIndex() ) -
			pdfData.currentPageAllowedY();
		const double spacing = font->GetFontMetrics()->GetLineSpacing();

		while( i < lines.size() && available >= spacing )
		{
			tmp.push_back( lines.at( i ) );
			h += spacing;
			++i;
			available -= spacing;
		}

		QString text;

		for( const auto & s : tmp )
			text.append( createQString( s ) );

		QString toSave = item->text();
		toSave.remove( text );

		item->setText( toSave.simplified() );

		pdfData.drawMultiLineText( pdfData.coords.margins.left + offset,
			pdfData.coords.y - h,
			width, h, createPdfString( text ) );

		if( !item->label().isEmpty() )
			m_dests.insert( item->label(), PdfDestination( pdfData.page,
				PdfRect( pdfData.coords.margins.left + offset,
					pdfData.coords.y - font->GetFontMetrics()->GetLineSpacing(),
					width, font->GetFontMetrics()->GetLineSpacing() ) ) );

		pdfData.coords.y -= height;

		ret.append( { pdfData.currentPageIndex(), pdfData.coords.y, height } );

		if( !item->text().isEmpty() )
		{
			createPage( pdfData );

			ret.append( drawHeading( pdfData, renderOpts, item, doc, offset, nextItemMinHeight,
				heightCalcOpt, scale ) );
		}

		return ret;
	}
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Text * item, QSharedPointer< MD::Document > doc, bool & newLine,
	PdfFont * footnoteFont, float footnoteFontScale, MD::Item * nextItem, int footnoteNum,
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
	MD::Link * item, QSharedPointer< MD::Document > doc, bool & newLine,
	PdfFont * footnoteFont, float footnoteFontScale, MD::Item * nextItem, int footnoteNum,
	double offset, bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote )
{
	QVector< QPair< QRectF, int > > rects;

	QString url = item->url();

	if( doc->labeledLinks().contains( url ) )
		url = doc->labeledLinks()[ url ]->url();

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

	// If text link.
	if( item->img()->isEmpty() )
	{
		pdfData.setColor( renderOpts.m_linkColor );

		auto * font = createFont( renderOpts.m_textFont, item->textOptions() & MD::TextOption::BoldText,
			item->textOptions() & MD::TextOption::ItalicText, renderOpts.m_textFontSize,
			pdfData.doc, scale, pdfData );

		if( item->textOptions() & MD::TextOption::StrikethroughText )
			font->SetStrikeOut( true );
		else
			font->SetStrikeOut( false );

		rects = normalizeRects( drawString( pdfData, renderOpts,
			( !item->text().isEmpty() ? item->text() : url ),
			createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
				pdfData.doc, scale, pdfData ),
			font, font->GetFontMetrics()->GetLineSpacing(),
			doc, newLine, footnoteFont, footnoteFontScale, nextItem, footnoteNum, offset,
			firstInParagraph, cw, QColor(), inFootnote ) );

		pdfData.restoreColor();
	}
	// Otherwise image link.
	else
		rects.append( drawImage( pdfData, renderOpts, item->img().data(), doc, newLine, offset,
			firstInParagraph, cw, scale ) );

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
	QSharedPointer< MD::Document > doc, bool & newLine, PdfFont * footnoteFont,
	float footnoteFontScale, MD::Item * nextItem, int footnoteNum, double offset,
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
		doc->footnotesMap().contains( static_cast< MD::FootnoteRef* > ( nextItem )->id() ) &&
		!inFootnote )
			footnoteAtEnd = true;

	if( footnoteAtEnd )
	{
		const auto old = footnoteFont->GetFontSize();
		footnoteFont->SetFontSize( old * footnoteFontScale );
		footnoteWidth = footnoteFont->GetFontMetrics()->StringWidth( createPdfString(
			QString::number( footnoteNum ) ) );
		footnoteFont->SetFontSize( old );
	}

	auto newLineFn = [&] ()
	{
		newLine = true;

		if( draw )
		{
			moveToNewLine( pdfData, offset, lineHeight, 1.0 );

			if( cw )
				cw->moveToNextLine();
		}
		else if( cw )
		{
			cw->append( { 0.0, lineHeight, false, true, true, "" } );
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

	// We need to draw space char if previous word with comma.
	if( !firstInParagraph && !newLine && !words.isEmpty() &&
		!charsWithoutSpaceBefore.contains( words.first() ) )
	{
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

				pdfData.drawText( pdfData.coords.x, pdfData.coords.y, " " );

				spaceFont->SetFontScale( 100.0 );
			}
			else if( cw )
				cw->append( { w, lineHeight, true, false, true, " " } );

			ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
				w * scale / 100.0, lineHeight ), pdfData.currentPageIndex() ) );

			pdfData.coords.x += w * scale / 100.0;
		}
		else
			newLineFn();
	}

	pdfData.painter->SetFont( font );

	// Draw words.
	for( auto it = words.begin(), last = words.end(); it != last; ++it )
	{
		if( *it == QStringLiteral( "including" ) )
		{
			int i = 0;
			++i;
		}
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
						font->GetFontMetrics()->GetDescent(), length,
						font->GetFontMetrics()->GetLineSpacing() );
					pdfData.painter->Fill();
					pdfData.restoreColor();
				}

				pdfData.drawText( pdfData.coords.x, pdfData.coords.y, str );
				ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
					length, lineHeight ), pdfData.currentPageIndex() ) );
			}
			else if( cw )
				cw->append( { length + ( it + 1 == last && footnoteAtEnd ? footnoteWidth : 0.0 ),
					lineHeight, false, false, true, *it } );

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

						ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
							spaceWidth * scale / 100.0, lineHeight ), pdfData.currentPageIndex() ) );

						if( background.isValid() )
						{
							pdfData.setColor( background );
							pdfData.drawRectangle( pdfData.coords.x, pdfData.coords.y +
								font->GetFontMetrics()->GetDescent(), spaceWidth * scale / 100.0,
								font->GetFontMetrics()->GetLineSpacing() );
							pdfData.painter->Fill();
							pdfData.restoreColor();
						}

						pdfData.drawText( pdfData.coords.x, pdfData.coords.y, " " );

						font->SetFontScale( 100.0 );
					}
					else if( cw )
						cw->append( { spaceWidth, lineHeight, true, false, true, " " } );

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
					pdfData.drawText( pdfData.coords.x, pdfData.coords.y, str );
					ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
							font->GetFontMetrics()->StringWidth( str ), lineHeight ),
						pdfData.currentPageIndex() ) );
				}
				else if( cw )
					cw->append( { font->GetFontMetrics()->StringWidth( str ),
						lineHeight, false, false, true, *it } );

				newLineFn();

				if( cw && it + 1 == last && footnoteAtEnd )
					cw->append( { footnoteWidth, lineHeight, false, false, true,
						QString::number( footnoteNum ) } );
			}
		}
	}

	return ret;
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
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

QVector< WhereDrawn >
PdfRenderer::drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Paragraph * item, QSharedPointer< MD::Document > doc, double offset, bool withNewLine,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote )
{
	QVector< QPair< QRectF, int > > rects;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return QVector< WhereDrawn > ();
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing paragraph." ) );

	auto * font = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

	auto * footnoteFont = font;

	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	pdfData.lineHeight = lineHeight;

	const auto oldPageId = pdfData.footnotePageIdx;

	if( ( withNewLine && !pdfData.firstOnPage && heightCalcOpt == CalcHeightOpt::Unknown ) ||
		( withNewLine && pdfData.drawFootnotes && heightCalcOpt == CalcHeightOpt::Unknown ) )
			moveToNewLine( pdfData, offset, lineHeight, 2.0 );

	pdfData.coords.x = pdfData.coords.margins.left + offset;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		if( pdfData.coords.y < pdfData.coords.margins.bottom )
		{
			createPage( pdfData );

			pdfData.coords.x = pdfData.coords.margins.left + offset;
		}
	}

	if( pdfData.drawFootnotes && oldPageId != pdfData.footnotePageIdx && withNewLine )
		moveToNewLine( pdfData, offset, lineHeight, 2.0 );

	bool newLine = false;
	CustomWidth cw;
	auto y = pdfData.coords.y;

	auto footnoteNum = m_footnoteNum;

	bool lineBreak = false;

	// Calculate words/lines/spaces widthes.
	for( auto it = item->items().begin(), last = item->items().end(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return QVector< WhereDrawn > ();
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Text :
				drawText( pdfData, renderOpts, static_cast< MD::Text* > ( it->data() ),
					doc, newLine, footnoteFont, c_footnoteScale,
					( it + 1 != last ? ( it + 1 )->data() : nullptr ),
					footnoteNum, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Code :
				drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code* > ( it->data() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Link :
				drawLink( pdfData, renderOpts, static_cast< MD::Link* > ( it->data() ),
					doc, newLine, footnoteFont, c_footnoteScale,
					( it + 1 != last ? ( it + 1 )->data() : nullptr ),
					footnoteNum, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote );
				lineBreak = false;
				break;

			case MD::ItemType::Image :
				drawImage( pdfData, renderOpts, static_cast< MD::Image* > ( it->data() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale );
				lineBreak = false;
				break;

			case MD::ItemType::LineBreak :
			{
				lineBreak = true;
				cw.append( { 0.0, lineHeight, false, true, false, "" } );
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

	cw.append( { 0.0, lineHeight, false, true, false, "" } );

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		{
			QVector< WhereDrawn > r;
			r.append( { 0, 0.0,
				( ( withNewLine && !pdfData.firstOnPage ) ||
					( withNewLine && pdfData.drawFootnotes ) ?
						lineHeight + cw.firstItemHeight() :
						cw.firstItemHeight() ) } );

			return r;
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
					r.append( { 0, 0.0, max } );
					max = 0.0;
					h = 0.0;
				}
			}

			return r;
		}

		default :
			break;
	}

	cw.calcScale( pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset );

	cw.setDrawing();

	newLine = false;
	pdfData.coords.y = y;
	pdfData.coords.x = pdfData.coords.margins.left + offset;

	pdfData.continueParagraph = true;

	lineBreak = false;

	// Actual drawing.
	for( auto it = item->items().begin(), last = item->items().end(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return QVector< WhereDrawn > ();
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Text :
				rects.append( drawText( pdfData, renderOpts, static_cast< MD::Text* > ( it->data() ),
					doc, newLine, nullptr, 1.0, nullptr, m_footnoteNum,
					offset, ( it == item->items().begin() || lineBreak ), &cw, scale, inFootnote ) );
				lineBreak = false;
				break;

			case MD::ItemType::Code :
				rects.append( drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code* > ( it->data() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale,
					inFootnote ) );
				lineBreak = false;
				break;

			case MD::ItemType::Link :
				rects.append( drawLink( pdfData, renderOpts, static_cast< MD::Link* > ( it->data() ),
					doc, newLine, nullptr, 1.0, nullptr, m_footnoteNum,
					offset, ( it == item->items().begin() || lineBreak ), &cw, scale, inFootnote ) );
				lineBreak = false;
				break;

			case MD::ItemType::Image :
				rects.append( drawImage( pdfData, renderOpts, static_cast< MD::Image* > ( it->data() ),
					doc, newLine, offset, ( it == item->items().begin() || lineBreak ), &cw, scale ) );
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
					auto * ref = static_cast< MD::FootnoteRef* > ( it->data() );

					if( doc->footnotesMap().contains( ref->id()  ) )
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

						auto f = doc->footnotesMap()[ ref->id() ];

						addFootnote( f, pdfData, renderOpts, doc );
					}
				}
			}
				break;

			default :
				break;
		}
	}

	pdfData.continueParagraph = false;

	return toWhereDrawn( normalizeRects( rects ), pdfData.coords.pageHeight );
}

void
PdfRenderer::reserveSpaceForFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	const QVector< WhereDrawn > & h, const double & currentY, int currentPage )
{
	const auto topY = pdfData.topFootnoteY( currentPage );
	const auto available = currentY - topY -
		( qAbs( topY ) < 0.01 ? pdfData.coords.margins.bottom : 0.0 );

	auto height = totalHeight( h );
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

		if( pdfData.footnotePageIdx == -1 )
			pdfData.footnotePageIdx = currentPage;
	};

	if( height + extra < available )
		add( height + extra, currentPage );
	else
	{
		height = extra;

		for( int i = 0; i < h.size(); ++i )
		{
			const auto tmp = h[ i ].height;

			if( height + tmp < available )
				height += tmp;
			else
			{
				if( qAbs( height - extra ) > 0.01 )
					add( height, currentPage );

				reserveSpaceForFootnote( pdfData, renderOpts, h.mid( i, h.size() - i ),
					pdfData.coords.pageHeight - pdfData.coords.margins.top,
					currentPage + 1 );

				break;
			}
		}
	}
}

QVector< WhereDrawn >
PdfRenderer::drawFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	QSharedPointer< MD::Document > doc, MD::Footnote * note, CalcHeightOpt heightCalcOpt )
{
	QVector< WhereDrawn > ret;

	static const double c_offset = 2.0;

	auto * font = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, c_footnoteScale, pdfData );
	auto footnoteOffset = c_offset * 2.0 / c_mmInPt +
		font->GetFontMetrics()->StringWidth( createPdfString(
			QString::number( doc->footnotesMap().size() ) ) );

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
					static_cast< MD::Heading* > ( it->data() ),
					doc, footnoteOffset,
					// If there is another item after heading we need to know its min
					// height to glue heading with it.
					( it + 1 != last ?
						minNecessaryHeight( pdfData, renderOpts, *( it + 1 ), doc, 0.0,
							c_footnoteScale, true ) :
						0.0 ), heightCalcOpt, c_footnoteScale ) );
				break;

			case MD::ItemType::Paragraph :
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph* > ( it->data() ), doc, footnoteOffset,
					true, heightCalcOpt, c_footnoteScale, true ) );
				break;

			case MD::ItemType::Code :
				ret.append( drawCode( pdfData, renderOpts, static_cast< MD::Code* > ( it->data() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale ) );
				break;

			case MD::ItemType::Blockquote :
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote* > ( it->data() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale, true ) );
				break;

			case MD::ItemType::List :
			{
				auto * list = static_cast< MD::List* > ( it->data() );
				const auto bulletWidth = maxListNumberWidth( list );

				pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

				ret.append( drawList( pdfData, renderOpts, list, doc, bulletWidth, footnoteOffset,
					heightCalcOpt, c_footnoteScale, true ) );
			}
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table* > ( it->data() ),
					doc, footnoteOffset, heightCalcOpt, c_footnoteScale, true ) );
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

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::footnoteHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	QSharedPointer< MD::Document > doc, MD::Footnote * note )
{
	return drawFootnote( pdfData, renderOpts, doc, note, CalcHeightOpt::Full );
}

QPair< QRectF, int >
PdfRenderer::drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Image * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
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
		cw->append( { 0.0, height, false, true, false, "" } );

		return qMakePair( QRectF(), pdfData.currentPageIndex() );
	}
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
	m_img.loadFromData( m_reply->readAll() );

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
PdfRenderer::loadImage( MD::Image * item )
{
	if( m_imageCache.contains( item->url() ) )
		return m_imageCache[ item->url() ];

	QImage img;

	if( QFileInfo::exists( item->url() ) )
		img = QImage( item->url() );
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

QVector< WhereDrawn >
PdfRenderer::drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code * item, QSharedPointer< MD::Document > doc, double offset, CalcHeightOpt heightCalcOpt,
	float scale )
{
	Q_UNUSED( doc )

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing code." ) );

	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );
	const auto textLHeight = textFont->GetFontMetrics()->GetLineSpacing();

	QStringList lines;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		if( pdfData.coords.y - ( textLHeight * 2.0 ) < pdfData.currentPageAllowedY() )
			createPage( pdfData );
		else
			pdfData.coords.y -= textLHeight * 2.0;

		pdfData.coords.x = pdfData.coords.margins.left + offset;

		lines = item->text().split( QLatin1Char( '\n' ), Qt::KeepEmptyParts );

		for( auto it = lines.begin(), last = lines.end(); it != last; ++it )
			it->replace( QStringLiteral( "\t" ), QStringLiteral( "    " ) );
	}

	auto * font = createFont( renderOpts.m_codeFont, false, false, renderOpts.m_codeFontSize,
		pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	switch( heightCalcOpt )
	{
		case CalcHeightOpt::Minimum :
		{
			QVector< WhereDrawn > r;
			r.append( { 0, 0.0, textLHeight * 2.0 + lineHeight } );

			return r;
		}

		case CalcHeightOpt::Full :
		{
			QVector< WhereDrawn > r;
			r.append( { 0, 0.0, textLHeight * 2.0 + lineHeight } );

			auto i = 1;

			while( i < lines.size() )
			{
				r.append( { 0, 0.0, lineHeight } );
				++i;
			}

			return r;
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
			return ret;
	}

	const auto colored = Syntax::createSyntaxHighlighter( item->syntax() )->prepare( lines );
	int currentWord = 0;
	const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

	while( i < lines.size() )
	{
		auto y = pdfData.coords.y;
		int j = i;
		double h = 0.0;

		while( y - lineHeight > pdfData.currentPageAllowedY() && j < lines.size() )
		{
			h += lineHeight;
			y -= lineHeight;
			++j;
		}

		if( i < j )
		{
			pdfData.setColor( renderOpts.m_codeBackground );
			pdfData.drawRectangle( pdfData.coords.x, y,
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

	pdfData.coords.y -= lineHeight;

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawBlockquote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Blockquote * item, QSharedPointer< MD::Document > doc, double offset,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing blockquote." ) );

	// Draw items.
	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return ret;
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Heading :
			{
				if( heightCalcOpt != CalcHeightOpt::Minimum )
					ret.append( drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading* > ( it->data() ),
						doc, offset + c_blockquoteBaseOffset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ), doc,
								offset + c_blockquoteBaseOffset, scale, inFootnote  ) : 0.0 ),
						heightCalcOpt, scale ) );
				else
				{
					ret.append( { -1, 0.0, 0.0 } );

					return ret;
				}
			}
				break;

			case MD::ItemType::Paragraph :
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, true, heightCalcOpt,
					scale, inFootnote ) );
				break;

			case MD::ItemType::Code :
				ret.append( drawCode( pdfData, renderOpts,
					static_cast< MD::Code* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale ) );
				break;

			case MD::ItemType::Blockquote :
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote ) );
				break;

			case MD::ItemType::List :
			{
				auto * list = static_cast< MD::List* > ( it->data() );
				const auto bulletWidth = maxListNumberWidth( list );

				auto * font = createFont( m_opts.m_textFont, false, false,
					m_opts.m_textFontSize, pdfData.doc, scale, pdfData );
				pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

				ret.append( drawList( pdfData, renderOpts,
					list,
					doc, bulletWidth, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote ) );
			}
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					scale, inFootnote ) );
				break;

			default :
				break;
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			return ret;
	}

	if( heightCalcOpt == CalcHeightOpt::Full )
		return ret;

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

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::List * item, QSharedPointer< MD::Document > doc, int bulletWidth, double offset,
	CalcHeightOpt heightCalcOpt, float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing list." ) );

	int idx = 1;
	ListItemType prevListItemType = ListItemType::Unknown;

	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
			ret.append( drawListItem( pdfData, renderOpts,
				static_cast< MD::ListItem* > ( it->data() ), doc, idx,
				prevListItemType, bulletWidth, offset, heightCalcOpt,
				scale, inFootnote ) );

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			break;
	}

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::ListItem * item, QSharedPointer< MD::Document > doc, int & idx,
	ListItemType & prevListItemType, int bulletWidth, double offset, CalcHeightOpt heightCalcOpt,
	float scale, bool inFootnote )
{
	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		pdfData.painter->SetFont( font );

		if( pdfData.coords.y - lineHeight < pdfData.currentPageAllowedY() )
			createPage( pdfData );

		pdfData.coords.y -= lineHeight;

		const auto orderedListNumberWidth =
			font->GetFontMetrics()->StringWidth( PdfString( "9" ) ) * bulletWidth +
			font->GetFontMetrics()->StringWidth( PdfString( "." ) );
		const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );
		const auto unorderedMarkWidth = spaceWidth * 0.75;

		if( item->listType() == MD::ListItem::Ordered )
		{
			if( prevListItemType == ListItemType::Unordered )
				idx = 1;
			else if( prevListItemType == ListItemType::Ordered )
				++idx;

			prevListItemType = ListItemType::Ordered;

			const QString idxText = QString::number( idx ) + QLatin1Char( '.' );

			pdfData.drawText( pdfData.coords.margins.left + offset,
				pdfData.coords.y, createPdfString( idxText ) );
		}
		else
		{
			prevListItemType = ListItemType::Unordered;

			pdfData.setColor( Qt::black );
			const auto r = unorderedMarkWidth / 2.0;
			pdfData.painter->Circle( pdfData.coords.margins.left + offset + r,
				pdfData.coords.y + unorderedMarkWidth, r );
			pdfData.painter->Fill();
			pdfData.restoreColor();
		}

		offset += orderedListNumberWidth + spaceWidth;
	}

	QVector< WhereDrawn > ret;

	bool addExtraSpace = false;

	for( auto it = item->items().cbegin(), last = item->items().cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return ret;
		}

		switch( (*it)->type() )
		{
			case MD::ItemType::Heading :
			{
				if( heightCalcOpt != CalcHeightOpt::Minimum )
					ret.append( drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading* > ( it->data() ),
						doc, offset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ),  doc, offset,
								scale, inFootnote ) :
							0.0 ),
						heightCalcOpt, scale ) );
				else
					ret.append( { -1, 0.0, 0.0 } );
			}
				break;

			case MD::ItemType::Paragraph :
			{
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph* > ( it->data() ),
					doc, offset, ( it != item->items().cbegin() ), heightCalcOpt,
					scale, inFootnote ) );

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::Code :
			{
				ret.append( drawCode( pdfData, renderOpts,
					static_cast< MD::Code* > ( it->data() ),
					doc, offset, heightCalcOpt, scale ) );

				addExtraSpace = false;
			}
				break;

			case MD::ItemType::Blockquote :
			{
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote* > ( it->data() ),
					doc, offset, heightCalcOpt, scale, inFootnote ) );

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::List :
				ret.append( drawList( pdfData, renderOpts,
					static_cast< MD::List* > ( it->data() ),
					doc, bulletWidth, offset, heightCalcOpt,
					scale, inFootnote ) );
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table* > ( it->data() ),
					doc, offset, heightCalcOpt, scale, inFootnote ) );
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

	return ret;
}

int
PdfRenderer::maxListNumberWidth( MD::List * list ) const
{
	int counter = 0;

	for( auto it = list->items().cbegin(), last = list->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
		{
			auto * item = static_cast< MD::ListItem* > ( it->data() );

			if( item->listType() == MD::ListItem::Ordered )
				++counter;
		}
	}

	for( auto it = list->items().cbegin(), last = list->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == MD::ItemType::ListItem )
		{
			auto * item = static_cast< MD::ListItem* > ( it->data() );

			for( auto lit = item->items().cbegin(), llast = item->items().cend(); lit != llast; ++lit )
			{
				if( (*lit)->type() == MD::ItemType::List )
				{
					auto i = maxListNumberWidth( static_cast< MD::List* > ( lit->data() ) );

					if( i > counter )
						counter = i;
				}
			}
		}
	}

	return ( counter / 10 + 1 );
}

QVector< QVector< PdfRenderer::CellData > >
PdfRenderer::createAuxTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Table * item, QSharedPointer< MD::Document > doc, float scale, bool inFootnote )
{
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
						auto * t = static_cast< MD::Text* > ( it->data() );

						auto * font = createFont( renderOpts.m_textFont,
							t->opts() & MD::TextOption::BoldText,
							t->opts() & MD::TextOption::ItalicText,
							renderOpts.m_textFontSize, pdfData.doc, scale, pdfData );

						if( t->opts() & MD::TextOption::StrikethroughText )
							font->SetStrikeOut( true );
						else
							font->SetStrikeOut( false );

						const auto words = t->text().split( QLatin1Char( ' ' ),
							Qt::SkipEmptyParts );

						for( const auto & w : words )
						{
							CellItem item;
							item.word = w;
							item.font = font;

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Code :
					{
						auto * c = static_cast< MD::Code* > ( it->data() );

						auto * font = createFont( renderOpts.m_codeFont, false, false,
							renderOpts.m_codeFontSize, pdfData.doc, scale, pdfData );

						const auto words = c->text().split( QLatin1Char( ' ' ),
							Qt::SkipEmptyParts );

						for( const auto & w : words )
						{
							CellItem item;
							item.word = w;
							item.font = font;
							item.background = renderOpts.m_codeBackground;

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Link :
					{
						auto * l = static_cast< MD::Link* > ( it->data() );

						auto * font = createFont( renderOpts.m_textFont,
							l->textOptions() & MD::TextOption::BoldText,
							l->textOptions() & MD::TextOption::ItalicText,
							renderOpts.m_textFontSize, pdfData.doc,
							scale, pdfData );

						if( l->textOptions() & MD::TextOption::StrikethroughText )
							font->SetStrikeOut( true );
						else
							font->SetStrikeOut( false );

						QString url = l->url();

						if( doc->labeledLinks().contains( url ) )
							url = doc->labeledLinks()[ url ]->url();

						if( !l->img()->isEmpty() )
						{
							CellItem item;
							item.image = loadImage( l->img().data() );
							item.url = url;

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
								item.font = font;
								item.url = url;
								item.color = renderOpts.m_linkColor;

								data.items.append( item );
							}
						}
						else
						{
							CellItem item;
							item.font = font;
							item.url = url;
							item.color = renderOpts.m_linkColor;

							data.items.append( item );
						}
					}
						break;

					case MD::ItemType::Image :
					{
						auto * i = static_cast< MD::Image* > ( it->data() );

						CellItem item;

						emit status( tr( "Loading image." ) );

						item.image = loadImage( i );

						data.items.append( item );
					}
						break;

					case MD::ItemType::FootnoteRef :
					{
						if( !inFootnote )
						{
							auto * ref = static_cast< MD::FootnoteRef* > ( it->data() );

							if( doc->footnotesMap().contains( ref->id() ) )
							{
								auto * font = createFont( renderOpts.m_textFont,
									false, false,
									renderOpts.m_textFontSize, pdfData.doc,
									scale, pdfData );

								CellItem item;
								item.font = font;
								item.footnote = QString::number( m_footnoteNum++ );
								item.footnoteObj = doc->footnotesMap()[ ref->id() ];

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
			cit->heightToWidth( lineHeight, spaceWidth, scale, pdfData );
}

QVector< WhereDrawn >
PdfRenderer::drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Table * item, QSharedPointer< MD::Document > doc, double offset, CalcHeightOpt heightCalcOpt,
	float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
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

			return ret;
		}

		case CalcHeightOpt::Full :
		{
			ret.append( { -1, 0.0, r0h + r1h + ( c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) +
				lineHeight - ( font->GetFontMetrics()->GetDescent() * ( justHeader ? 1.0 : 2.0 ) ) } );

			for( int i = 2; i < auxTable.at( 0 ).size(); ++i )
				ret.append( { -1, 0.0, rowHeight( auxTable, i ) + c_tableMargin * 2.0 -
					font->GetFontMetrics()->GetDescent() } );

			return ret;
		}

		default :
			break;
	}

	if( pdfData.coords.y - ( r0h + r1h + ( c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) -
			( font->GetFontMetrics()->GetDescent() * ( justHeader ? 1.0 : 2.0 ) ) ) <
				pdfData.currentPageAllowedY() )
	{
		createPage( pdfData );

		pdfData.freeSpaceOn( pdfData.currentPageIndex() );
	}

	moveToNewLine( pdfData, offset, lineHeight, 1.0 );

	QVector< QSharedPointer< MD::Footnote > > footnotes;

	for( int row = 0; row < auxTable[ 0 ].size(); ++row )
		ret.append( drawTableRow( auxTable, row, pdfData, offset, lineHeight, renderOpts,
			doc, footnotes, scale, inFootnote ) );

	for( const auto & f : qAsConst( footnotes ) )
		addFootnote( f, pdfData, renderOpts, doc );

	return ret;
}

void
PdfRenderer::addFootnote( QSharedPointer< MD::Footnote > f, PdfAuxData & pdfData,
	const RenderOpts & renderOpts, QSharedPointer< MD::Document > doc )
{
	PdfAuxData tmpData = pdfData;
	tmpData.coords = { { pdfData.coords.margins.left, pdfData.coords.margins.right,
			pdfData.coords.margins.top, pdfData.coords.margins.bottom },
		pdfData.page->GetPageSize().GetWidth(),
		pdfData.page->GetPageSize().GetHeight(),
		pdfData.coords.margins.left, pdfData.page->GetPageSize().GetHeight() -
			pdfData.coords.margins.top };

	auto h = footnoteHeight( tmpData, renderOpts,
		doc, f.data() );

	reserveSpaceForFootnote( pdfData, renderOpts, h, pdfData.coords.y,
		pdfData.currentPageIdx );

	m_footnotes.append( f );
}

QVector< WhereDrawn >
PdfRenderer::drawTableRow( QVector< QVector< CellData > > & table, int row, PdfAuxData & pdfData,
	double offset, double lineHeight, const RenderOpts & renderOpts,
	QSharedPointer< MD::Document > doc, QVector< QSharedPointer< MD::Footnote > > & footnotes,
	float scale, bool inFootnote )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	emit status( tr( "Drawing table row." ) );

	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, scale, pdfData );

	const auto startPage = pdfData.currentPageIndex();
	const auto startY = pdfData.coords.y;
	auto endPage = startPage;
	auto endY = startY;
	int currentPage = startPage;

	TextToDraw text;
	QMap< QString, QVector< QPair< QRectF, int > > > links;

	int column = 0;

	// Draw cells.
	for( auto it = table.cbegin(), last = table.cend(); it != last; ++it )
	{
		{
			QMutexLocker lock( &m_mutex );

			if( m_terminate )
				return ret;
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

		if( y < pdfData.currentPageAllowedY() )
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
				drawTextLineInTable( x, y, text, lineHeight, pdfData, links, font, currentPage,
					endPage, endY, footnotes, inFootnote );

			if( !c->image.isNull() )
			{
				if( textBefore )
					y -= lineHeight;

				PdfImage img( pdfData.doc );
				img.LoadFromData( reinterpret_cast< const unsigned char * >( c->image.data() ),
					c->image.size() );

				auto ratio = it->at( 0 ).width / img.GetWidth() * scale;

				auto h = img.GetHeight() * ratio;

				if(  y - h < pdfData.currentPageAllowedY() )
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
					links[ c->url ].append( qMakePair( QRectF( x, y, c->width( pdfData ),
							img.GetHeight() * ratio ),
						currentPage ) );

				textBefore = false;
			}
			else
			{
				auto w = c->font->GetFontMetrics()->StringWidth(
					createPdfString( c->word.isEmpty() ? c->url : c->word ) );
				double s = 0.0;

				if( !text.text.isEmpty() )
				{
					if( text.text.last().font == c->font )
						s = c->font->GetFontMetrics()->StringWidth( PdfString( " " ) );
					else
						s = font->GetFontMetrics()->StringWidth( PdfString( " " ) );
				}

				double fw = 0.0;

				if( c + 1 != clast && !( c + 1 )->footnote.isEmpty() )
				{
					const auto old = ( c + 1 )->font->GetFontSize();
					( c + 1 )->font->SetFontSize( old * scale );
					fw = ( c + 1 )->font->GetFontMetrics()->StringWidth( createPdfString(
						( c + 1 )->footnote ) );
					w += fw;
					( c + 1 )->font->SetFontSize( old );
				}

				if( text.width + s + w <= it->at( 0 ).width )
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
							font, currentPage, endPage, endY, footnotes, inFootnote );
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
							font, currentPage, endPage, endY, footnotes, inFootnote );

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
			drawTextLineInTable( x, y, text, lineHeight, pdfData, links, font, currentPage,
				endPage, endY, footnotes, inFootnote );

		y -= c_tableMargin - font->GetFontMetrics()->GetDescent();

		if( y < endY  && currentPage == pdfData.currentPageIndex() )
			endY = y;

		++column;
	}

	drawRowBorder( pdfData, startPage, ret, renderOpts, offset, table, startY, endY );

	pdfData.coords.y = endY;
	pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIndex() ) );

	processLinksInTable( pdfData, links, doc );

	return ret;
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
	QVector< QSharedPointer< MD::Footnote > > & footnotes, bool inFootnote )
{
	y -= lineHeight;

	if( y < pdfData.allowedY( currentPage ) )
	{
		newPageInTable( pdfData, currentPage, endPage, endY );

		y = pdfData.topY( currentPage ) - lineHeight;

		if( pdfData.drawFootnotes )
			y -= pdfData.extraInFootnote;
	}

	if( text.width <= text.availableWidth )
	{
		switch( text.alignment )
		{
			case MD::Table::AlignRight :
				x = x + text.availableWidth - text.width;
				break;

			case MD::Table::AlignCenter :
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

		auto * fm = text.text.first().font->GetFontMetrics();

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
		if( it->background.isValid() )
		{
			pdfData.setColor( it->background );

			pdfData.drawRectangle( x, y + it->font->GetFontMetrics()->GetDescent(),
				it->width( pdfData ), it->font->GetFontMetrics()->GetLineSpacing() );

			pdfData.painter->Fill();

			pdfData.restoreColor();
		}

		if( it->color.isValid() )
			pdfData.setColor( it->color );

		pdfData.painter->SetFont( it->font );
		pdfData.drawText( x, y, createPdfString( it->word.isEmpty() ?
			it->url : it->word ) );

		pdfData.restoreColor();

		if( !it->url.isEmpty() )
			links[ it->url ].append( qMakePair( QRectF( x, y, it->width( pdfData ), lineHeight ),
				currentPage ) );

		x += it->width( pdfData );

		if( !inFootnote )
		{
			if( it + 1 != last && !( it + 1 )->footnote.isEmpty() )
			{
				++it;

				const auto str = createPdfString( it->footnote );

				const auto old = it->font->GetFontSize();
				it->font->SetFontSize( old * c_footnoteScale );

				const auto w = it->font->GetFontMetrics()->StringWidth( str );

				pdfData.drawText( x, y + lineHeight -
					it->font->GetFontMetrics()->GetLineSpacing(), str );
				it->font->SetFontSize( old );

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

				const auto sw = it->font->GetFontMetrics()->StringWidth( PdfString( " " ) );

				pdfData.drawRectangle( x, y + it->font->GetFontMetrics()->GetDescent(),
					sw, it->font->GetFontMetrics()->GetLineSpacing() );

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
	QSharedPointer< MD::Document > doc )
{
	for( auto it = links.cbegin(), last = links.cend(); it != last; ++it )
	{
		QString url = it.key();

		if( doc->labeledLinks().contains( url ) )
			url = doc->labeledLinks()[ url ]->url();

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
	QSharedPointer< MD::Item > item, QSharedPointer< MD::Document > doc,
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
			ret = drawParagraph( tmp, renderOpts, static_cast< MD::Paragraph* > ( item.data() ),
				doc, offset, true, CalcHeightOpt::Minimum, scale, inFootnote );
		}
			break;

		case MD::ItemType::Code :
		{
			ret = drawCode( tmp, renderOpts, static_cast< MD::Code* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, scale );
		}
			break;

		case MD::ItemType::Blockquote :
		{
			ret = drawBlockquote( tmp, renderOpts,
				static_cast< MD::Blockquote* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, scale, inFootnote );
		}
			break;

		case MD::ItemType::List :
		{
			auto * list = static_cast< MD::List* > ( item.data() );
			const auto bulletWidth = maxListNumberWidth( list );

			auto * font = createFont( m_opts.m_textFont, false, false,
				m_opts.m_textFontSize, pdfData.doc, scale, pdfData );
			tmp.coords.y -= font->GetFontMetrics()->GetLineSpacing();

			ret = drawList( tmp, m_opts, list, m_doc, bulletWidth, offset,
				CalcHeightOpt::Minimum, scale, inFootnote );
		}
			break;

		case MD::ItemType::Table :
		{
			ret = drawTable( tmp, renderOpts,
				static_cast< MD::Table* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, scale, inFootnote );
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
