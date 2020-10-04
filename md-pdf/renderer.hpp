
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

#ifndef MD_PDF_RENDERER_HPP_INCLUDED
#define MD_PDF_RENDERER_HPP_INCLUDED

// md-pdf include.
#include "md_doc.hpp"

// Qt include.
#include <QColor>
#include <QObject>
#include <QMutex>
#include <QImage>
#include <QNetworkReply>

// podofo include.
#include <podofo/podofo.h>

using namespace PoDoFo;


//
// RenderOpts
//

//! Options for rendering.
struct RenderOpts
{
	QString m_textFont;
	int m_textFontSize;
	QString m_codeFont;
	int m_codeFontSize;
	QColor m_linkColor;
	QColor m_borderColor;
	QColor m_codeBackground;
	double m_left;
	double m_right;
	double m_top;
	double m_bottom;
}; // struct RenderOpts


//
// Renderer
//

//! Abstract renderer.
class Renderer
	:	public QObject
{
	Q_OBJECT

signals:
	void progress( int percent );
	void error( const QString & msg );
	void done( bool terminated );
	void status( const QString & msg );

public:
	Renderer() = default;
	~Renderer() override = default;

	virtual void render( const QString & fileName, QSharedPointer< MD::Document > doc,
		const RenderOpts & opts ) = 0;
	virtual void clean() = 0;
}; // class Renderer


static const double c_margin = 72.0 / 25.4 * 20.0;
static const double c_beforeHeading = 15.0;
static const double c_blockquoteBaseOffset = 10.0;
static const double c_blockquoteMarkWidth = 3.0;
static const double c_tableMargin = 2.0;

struct PageMargins {
	double left = c_margin;
	double right = c_margin;
	double top = c_margin;
	double bottom = c_margin;
}; // struct PageMargins

struct CoordsPageAttribs {
	PageMargins margins;
	double pageWidth = 0.0;
	double pageHeight = 0.0;
	double x = 0.0;
	double y = 0.0;
}; // struct CoordsPageAttribs

struct PdfAuxData {
	PdfMemDocument * doc = nullptr;
	PdfPainter * painter = nullptr;
	PdfPage * page = nullptr;
	int currentPageIdx = -1;
	CoordsPageAttribs coords;
}; // struct PdfAuxData;

struct WhereDrawn {
	int pageIdx = 0;
	double y = 0.0;
	double height = 0.0;
}; // struct WhereDrawn


//
// PdfRenderer
//

//! Renderer to PDF.
class PdfRenderer
	:	public Renderer
{
	Q_OBJECT

signals:
	void start();

public:
	PdfRenderer();
	~PdfRenderer() override = default;

public slots:
	//! Render document. \note Document can be changed during rendering.
	//! Don't reuse the same document twice.
	void render( const QString & fileName, QSharedPointer< MD::Document > doc,
		const RenderOpts & opts ) override;
	//! Terminate rendering.
	void terminate();

private slots:
	void renderImpl();
	void clean() override;

private:
	PdfFont * createFont( const QString & name, bool bold, bool italic, float size,
		PdfMemDocument * doc );
	void createPage( PdfAuxData & pdfData );
	static PdfString createPdfString( const QString & text );
	static QString createQString( const PdfString & str );

	void moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
		double yOffsetMultiplier = 1.0 );
	QImage loadImage( MD::Image * item );
	void resolveLinks( PdfAuxData & pdfData );
	int maxListNumberWidth( MD::List * list ) const;

	QVector< WhereDrawn > drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Heading * item, QSharedPointer< MD::Document > doc, double offset = 0.0,
		double nextItemMinHeight = 0.0 );
	QVector< WhereDrawn > drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Paragraph * item, QSharedPointer< MD::Document > doc, double offset = 0.0,
		bool withNewLine = true, bool justCalcHeight = false );
	QVector< WhereDrawn > drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code * item, QSharedPointer< MD::Document > doc, double offset = 0.0,
		bool justCalcHeight = false );
	QVector< WhereDrawn > drawBlockquote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Blockquote * item, QSharedPointer< MD::Document > doc, double offset = 0.0,
		bool justCalcHeight = false );
	QVector< WhereDrawn > drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::List * item, QSharedPointer< MD::Document > doc, int bulletWidth,
		double offset = 0.0, bool justCalcHeight = false );
	QVector< WhereDrawn > drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table * item, QSharedPointer< MD::Document > doc, double offset = 0.0,
		bool justCalcHeight = false );

	//! \return Minimum necessary height to draw item, meant at least one line.
	double minNecessaryHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		QSharedPointer< MD::Item > item, QSharedPointer< MD::Document > doc,
		double offset );

	enum class ListItemType
	{
		Unknown,
		Ordered,
		Unordered
	}; // enum class ListItemType

	QVector< WhereDrawn > drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::ListItem * item, QSharedPointer< MD::Document > doc, int & idx,
		ListItemType & prevListItemType, int bulletWidth, double offset = 0.0,
		bool justCalcHeight = false );

	struct CustomWidth {
		struct Width {
			double width = 0.0;
			double height = 0.0;
			bool isSpace = false;
			bool isNewLine = false;
			bool shrink = true;
			QString word;
		}; // struct Width

		void append( const Width & w ) { m_width.append( w ); }
		double scale() { return m_scale.at( m_pos ); }
		void moveToNextLine() { ++m_pos; }
		bool isDrawing() const { return m_drawing; }
		void setDrawing( bool on = true ) { m_drawing = on; }
		double firstItemHeight() const
		{
			if( !m_width.isEmpty() )
				return m_width.constFirst().height;
			else
				return 0.0;
		}

		void calcScale( double lineWidth )
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
						auto ss = ( lineWidth - w + sw ) / sw;

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

	private:
		bool m_drawing = false;
		QVector< Width > m_width;
		QVector< double > m_scale;
		int m_pos = 0;
	}; // struct CustomWidth

	QVector< QPair< QRectF, int > > drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Text * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false, CustomWidth * cw = nullptr );
	QVector< QPair< QRectF, int > > drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw = nullptr );
	QVector< QPair< QRectF, int > > drawString( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		const QString & str, PdfFont * spaceFont, PdfFont * font, double lineHeight,
		QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw = nullptr, const QColor & background = QColor() );
	QVector< QPair< QRectF, int > > drawLink( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Link * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false, CustomWidth * cw = nullptr );
	QPair< QRectF, int > drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Image * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false, CustomWidth * cw = nullptr );

	struct CellItem {
		QString word;
		QImage image;
		QString url;
		QColor color;
		QColor background;
		PdfFont * font = nullptr;

		double width() const
		{
			if( !word.isEmpty() )
				return font->GetFontMetrics()->StringWidth( createPdfString( word ) );
			else if( !image.isNull() )
				return image.width();
			else if( !url.isEmpty() )
				return font->GetFontMetrics()->StringWidth( createPdfString( url ) );
			else
				return 0.0;
		}
	}; // struct CellItem

	struct CellData {
		double width = 0.0;
		double height = 0.0;
		MD::Table::Alignment alignment;
		QVector< CellItem > items;

		void setWidth( double w )
		{
			width = w;
		}

		void heightToWidth( double lineHeight, double spaceWidth )
		{
			height = 0.0;

			bool newLine = true;

			double w = 0.0;

			for( auto it = items.cbegin(), last = items.cend(); it != last; ++it )
			{
				if( it->image.isNull() )
				{
					if( newLine )
						height += lineHeight;

					w += it->width();

					if( w >= width )
						newLine = true;

					double sw = spaceWidth;

					if( it != items.cbegin() && it->font == ( it - 1 )->font )
						sw = it->font->GetFontMetrics()->StringWidth( PdfString( " " ) );

					if( it + 1 != last )
					{
						if( w + sw + ( it + 1 )->width() > width )
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
					height += it->image.height() / ( it->image.width() / width );
					newLine = true;
				}
			}
		}
	}; //  struct CellData

	double rowHeight( const QVector< QVector< CellData > > & table, int row )
	{
		double h = 0.0;

		for( auto it = table.cbegin(), last = table.cend(); it != last; ++it )
		{
			if( (*it)[ row ].height > h )
				h = (*it)[ row ].height;
		}

		return  h;
	}

	QVector< QVector< CellData > >
	createAuxTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table * item, QSharedPointer< MD::Document > doc );
	void calculateCellsSize( PdfAuxData & pdfData, QVector< QVector< CellData > > & auxTable,
		double spaceWidth, double offset, double lineHeight );
	QVector< WhereDrawn > drawTableRow( QVector< QVector< CellData > > & table, int row,
		PdfAuxData & pdfData, double offset, double lineHeight,
		const RenderOpts & renderOpts, QSharedPointer< MD::Document > doc );
	void drawTableBorder( PdfAuxData & pdfData, int startPage, QVector< WhereDrawn > & ret,
		const RenderOpts & renderOpts, double offset, const QVector< QVector< CellData > > & table,
		double startY, double endY );

	// Holder of single line in table.
	struct TextToDraw {
		double width = 0.0;
		double availableWidth = 0.0;
		double lineHeight = 0.0;
		MD::Table::Alignment alignment;
		QVector< CellItem > text;

		void clear()
		{
			width = 0.0;
			text.clear();
		}
	}; // struct TextToDraw

	void drawTextLineInTable( double x, double & y, TextToDraw & text, double lineHeight,
		PdfAuxData & pdfData, QMap< QString, QVector< QPair< QRectF, int > > > & links,
		PdfFont * font, int & currentPage, int & endPage, double & endY );
	void newPageInTable( PdfAuxData & pdfData, int & currentPage, int & endPage,
		double & endY );
	void processLinksInTable( PdfAuxData & pdfData,
		const QMap< QString, QVector< QPair< QRectF, int > > > & links,
		QSharedPointer< MD::Document > doc );

private:
	QString m_fileName;
	QSharedPointer< MD::Document > m_doc;
	RenderOpts m_opts;
	QMutex m_mutex;
	bool m_terminate;
	QMap< QString, PdfDestination > m_dests;
	QMultiMap< QString, QVector< QPair< QRectF, int > > > m_unresolvedLinks;
	QMap< QString, QImage > m_imageCache;
}; // class Renderer


//
// LoadImageFromNetwork
//

//! Loader of image from network.
class LoadImageFromNetwork final
	:	public QObject
{
	Q_OBJECT

signals:
	void start();

public:
	LoadImageFromNetwork( const QUrl & url, QThread * thread );
	~LoadImageFromNetwork() override = default;

	const QImage & image() const;
	void load();

private slots:
	void loadImpl();
	void loadFinished();
	void loadError( QNetworkReply::NetworkError );

private:
	Q_DISABLE_COPY( LoadImageFromNetwork )

	QThread * m_thread;
	QImage m_img;
	QNetworkReply * m_reply;
	QUrl m_url;
}; // class LoadImageFromNetwork

#endif // MD_PDF_RENDERER_HPP_INCLUDED
