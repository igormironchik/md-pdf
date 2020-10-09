
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

// Qt include.
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QThread>
#include <QBuffer>


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
// PdfRenderer
//

PdfRenderer::PdfRenderer()
	:	m_terminate( false )
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
}

void
PdfRenderer::renderImpl()
{
	{
		const int itemsCount = m_doc->items().size();

		emit progress( 0 );

		PdfMemDocument document;

		PdfPainter painter;

		try {
			int itemIdx = 0;

			PdfAuxData pdfData;

			pdfData.doc = &document;
			pdfData.painter = &painter;
			pdfData.coords.margins.left = m_opts.m_left;
			pdfData.coords.margins.right = m_opts.m_right;
			pdfData.coords.margins.top = m_opts.m_top;
			pdfData.coords.margins.bottom = m_opts.m_bottom;

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
								minNecessaryHeight( pdfData, m_opts, *( it + 1 ), m_doc, 0.0 ) :
								0.0 ) );
						break;

					case MD::ItemType::Paragraph :
						drawParagraph( pdfData, m_opts, static_cast< MD::Paragraph* > ( it->data() ),
							m_doc );
						break;

					case MD::ItemType::Code :
						drawCode( pdfData, m_opts, static_cast< MD::Code* > ( it->data() ),
							m_doc );
						break;

					case MD::ItemType::Blockquote :
						drawBlockquote( pdfData, m_opts,
							static_cast< MD::Blockquote* > ( it->data() ),
							m_doc );
						break;

					case MD::ItemType::List :
					{
						auto * list = static_cast< MD::List* > ( it->data() );
						const auto bulletWidth = maxListNumberWidth( list );

						auto * font = createFont( m_opts.m_textFont, false, false,
							m_opts.m_textFontSize, pdfData.doc, 1.0 );
						pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

						drawList( pdfData, m_opts, list, m_doc, bulletWidth );
					}
						break;

					case MD::ItemType::Table :
						drawTable( pdfData, m_opts,
							static_cast< MD::Table* > ( it->data() ),
							m_doc );
						break;

					case MD::ItemType::PageBreak :
					{
						if( itemIdx < itemsCount )
						{
							painter.FinishPage();

							createPage( pdfData );
						}
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

			painter.FinishPage();

			resolveLinks( pdfData );

			document.Write( m_fileName.toLocal8Bit().data() );

			emit done( m_terminate );
		}
		catch( const PdfError & e )
		{
			try {
				painter.FinishPage();
				document.Write( m_fileName.toLocal8Bit().data() );
			}
			catch( ... )
			{
			}

			emit error( QString::fromLatin1( PdfError::ErrorMessage( e.GetError() ) ) );
		}
		catch( const PdfRendererError & e )
		{
			try {
				painter.FinishPage();
				document.Write( m_fileName.toLocal8Bit().data() );
			}
			catch( ... )
			{
			}

			emit error( e.what() );
		}
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
PdfRenderer::clean()
{
	m_dests.clear();
	m_unresolvedLinks.clear();
	PdfEncodingFactory::FreeGlobalEncodingInstances();
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
	}
}

PdfFont *
PdfRenderer::createFont( const QString & name, bool bold, bool italic, float size,
	PdfMemDocument * doc, float scale )
{
	auto * font = doc->CreateFont( name.toLocal8Bit().data(), bold, italic , false,
		PdfEncodingFactory::GlobalIdentityEncodingInstance(),
		PdfFontCache::eFontCreationFlags_None );

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
	pdfData.page = pdfData.doc->CreatePage(
		PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );

	if( !pdfData.page )
		throw PdfRendererError( QLatin1String( "Oops, can't create empty page in PDF.\n\n"
			"This is very strange, it should not appear ever, but it is. "
			"I'm sorry for the inconvenience." ) );

	pdfData.painter->SetPage( pdfData.page );

	pdfData.coords = { { pdfData.coords.margins.left, pdfData.coords.margins.right,
			pdfData.coords.margins.top, pdfData.coords.margins.bottom },
		pdfData.page->GetPageSize().GetWidth(),
		pdfData.page->GetPageSize().GetHeight(),
		pdfData.coords.margins.left, pdfData.page->GetPageSize().GetHeight() -
			pdfData.coords.margins.top };

	++pdfData.currentPageIdx;
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
	double nextItemMinHeight, float fontScale )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	emit status( tr( "Drawing heading." ) );

	PdfFont * font = createFont( renderOpts.m_textFont.toLocal8Bit().data(),
		true, false, renderOpts.m_textFontSize + 16 - ( item->level() < 7 ? item->level() * 2 : 12 ),
		pdfData.doc, fontScale );

	pdfData.painter->SetFont( font );
	pdfData.painter->SetColor( 0.0, 0.0, 0.0 );

	const double width = pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset;

	const auto lines = pdfData.painter->GetMultiLineTextAsLines(
		width, createPdfString( item->text() ) );

	const double height = lines.size() * font->GetFontMetrics()->GetLineSpacing();
	const double availableHeight = pdfData.coords.pageHeight - pdfData.coords.margins.top -
		pdfData.coords.margins.bottom;

	pdfData.coords.y -= c_beforeHeading;

	// If heading can be placed with next item on current page.
	if( pdfData.coords.y - height - nextItemMinHeight > pdfData.coords.margins.bottom )
	{
		pdfData.painter->DrawMultiLineText( pdfData.coords.margins.left + offset,
			pdfData.coords.y - height,
			width, height, createPdfString( item->text() ) );

		if( !item->label().isEmpty() )
			m_dests.insert( item->label(), PdfDestination( pdfData.page,
				PdfRect( pdfData.coords.margins.left + offset,
					pdfData.coords.y - font->GetFontMetrics()->GetLineSpacing(),
					width, font->GetFontMetrics()->GetLineSpacing() ) ) );

		pdfData.coords.y -= height;

		ret.append( { pdfData.currentPageIdx, pdfData.coords.y, height } );

		return ret;
	}
	// If heading can be placed with next item on empty page.
	else if( height + nextItemMinHeight <= availableHeight )
	{
		pdfData.painter->FinishPage();
		createPage( pdfData );
		return drawHeading( pdfData, renderOpts, item, doc, offset, nextItemMinHeight );
	}
	// Otherwise we need to split heading to place it on different pages.
	else
	{
		std::vector< PdfString > tmp;
		double h = 0.0;
		std::size_t i = 0;
		double available = pdfData.coords.pageHeight - pdfData.coords.margins.top -
			pdfData.coords.margins.bottom;
		const double spacing = font->GetFontMetrics()->GetLineSpacing();

		while( available >= spacing )
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

		pdfData.painter->DrawMultiLineText( pdfData.coords.margins.left + offset,
			pdfData.coords.y - h,
			width, h, createPdfString( text ) );

		if( !item->label().isEmpty() )
			m_dests.insert( item->label(), PdfDestination( pdfData.page,
				PdfRect( pdfData.coords.margins.left + offset,
					pdfData.coords.y - font->GetFontMetrics()->GetLineSpacing(),
					width, font->GetFontMetrics()->GetLineSpacing() ) ) );

		pdfData.coords.y -= height;

		ret.append( { pdfData.currentPageIdx, pdfData.coords.y, height } );

		pdfData.painter->FinishPage();

		createPage( pdfData );

		ret.append( drawHeading( pdfData, renderOpts, item, doc, offset, nextItemMinHeight ) );

		return ret;
	}
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Text * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float fontScale )
{
	auto * spaceFont = createFont( renderOpts.m_textFont, false, false,
		renderOpts.m_textFontSize, pdfData.doc, fontScale );

	auto * font = createFont( renderOpts.m_textFont, item->opts() & MD::TextOption::BoldText,
		item->opts() & MD::TextOption::ItalicText,
		renderOpts.m_textFontSize, pdfData.doc, fontScale );

	if( item->opts() & MD::TextOption::StrikethroughText )
		font->SetStrikeOut( true );
	else
		font->SetStrikeOut( false );

	return drawString( pdfData, renderOpts, item->text(),
		spaceFont, font, font->GetFontMetrics()->GetLineSpacing(),
		doc, newLine, offset, firstInParagraph, cw );
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
	MD::Link * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float fontScale )
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
		pdfData.painter->Save();
		pdfData.painter->SetColor( renderOpts.m_linkColor.redF(),
			renderOpts.m_linkColor.greenF(),
			renderOpts.m_linkColor.blueF() );

		auto * font = createFont( renderOpts.m_textFont, item->textOptions() & MD::TextOption::BoldText,
			item->textOptions() & MD::TextOption::ItalicText, renderOpts.m_textFontSize,
			pdfData.doc, fontScale );

		if( item->textOptions() & MD::TextOption::StrikethroughText )
			font->SetStrikeOut( true );
		else
			font->SetStrikeOut( false );

		rects = normalizeRects( drawString( pdfData, renderOpts,
			( !item->text().isEmpty() ? item->text() : url ),
			createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
				pdfData.doc, fontScale ),
			font, font->GetFontMetrics()->GetLineSpacing(),
			doc, newLine, offset, firstInParagraph, cw ) );

		pdfData.painter->Restore();
	}
	// Otherwise image link.
	else
		rects.append( drawImage( pdfData, renderOpts, item->img().data(), doc, newLine, offset,
			firstInParagraph, cw ) );

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
	QSharedPointer< MD::Document > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, const QColor & background )
{
	Q_UNUSED( doc )
	Q_UNUSED( renderOpts )

	bool draw = true;

	if( cw && !cw->isDrawing() )
		draw = false;

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

		auto scale = 1.0;

		if( draw && cw )
			scale = cw->scale();

		const auto xv = pdfData.coords.x + w * scale / 100.0 + font->GetFontMetrics()->StringWidth(
			createPdfString( words.first() ) );

		if( xv < wv || qAbs( xv - wv ) < 0.01 )
		{
			if( draw )
			{
				spaceFont->SetFontScale( scale );

				pdfData.painter->DrawText( pdfData.coords.x, pdfData.coords.y, " " );

				spaceFont->SetFontScale( 100.0 );
			}
			else if( cw )
				cw->append( { w, lineHeight, true, false, true, " " } );

			ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
				w * scale / 100.0, lineHeight ), pdfData.currentPageIdx ) );

			pdfData.coords.x += w * scale / 100.0;
		}
		else
			newLineFn();
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

		const auto xv = pdfData.coords.x + length;

		if( xv < wv || qAbs( xv - wv ) < 0.01 )
		{
			newLine = false;

			if( draw )
			{
				if( background.isValid() )
				{
					pdfData.painter->Save();
					pdfData.painter->SetColor( background.redF(),
						background.greenF(), background.blueF() );
					pdfData.painter->Rectangle( pdfData.coords.x, pdfData.coords.y +
						font->GetFontMetrics()->GetDescent(), length,
						font->GetFontMetrics()->GetLineSpacing() );
					pdfData.painter->Fill();
					pdfData.painter->Restore();
				}

				pdfData.painter->DrawText( pdfData.coords.x, pdfData.coords.y, str );
				ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
					length, lineHeight ), pdfData.currentPageIdx ) );
			}
			else if( cw )
				cw->append( { length, lineHeight, false, false, true, *it } );

			pdfData.coords.x += length;

			// Draw space if needed.
			if( it + 1 != last )
			{
				const auto spaceWidth = font->GetFontMetrics()->StringWidth( " " );
				const auto nextLength = font->GetFontMetrics()->StringWidth( createPdfString(
					*( it + 1 ) ) );

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
							spaceWidth * scale / 100.0, lineHeight ), pdfData.currentPageIdx ) );

						if( background.isValid() )
						{
							pdfData.painter->Save();
							pdfData.painter->SetColor( background.redF(),
								background.greenF(), background.blueF() );
							pdfData.painter->Rectangle( pdfData.coords.x, pdfData.coords.y +
								font->GetFontMetrics()->GetDescent(), spaceWidth * scale / 100.0,
								font->GetFontMetrics()->GetLineSpacing() );
							pdfData.painter->Fill();
							pdfData.painter->Restore();
						}

						pdfData.painter->DrawText( pdfData.coords.x, pdfData.coords.y, " " );

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
			const auto xv = pdfData.coords.margins.left + offset + length;

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
					pdfData.painter->DrawText( pdfData.coords.x, pdfData.coords.y, str );
					ret.append( qMakePair( QRectF( pdfData.coords.x, pdfData.coords.y,
							font->GetFontMetrics()->StringWidth( str ), lineHeight ),
						pdfData.currentPageIdx ) );
				}
				else if( cw )
					cw->append( { font->GetFontMetrics()->StringWidth( str ), lineHeight, false, false, true, *it } );

				newLineFn();
			}
		}
	}

	return ret;
}

QVector< QPair< QRectF, int > >
PdfRenderer::drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float fontScale )
{
	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, fontScale );

	auto * font = createFont( renderOpts.m_codeFont, false, false, renderOpts.m_codeFontSize,
		pdfData.doc, fontScale );

	return drawString( pdfData, renderOpts, item->text(), font, font,
		textFont->GetFontMetrics()->GetLineSpacing(),
		doc, newLine, offset, firstInParagraph, cw, renderOpts.m_codeBackground );
}

void
PdfRenderer::moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
	double yOffsetMultiplier )
{
	pdfData.coords.x = pdfData.coords.margins.left + xOffset;
	pdfData.coords.y -= yOffset * yOffsetMultiplier;

	if( pdfData.coords.y - yOffset < pdfData.coords.margins.bottom )
	{
		pdfData.painter->FinishPage();
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

} /* namespace anonymous */

QVector< WhereDrawn >
PdfRenderer::drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Paragraph * item, QSharedPointer< MD::Document > doc, double offset, bool withNewLine,
	CalcHeightOpt heightCalcOpt, float fontScale )
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
		renderOpts.m_textFontSize, pdfData.doc, fontScale );

	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	if( withNewLine && heightCalcOpt == CalcHeightOpt::Unknown )
		moveToNewLine( pdfData, 0.0, lineHeight, 2.0 );

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		pdfData.coords.x = pdfData.coords.margins.left + offset;

		if( pdfData.coords.y < pdfData.coords.margins.bottom )
		{
			pdfData.painter->FinishPage();
			createPage( pdfData );
			pdfData.coords.x = pdfData.coords.margins.left + offset;
		}
	}

	bool newLine = false;
	CustomWidth cw;
	auto y = pdfData.coords.y;

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
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale );
				break;

			case MD::ItemType::Code :
				drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale );
				break;

			case MD::ItemType::Link :
				drawLink( pdfData, renderOpts, static_cast< MD::Link* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale );
				break;

			case MD::ItemType::Image :
				drawImage( pdfData, renderOpts, static_cast< MD::Image* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale );
				break;

			case MD::ItemType::LineBreak :
			{
				cw.append( { 0.0, lineHeight, false, true, false, "" } );
				pdfData.coords.x = pdfData.coords.margins.left + offset;
			}
				break;

			default :
				break;
		}
	}

	if( heightCalcOpt == CalcHeightOpt::Minimum )
	{
		QVector< WhereDrawn > r;
		r.append( { 0, 0.0, cw.firstItemHeight() } );

		return r;
	}

	cw.append( { 0.0, lineHeight, false, true, false, "" } );

	cw.calcScale( pdfData.coords.pageWidth - pdfData.coords.margins.left -
		pdfData.coords.margins.right - offset );

	cw.setDrawing();

	newLine = false;
	pdfData.coords.y = y;
	pdfData.coords.x = pdfData.coords.margins.left + offset;

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
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale ) );
				break;

			case MD::ItemType::Code :
				rects.append( drawInlinedCode( pdfData, renderOpts, static_cast< MD::Code* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale ) );
				break;

			case MD::ItemType::Link :
				rects.append(drawLink( pdfData, renderOpts, static_cast< MD::Link* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale ) );
				break;

			case MD::ItemType::Image :
				rects.append( drawImage( pdfData, renderOpts, static_cast< MD::Image* > ( it->data() ),
					doc, newLine, offset, it == item->items().begin(), &cw, fontScale ) );
				break;

			case MD::ItemType::LineBreak :
				moveToNewLine( pdfData, offset, lineHeight );
				break;

			default :
				break;
		}
	}

	return toWhereDrawn( normalizeRects( rects ), pdfData.coords.pageHeight );
}

QPair< QRectF, int >
PdfRenderer::drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Image * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
	bool firstInParagraph, CustomWidth * cw, float fontScale )
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
			QByteArray data;
			QBuffer buf( &data );

			img.save( &buf, "png" );

			PdfImage pdfImg( pdfData.doc );
			pdfImg.LoadFromData( reinterpret_cast< unsigned char * >( data.data() ), data.size() );

			newLine = true;

			auto * font = createFont( renderOpts.m_textFont, false, false,
				renderOpts.m_textFontSize, pdfData.doc, fontScale );

			const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

			if( !firstInParagraph )
				moveToNewLine( pdfData, offset, lineHeight, 2.0 );
			else
				pdfData.coords.x += offset;

			double x = 0.0;
			double scale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;
			double availableHeight = pdfData.coords.y - pdfData.coords.margins.bottom;

			if( pdfImg.GetWidth() > availableWidth )
				scale = availableWidth / pdfImg.GetWidth();

			const double pageHeight = pdfData.coords.pageHeight - pdfData.coords.margins.top -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * scale > pageHeight )
			{
				scale = pageHeight / ( pdfImg.GetHeight() * scale );

				pdfData.painter->FinishPage();

				createPage( pdfData );

				pdfData.coords.x += offset;
			}
			else if( pdfImg.GetHeight() * scale > availableHeight )
			{
				pdfData.painter->FinishPage();

				createPage( pdfData );

				pdfData.coords.x += offset;
			}

			if( pdfImg.GetWidth() * scale < availableWidth )
				x = ( availableWidth - pdfImg.GetWidth() * scale ) / 2.0;

			pdfData.painter->DrawImage( pdfData.coords.x + x,
				pdfData.coords.y - pdfImg.GetHeight() * scale,
				&pdfImg, scale, scale );

			pdfData.coords.y -= pdfImg.GetHeight() * scale;

			QRectF r( pdfData.coords.x + x, pdfData.coords.y,
				pdfImg.GetWidth() * scale, pdfImg.GetHeight() * scale );

			moveToNewLine( pdfData, offset, lineHeight, 1.0 );

			return qMakePair( r, pdfData.currentPageIdx );
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
			QByteArray data;
			QBuffer buf( &data );

			img.save( &buf, "jpg" );

			PdfImage pdfImg( pdfData.doc );
			pdfImg.LoadFromData( reinterpret_cast< unsigned char * >( data.data() ), data.size() );

			newLine = true;

			auto * font = createFont( renderOpts.m_textFont, false, false,
				renderOpts.m_textFontSize, pdfData.doc, fontScale );

			const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

			height += lineHeight * 2.0;

			double scale = 1.0;
			const double availableWidth = pdfData.coords.pageWidth - pdfData.coords.margins.left -
				pdfData.coords.margins.right - offset;

			if( pdfImg.GetWidth() > availableWidth )
				scale = availableWidth / pdfImg.GetWidth();

			const double pageHeight = pdfData.coords.pageHeight - pdfData.coords.margins.top -
				pdfData.coords.margins.bottom;

			if( pdfImg.GetHeight() * scale > pageHeight )
				scale = pageHeight / ( pdfImg.GetHeight() * scale );

			height += pdfImg.GetHeight() * scale;
		}

		pdfData.coords.x = pdfData.coords.margins.left + offset;
		cw->append( { 0.0, height, false, true, false, "" } );

		return qMakePair( QRectF(), pdfData.currentPageIdx );
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

QImage
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
	}
	else
		throw PdfRendererError(
			tr( "Hmm, I don't know how to load this image: %1.\n\n"
				"This image is not a local existing file, and not in the Web. Check your Markdown." )
					.arg( item->url() ) );

	m_imageCache.insert( item->url(), img );

	return img;
}

QVector< WhereDrawn >
PdfRenderer::drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Code * item, QSharedPointer< MD::Document > doc, double offset, CalcHeightOpt heightCalcOpt,
	float fontScale )
{
	Q_UNUSED( doc )

	if( heightCalcOpt == CalcHeightOpt::Unknown )
		emit status( tr( "Drawing code." ) );

	auto * textFont = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, fontScale );
	const auto textLHeight = textFont->GetFontMetrics()->GetLineSpacing();

	QStringList lines;

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		if( pdfData.coords.y - ( textLHeight * 2.0 ) < pdfData.coords.margins.bottom )
			createPage( pdfData );
		else
			pdfData.coords.y -= textLHeight * 2.0;

		pdfData.coords.x = pdfData.coords.margins.left + offset;

		lines = item->text().split( QLatin1Char( '\n' ), Qt::KeepEmptyParts );

		for( auto it = lines.begin(), last = lines.end(); it != last; ++it )
			it->replace( QStringLiteral( "\t" ), QStringLiteral( "    " ) );
	}

	auto * font = createFont( renderOpts.m_codeFont, false, false, renderOpts.m_codeFontSize,
		pdfData.doc, fontScale );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	if( heightCalcOpt == CalcHeightOpt::Minimum )
	{
		QVector< WhereDrawn > r;
		r.append( { 0, 0.0, textLHeight * 2.0 + lineHeight } );

		return r;
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

		while( y - lineHeight > pdfData.coords.margins.bottom && j < lines.size() )
		{
			h += lineHeight;
			y -= lineHeight;
			++j;
		}

		if( i < j )
		{
			pdfData.painter->Save();
			pdfData.painter->SetColor( renderOpts.m_codeBackground.redF(),
				renderOpts.m_codeBackground.greenF(),
				renderOpts.m_codeBackground.blueF() );
			pdfData.painter->Rectangle( pdfData.coords.x, y,
				pdfData.coords.pageWidth - pdfData.coords.x - pdfData.coords.margins.right,
				 h + lineHeight );
			pdfData.painter->Fill();
			pdfData.painter->Restore();

			ret.append( { pdfData.currentPageIdx, y, h + lineHeight } );
		}

		for( ; i < j; ++i )
		{
			pdfData.coords.x = pdfData.coords.margins.left + offset;

			while( true )
			{
				if( currentWord == colored.size() || colored[ currentWord ].line != i )
					break;

				pdfData.painter->Save();

				switch( colored[ currentWord ].color )
				{
					case Syntax::ColorRole::Keyword :
					{
						pdfData.painter->SetColor( renderOpts.m_keywordColor.redF(),
							renderOpts.m_keywordColor.greenF(),
							renderOpts.m_keywordColor.blueF() );
					}
						break;

					case Syntax::ColorRole::Regular :
					{
						pdfData.painter->SetColor( renderOpts.m_codeColor.redF(),
							renderOpts.m_codeColor.greenF(),
							renderOpts.m_codeColor.blueF() );
					}
						break;

					case Syntax::ColorRole::Comment :
					{
						pdfData.painter->SetColor( renderOpts.m_commentColor.redF(),
							renderOpts.m_commentColor.greenF(),
							renderOpts.m_commentColor.blueF() );
					}
						break;
				}

				const auto length = colored[ currentWord ].endPos -
					colored[ currentWord ].startPos + 1;

				pdfData.painter->DrawText( pdfData.coords.x, pdfData.coords.y,
					createPdfString( lines.at( i ).mid( colored[ currentWord ].startPos, length ) ) );

				pdfData.coords.x += spaceWidth * length;

				pdfData.painter->Restore();

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
	CalcHeightOpt heightCalcOpt, float fontScale )
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
				if( heightCalcOpt == CalcHeightOpt::Unknown )
					ret.append( drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading* > ( it->data() ),
						doc, offset + c_blockquoteBaseOffset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ), doc,
								offset + c_blockquoteBaseOffset  ) : 0.0 ),
						fontScale ) );
				else
				{
					ret.append( { 0, 0.0, 0.0 } );

					return ret;
				}
			}
				break;

			case MD::ItemType::Paragraph :
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, true, heightCalcOpt,
					fontScale ) );
				break;

			case MD::ItemType::Code :
				ret.append( drawCode( pdfData, renderOpts,
					static_cast< MD::Code* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					fontScale ) );
				break;

			case MD::ItemType::Blockquote :
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					fontScale ) );
				break;

			case MD::ItemType::List :
			{
				auto * list = static_cast< MD::List* > ( it->data() );
				const auto bulletWidth = maxListNumberWidth( list );

				auto * font = createFont( m_opts.m_textFont, false, false,
					m_opts.m_textFontSize, pdfData.doc, fontScale );
				pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

				ret.append( drawList( pdfData, renderOpts,
					list,
					doc, bulletWidth, offset + c_blockquoteBaseOffset, heightCalcOpt,
					fontScale ) );
			}
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table* > ( it->data() ),
					doc, offset + c_blockquoteBaseOffset, heightCalcOpt,
					fontScale ) );
				break;

			default :
				break;
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			return ret;
	}

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

	pdfData.painter->FinishPage();

	// Draw blockquote left vertival bar.
	for( auto it = map.cbegin(), last = map.cend(); it != last; ++it )
	{
		pdfData.painter->SetPage( pdfData.doc->GetPage( it.key() ) );
		pdfData.painter->Save();
		pdfData.painter->SetColor( renderOpts.m_borderColor.redF(),
			renderOpts.m_borderColor.greenF(),
			renderOpts.m_borderColor.blueF() );
		pdfData.painter->Rectangle( pdfData.coords.margins.left + offset, it.value().y,
			c_blockquoteMarkWidth, it.value().height );
		pdfData.painter->Fill();
		pdfData.painter->Restore();
	}

	pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIdx ) );

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::List * item, QSharedPointer< MD::Document > doc, int bulletWidth, double offset,
	CalcHeightOpt heightCalcOpt, float fontScale )
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
				fontScale ) );

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			break;
	}

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::ListItem * item, QSharedPointer< MD::Document > doc, int & idx,
	ListItemType & prevListItemType, int bulletWidth, double offset, CalcHeightOpt heightCalcOpt,
	float fontScale )
{
	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, fontScale );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();

	if( heightCalcOpt == CalcHeightOpt::Unknown )
	{
		pdfData.painter->SetFont( font );

		if( pdfData.coords.y - lineHeight < pdfData.coords.margins.bottom )
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

			pdfData.painter->DrawText( pdfData.coords.margins.left + offset,
				pdfData.coords.y, createPdfString( idxText ) );
		}
		else
		{
			prevListItemType = ListItemType::Unordered;

			pdfData.painter->Save();
			pdfData.painter->SetColor( 0.0, 0.0, 0.0 );
			const auto r = unorderedMarkWidth / 2.0;
			pdfData.painter->Circle( pdfData.coords.margins.left + offset + r,
				pdfData.coords.y + unorderedMarkWidth, r );
			pdfData.painter->Fill();
			pdfData.painter->Restore();
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
				if( heightCalcOpt == CalcHeightOpt::Unknown )
					ret.append( drawHeading( pdfData, renderOpts,
						static_cast< MD::Heading* > ( it->data() ),
						doc, offset,
						( it + 1 != last ?
							minNecessaryHeight( pdfData, renderOpts, *( it + 1 ),  doc, offset ) :
							0.0 ),
						fontScale ) );
				else
					ret.append( { 0, 0.0, 0.0 } );
			}
				break;

			case MD::ItemType::Paragraph :
			{
				ret.append( drawParagraph( pdfData, renderOpts,
					static_cast< MD::Paragraph* > ( it->data() ),
					doc, offset, ( it != item->items().cbegin() ), heightCalcOpt,
					fontScale ) );

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::Code :
			{
				ret.append( drawCode( pdfData, renderOpts,
					static_cast< MD::Code* > ( it->data() ),
					doc, offset, heightCalcOpt, fontScale ) );

				addExtraSpace = false;
			}
				break;

			case MD::ItemType::Blockquote :
			{
				ret.append( drawBlockquote( pdfData, renderOpts,
					static_cast< MD::Blockquote* > ( it->data() ),
					doc, offset, heightCalcOpt, fontScale ) );

				addExtraSpace = ( it != item->items().cbegin() );
			}
				break;

			case MD::ItemType::List :
				ret.append( drawList( pdfData, renderOpts,
					static_cast< MD::List* > ( it->data() ),
					doc, bulletWidth, offset, heightCalcOpt,
					fontScale ) );
				break;

			case MD::ItemType::Table :
				ret.append( drawTable( pdfData, renderOpts,
					static_cast< MD::Table* > ( it->data() ),
					doc, offset, heightCalcOpt, fontScale ) );
				break;

			default :
				break;
		}

		if( heightCalcOpt == CalcHeightOpt::Minimum )
			break;
	}

	if( addExtraSpace )
	{
		ret.append( { pdfData.currentPageIdx, pdfData.coords.y, lineHeight } );
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
	MD::Table * item, QSharedPointer< MD::Document > doc, float fontScale )
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
							renderOpts.m_textFontSize, pdfData.doc, fontScale );

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
							renderOpts.m_codeFontSize, pdfData.doc, fontScale );

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
							fontScale );

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
	double spaceWidth, double offset, double lineHeight )
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
			cit->heightToWidth( lineHeight, spaceWidth );
}

QVector< WhereDrawn >
PdfRenderer::drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
	MD::Table * item, QSharedPointer< MD::Document > doc, double offset, CalcHeightOpt heightCalcOpt,
	float fontScale )
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
		pdfData.doc, fontScale );
	const auto lineHeight = font->GetFontMetrics()->GetLineSpacing();
	const auto spaceWidth = font->GetFontMetrics()->StringWidth( PdfString( " " ) );

	auto auxTable = createAuxTable( pdfData, renderOpts, item, doc );

	calculateCellsSize( pdfData, auxTable, spaceWidth, offset, lineHeight );

	const auto r0h = rowHeight( auxTable, 0 );
	const bool justHeader = auxTable.at( 0 ).size() == 1;
	const auto r1h = ( !justHeader ? rowHeight( auxTable, 1 ) : 0 );

	if( heightCalcOpt == CalcHeightOpt::Minimum )
	{
		ret.append( { 0, 0.0, r0h + r1h + c_tableMargin * ( justHeader ? 2.0 : 4.0 ) } );

		return ret;
	}

	if( pdfData.coords.y - ( r0h + r1h + c_tableMargin * ( justHeader ? 2.0 : 4.0 ) ) <
		pdfData.coords.margins.bottom )
	{
		if( r0h + r1h + c_tableMargin * ( justHeader ? 2.0 : 4.0 ) <= pdfData.coords.pageHeight -
			pdfData.coords.margins.top - pdfData.coords.margins.bottom )
		{
			createPage( pdfData );
		}
	}

	moveToNewLine( pdfData, offset, lineHeight, 1.0 );

	for( int row = 0; row < auxTable[ 0 ].size(); ++row )
		ret.append( drawTableRow( auxTable, row, pdfData, offset, lineHeight, renderOpts,
			doc, fontScale ) );

	return ret;
}

QVector< WhereDrawn >
PdfRenderer::drawTableRow( QVector< QVector< CellData > > & table, int row, PdfAuxData & pdfData,
	double offset, double lineHeight, const RenderOpts & renderOpts,
	QSharedPointer< MD::Document > doc, float fontScale )
{
	QVector< WhereDrawn > ret;

	{
		QMutexLocker lock( &m_mutex );

		if( m_terminate )
			return ret;
	}

	emit status( tr( "Drawing table row." ) );

	auto * font = createFont( renderOpts.m_textFont, false, false, renderOpts.m_textFontSize,
		pdfData.doc, fontScale );

	const auto startPage = pdfData.currentPageIdx;
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

		if( y < pdfData.coords.margins.bottom )
		{
			newPageInTable( pdfData, currentPage, endPage, endY );

			y = pdfData.coords.pageHeight - pdfData.coords.margins.top;
		}

		bool textBefore = false;

		for( auto c = it->at( row ).items.cbegin(), clast = it->at( row ).items.cend(); c != clast; ++c )
		{
			if( !c->image.isNull() && !text.text.isEmpty() )
				drawTextLineInTable( x, y, text, lineHeight, pdfData, links, font, currentPage,
					endPage, endY );

			if( !c->image.isNull() )
			{
				if( textBefore )
					y -= lineHeight;

				auto ratio = it->at( 0 ).width /
					static_cast< double > ( c->image.width() );

				auto h = static_cast< double > ( c->image.height() ) * ratio;

				if(  y - h < pdfData.coords.margins.bottom )
				{
					newPageInTable( pdfData, currentPage, endPage, endY );

					y = pdfData.coords.pageHeight - pdfData.coords.margins.top;
				}

				const auto availableHeight = pdfData.coords.pageHeight - pdfData.coords.margins.top -
					pdfData.coords.margins.bottom;

				if( h > availableHeight )
					ratio = availableHeight / static_cast< double > ( c->image.height() );

				const auto w = static_cast< double > ( c->image.width() ) * ratio;
				auto o = 0.0;

				if( w < table[ column ][ 0 ].width )
					o = ( table[ column ][ 0 ].width - w ) / 2.0;

				QByteArray data;
				QBuffer buf( &data );

				c->image.save( &buf, "jpg" );

				PdfImage img( pdfData.doc );
				img.LoadFromData( reinterpret_cast< unsigned char * >( data.data() ), data.size() );

				y -= static_cast< double > ( c->image.height() ) * ratio;

				pdfData.painter->DrawImage( x + o, y, &img, ratio, ratio );

				textBefore = false;
			}
			else
			{
				const auto w = c->font->GetFontMetrics()->StringWidth(
					createPdfString( c->word.isEmpty() ? c->url : c->word ) );
				double s = 0.0;

				if( !text.text.isEmpty() )
				{
					if( text.text.last().font == c->font )
						s = c->font->GetFontMetrics()->StringWidth( PdfString( " " ) );
					else
						s = font->GetFontMetrics()->StringWidth( PdfString( " " ) );
				}

				if( text.width + s + w <= it->at( 0 ).width )
				{
					text.text.append( *c );
					text.width += s + w;
				}
				else
				{
					if( !text.text.isEmpty() )
					{
						drawTextLineInTable( x, y, text, lineHeight, pdfData, links,
							font, currentPage, endPage, endY );
						text.text.append( *c );
						text.width += w;
					}
					else
					{
						text.text.append( *c );
						text.width += w;
						drawTextLineInTable( x, y, text, lineHeight, pdfData, links,
							font, currentPage, endPage, endY  );
					}
				}

				textBefore = true;
			}
		}

		if( !text.text.isEmpty() )
			drawTextLineInTable( x, y, text, lineHeight, pdfData, links, font, currentPage,
				endPage, endY );

		y -= c_tableMargin - font->GetFontMetrics()->GetDescent();

		if( y < endY  && currentPage == pdfData.currentPageIdx )
			endY = y;

		++ column;
	}

	drawTableBorder( pdfData, startPage, ret, renderOpts, offset, table, startY, endY );

	pdfData.coords.y = endY;
	pdfData.painter->SetPage( pdfData.doc->GetPage( pdfData.currentPageIdx ) );

	processLinksInTable( pdfData, links, doc );

	return ret;
}

void
PdfRenderer::drawTableBorder( PdfAuxData & pdfData, int startPage, QVector< WhereDrawn > & ret,
	const RenderOpts & renderOpts, double offset, const QVector< QVector< CellData > > & table,
	double startY, double endY )
{
	for( int i = startPage; i <= pdfData.currentPageIdx; ++i )
	{
		pdfData.painter->SetPage( pdfData.doc->GetPage( i ) );

		pdfData.painter->Save();

		pdfData.painter->SetColor( renderOpts.m_borderColor.redF(),
			renderOpts.m_borderColor.greenF(),
			renderOpts.m_borderColor.blueF() );

		const auto startX = pdfData.coords.margins.left + offset;
		auto endX = startX;

		for( int c = 0; c < table.size(); ++ c )
			endX += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

		if( i == startPage )
		{
			pdfData.painter->DrawLine( startX, startY, endX, startY );

			auto x = startX;
			auto y = endY;

			if( i == pdfData.currentPageIdx )
			{
				pdfData.painter->DrawLine( startX, endY, endX, endY );
				pdfData.painter->DrawLine( x, startY, x, endY );
			}
			else
			{
				pdfData.painter->DrawLine( x, startY, x, pdfData.coords.margins.bottom );
				y = pdfData.coords.margins.bottom;
			}

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.painter->DrawLine( x, startY, x, y );
			}

			ret.append( { i, ( i < pdfData.currentPageIdx ? pdfData.coords.margins.bottom : endY ),
				( i < pdfData.currentPageIdx ? startY - pdfData.coords.margins.bottom : startY - endY  ) } );
		}
		else if( i < pdfData.currentPageIdx )
		{
			auto x = startX;
			auto y = pdfData.coords.margins.bottom;
			auto sy = pdfData.coords.pageHeight - pdfData.coords.margins.top;

			pdfData.painter->DrawLine( x, sy, x, y );

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.painter->DrawLine( x, sy, x, y );
			}

			ret.append( { i, pdfData.coords.margins.bottom,
				pdfData.coords.pageHeight - pdfData.coords.margins.top -
					pdfData.coords.margins.bottom } );
		}
		else
		{
			auto x = startX;
			auto y = endY;
			auto sy = pdfData.coords.pageHeight - pdfData.coords.margins.top;

			pdfData.painter->DrawLine( x, sy, x, y );

			for( int c = 0; c < table.size(); ++c )
			{
				x += table.at( c ).at( 0 ).width + c_tableMargin * 2.0;

				pdfData.painter->DrawLine( x, sy, x, y );
			}

			pdfData.painter->DrawLine( startX, y, endX, y );

			ret.append( { pdfData.currentPageIdx, endY,
				pdfData.coords.pageHeight - pdfData.coords.margins.top - endY } );
		}

		pdfData.painter->Restore();
	}
}

void
PdfRenderer::drawTextLineInTable( double x, double & y, TextToDraw & text, double lineHeight,
	PdfAuxData & pdfData, QMap< QString, QVector< QPair< QRectF, int > > > & links,
	PdfFont * font, int & currentPage, int & endPage, double & endY )
{
	y -= lineHeight;

	if( y < pdfData.coords.margins.bottom )
	{
		newPageInTable( pdfData, currentPage, endPage, endY );

		y = pdfData.coords.pageHeight - pdfData.coords.margins.top - lineHeight;
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
			pdfData.painter->Save();

			pdfData.painter->SetColor( it->background.redF(),
				it->background.greenF(),
				it->background.redF() );

			pdfData.painter->Rectangle( x, y + it->font->GetFontMetrics()->GetDescent(),
				it->width(), it->font->GetFontMetrics()->GetLineSpacing() );

			pdfData.painter->Fill();

			pdfData.painter->Restore();
		}

		pdfData.painter->Save();

		if( it->color.isValid() )
			pdfData.painter->SetColor( it->color.redF(),
				it->color.greenF(), it->color.blueF() );

		pdfData.painter->SetFont( it->font );
		pdfData.painter->DrawText( x, y, createPdfString( it->word.isEmpty() ?
			it->url : it->word ) );

		pdfData.painter->Restore();

		if( !it->url.isEmpty() )
			links[ it->url ].append( qMakePair( QRectF( x, y, it->width(), lineHeight ),
				currentPage ) );

		x += it->width();

		if( it + 1 != last )
		{
			auto tmpX = x;

			if( it->background.isValid() && it->font == ( it + 1 )->font )
			{
				pdfData.painter->Save();

				pdfData.painter->SetColor( it->background.redF(),
					it->background.greenF(),
					it->background.redF() );

				const auto sw = it->font->GetFontMetrics()->StringWidth( PdfString( " " ) );

				pdfData.painter->Rectangle( x, y + it->font->GetFontMetrics()->GetDescent(),
					sw, it->font->GetFontMetrics()->GetLineSpacing() );

				x += sw;

				pdfData.painter->Fill();

				pdfData.painter->Restore();
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
	if( currentPage + 1 > pdfData.currentPageIdx )
	{
		createPage( pdfData );

		if( pdfData.currentPageIdx > endPage )
		{
			endPage = pdfData.currentPageIdx;
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
	double offset, float fontScale )
{
	QVector< WhereDrawn > ret;

	switch( item->type() )
	{
		case MD::ItemType::Heading :
			return 0.0;

		case MD::ItemType::Paragraph :
		{
			ret = drawParagraph( pdfData, renderOpts, static_cast< MD::Paragraph* > ( item.data() ),
				doc, offset, true, CalcHeightOpt::Minimum, fontScale );
		}
			break;

		case MD::ItemType::Code :
		{
			ret = drawCode( pdfData, renderOpts, static_cast< MD::Code* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, fontScale );
		}
			break;

		case MD::ItemType::Blockquote :
		{
			ret = drawBlockquote( pdfData, renderOpts,
				static_cast< MD::Blockquote* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, fontScale );
		}
			break;

		case MD::ItemType::List :
		{
			auto * list = static_cast< MD::List* > ( item.data() );
			const auto bulletWidth = maxListNumberWidth( list );

			auto * font = createFont( m_opts.m_textFont, false, false,
				m_opts.m_textFontSize, pdfData.doc, fontScale );
			pdfData.coords.y -= font->GetFontMetrics()->GetLineSpacing();

			ret = drawList( pdfData, m_opts, list, m_doc, bulletWidth, offset,
				CalcHeightOpt::Minimum, fontScale );
		}
			break;

		case MD::ItemType::Table :
		{
			ret = drawTable( pdfData, renderOpts,
				static_cast< MD::Table* > ( item.data() ),
				doc, offset, CalcHeightOpt::Minimum, fontScale );
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
