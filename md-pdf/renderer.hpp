
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
#include <QStack>

#ifdef MD_PDF_TESTING
#include <QFile>
#include <QTextStream>
#endif // MD_PDF_TESTING

// podofo include.
#include <podofo/podofo.h>

using namespace PoDoFo;

//! Footnote scale.
static const float c_footnoteScale = 0.75;


#ifdef MD_PDF_TESTING
struct DrawPrimitive {
	enum class Type {
		Text = 0,
		Line,
		Rectangle,
		Image,
		MultilineText
	};

	Type type;
	QString text;
	double x;
	double y;
	double x2;
	double y2;
	double width;
	double height;
	double xScale;
	double yScale;
};
#endif // MD_PDF_TESTING


//
// RenderOpts
//

//! Options for rendering.
struct RenderOpts
{
	//! Text font.
	QString m_textFont;
	//! Text font size.
	int m_textFontSize;
	//! Code font.
	QString m_codeFont;
	//! Code font size.
	int m_codeFontSize;
	//! Links color.
	QColor m_linkColor;
	//! Borders color.
	QColor m_borderColor;
	//! Code background.
	QColor m_codeBackground;
	//! Keyword color.
	QColor m_keywordColor;
	//! Comment color.
	QColor m_commentColor;
	//! Regular code color.
	QColor m_codeColor;
	//! Left margin.
	double m_left;
	//! Right margin.
	double m_right;
	//! Top margin.
	double m_top;
	//! Bottom margin.
	double m_bottom;

#ifdef MD_PDF_TESTING
	bool printDrawings = false;
	QVector< DrawPrimitive > testData;
#endif // MD_PDF_TESTING
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
	//! Progress of rendering.
	void progress( int percent );
	//! Error.
	void error( const QString & msg );
	//! Rendering is done.
	void done( bool terminated );
	//! Status message.
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

//! Mrgins.
struct PageMargins {
	double left = c_margin;
	double right = c_margin;
	double top = c_margin;
	double bottom = c_margin;
}; // struct PageMargins

//! Page current coordinates and etc...
struct CoordsPageAttribs {
	PageMargins margins;
	double pageWidth = 0.0;
	double pageHeight = 0.0;
	double x = 0.0;
	double y = 0.0;
}; // struct CoordsPageAttribs

class PdfRenderer;


//! Auxiliary struct for rendering.
struct PdfAuxData {
	//! Document.
	PdfMemDocument * doc = nullptr;
	//! Painter.
	PdfPainter * painter = nullptr;
	//! Page.
	PdfPage * page = nullptr;
	//! Index of the current page.
	int currentPageIdx = -1;
	//! Coordinates and margins.
	CoordsPageAttribs coords;
	//! Reserved spaces on the pages for footnotes.
	QMap< int, double > reserved;
	//! Drawing footnotes or the document?
	bool drawFootnotes = false;
	//! Current page index for drawing footnotes.
	int footnotePageIdx = -1;
	//! Current index of the footnote (for drawing number in the PDF).
	int currentFootnote = 1;
	//! Is this first item on the page?
	bool firstOnPage = true;
	//! Continue drawing of paragraph?
	bool continueParagraph = false;
	//! Current line height.
	double lineHeight = 0.0;
	//! Extra space before footnotes.
	double extraInFootnote = 0.0;
	//! Colors stack.
	QStack< QColor > m_colorsStack;

#ifdef MD_PDF_TESTING
	QMap< QString, QString > fonts;
	QSharedPointer< QFile > drawingsFile;
	QSharedPointer< QTextStream > drawingsStream;
	bool printDrawings = false;
	QVector< DrawPrimitive > testData;
	int testPos = 0;
	PdfRenderer * self = nullptr;
#endif // MD_PDF_TESTING

	//! \return Top Y coordinate on the page.
	double topY( int page ) const;
	//! \return Current page index.
	int currentPageIndex() const;
	//! \return Top footnote Y coordinate on the page.
	double topFootnoteY( int page ) const;
	//! \return Minimum allowe Y coordinate on the current page.
	double currentPageAllowedY() const;
	//! \return Minimum allowe Y coordinate on the page.
	double allowedY( int page ) const;
	//! Reserve space for drawing, i.e. move footnotes on the next page.
	void freeSpaceOn( int page );

	//! Draw text
	void drawText( double x, double y, const PdfString & text );
	//! Draw multiline text.
	void drawMultiLineText( double x, double y, double width, double height,
		const PdfString & text );
	//! Draw image.
	void drawImage( double x, double y, PdfImage * img, double xScale, double yScale );
	//! Draw line.
	void drawLine( double x1, double y1, double x2, double y2 );
	//! Save document.
	void save( const QString & fileName );
	//! Draw rectangle.
	void drawRectangle( double x, double y, double width, double height );

	//! Set color.
	void setColor( const QColor & c );
	//! Restore color.
	void restoreColor();
	//! Repeat color (needed after new page creation).
	void repeatColor();
}; // struct PdfAuxData;

//! Where was the item drawn?
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
	//! Internal signal for start rendering.
	void start();

public:
	PdfRenderer();
	~PdfRenderer() override = default;

	//! \return Is font can be created?
	static bool isFontCreatable( const QString & font );

	//! Convert QString to PdfString.
	static PdfString createPdfString( const QString & text );
	//! Convert PdfString to QString.
	static QString createQString( const PdfString & str );

public slots:
	//! Render document. \note Document can be changed during rendering.
	//! Don't reuse the same document twice.
	//! Renderer will delete himself on job finish.
	void render( const QString & fileName, QSharedPointer< MD::Document > doc,
		const RenderOpts & opts ) override;
	//! Terminate rendering.
	void terminate();

private slots:
	//! Real rendering.
	void renderImpl();
	//! Clean render.
	void clean() override;

private:
	//! Create font.
	PdfFont * createFont( const QString & name, bool bold, bool italic, float size,
		PdfMemDocument * doc, float scale, const PdfAuxData & pdfData );
	//! Create new page.
	void createPage( PdfAuxData & pdfData );

	//! Draw empty line.
	void moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
		double yOffsetMultiplier );
	//! Load image.
	QByteArray loadImage( MD::Image * item );
	//! Make all links clickable.
	void resolveLinks( PdfAuxData & pdfData );
	//! Max width of numbered list bullet.
	int maxListNumberWidth( MD::List * list ) const;

	//! What calculation of height to do?
	enum class CalcHeightOpt {
		//! Don't calculate, do drawing.
		Unknown = 0,
		//! Calculate minimum requred height (at least one line).
		Minimum = 1,
		//! Calculate full height.
		Full = 2
	}; // enum class CalcHeightOpt

	//! Finish pages.
	void finishPages( PdfAuxData & pdfData );

	//! Draw heading.
	QVector< WhereDrawn > drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Heading * item, QSharedPointer< MD::Document > doc, double offset,
		double nextItemMinHeight, CalcHeightOpt heightCalcOpt,
		float scale );
	//! Draw paragraph.
	QVector< WhereDrawn > drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Paragraph * item, QSharedPointer< MD::Document > doc, double offset,
		bool withNewLine, CalcHeightOpt heightCalcOpt,
		float scale, bool inFootnote );
	//! Draw block of code.
	QVector< WhereDrawn > drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code * item, QSharedPointer< MD::Document > doc, double offset,
		CalcHeightOpt heightCalcOpt,
		float scale );
	//! Draw blockquote.
	QVector< WhereDrawn > drawBlockquote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Blockquote * item, QSharedPointer< MD::Document > doc, double offset,
		CalcHeightOpt heightCalcOpt, float scale, bool inFootnote );
	//! Draw list.
	QVector< WhereDrawn > drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::List * item, QSharedPointer< MD::Document > doc, int bulletWidth,
		double offset = 0.0, CalcHeightOpt heightCalcOpt = CalcHeightOpt::Unknown,
		float scale = 1.0, bool inFootnote = false );
	//! Draw table.
	QVector< WhereDrawn > drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table * item, QSharedPointer< MD::Document > doc, double offset,
		CalcHeightOpt heightCalcOpt, float scale, bool inFootnote );

	//! \return Minimum necessary height to draw item, meant at least one line.
	double minNecessaryHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		QSharedPointer< MD::Item > item, QSharedPointer< MD::Document > doc,
		double offset, float scale, bool inFootnote );
	//! \return Height of the footnote.
	QVector< WhereDrawn > drawFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		QSharedPointer< MD::Document > doc, MD::Footnote * note,
		CalcHeightOpt heightCalcOpt );
	//! \return Height of the footnote.
	QVector< WhereDrawn > footnoteHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		QSharedPointer< MD::Document > doc, MD::Footnote * note );
	//! Reserve space for footnote.
	void reserveSpaceForFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		const QVector< WhereDrawn > & h, const double & currentY, int currentPage );
	//! Add footnote.
	void addFootnote( QSharedPointer< MD::Footnote > f, PdfAuxData & pdfData,
		const RenderOpts & renderOpts, QSharedPointer< MD::Document > doc );

	//! List item type.
	enum class ListItemType
	{
		Unknown,
		//! Ordered.
		Ordered,
		//! Unordered.
		Unordered
	}; // enum class ListItemType

	//! Draw list item.
	QVector< WhereDrawn > drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::ListItem * item, QSharedPointer< MD::Document > doc, int & idx,
		ListItemType & prevListItemType, int bulletWidth, double offset,
		CalcHeightOpt heightCalcOpt, float scale, bool inFootnote );

	//! Auxiliary struct for calculation of spaces scales to shrink text to width.
	struct CustomWidth {
		//! Item on line.
		struct Width {
			double width = 0.0;
			double height = 0.0;
			bool isSpace = false;
			bool isNewLine = false;
			bool shrink = true;
			QString word;
		}; // struct Width

		//! Append new item.
		void append( const Width & w ) { m_width.append( w ); }
		//! \return scale of space at line.
		double scale() { return m_scale.at( m_pos ); }
		//! Move to next line.
		void moveToNextLine() { ++m_pos; }
		//! Is drawing? This struct can be used to precalculate widthes and for actual drawing.
		bool isDrawing() const { return m_drawing; }
		//! Set drawing.
		void setDrawing( bool on = true ) { m_drawing = on; }

		//! \return Begin iterator.
		QVector< Width >::ConstIterator cbegin() const { return m_width.cbegin(); }
		//! \return End iterator.
		QVector< Width >::ConstIterator cend() const { return m_width.cend(); }

		//! \return Height of first item.
		double firstItemHeight() const;
		//! Calculate scales.
		void calcScale( double lineWidth );
		//! \return Total height.
		double totalHeight() const;

	private:
		//! Is drawing?
		bool m_drawing = false;
		//! Sizes of items.
		QVector< Width > m_width;
		//! Scales on lines.
		QVector< double > m_scale;
		//! Position of current line.
		int m_pos = 0;
	}; // struct CustomWidth

	//! Draw text.
	QVector< QPair< QRectF, int > > drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Text * item, QSharedPointer< MD::Document > doc, bool & newLine, PdfFont * footnoteFont,
		float footnoteFontScale, MD::Item * nextItem, int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote );
	//! Draw inlined code.
	QVector< QPair< QRectF, int > > drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote );
	//! Draw string.
	QVector< QPair< QRectF, int > > drawString( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		const QString & str, PdfFont * spaceFont, PdfFont * font, double lineHeight,
		QSharedPointer< MD::Document > doc, bool & newLine, PdfFont * footnoteFont,
		float footnoteFontScale, MD::Item * nextItem, int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, const QColor & background,
		bool inFootnote );
	//! Draw link.
	QVector< QPair< QRectF, int > > drawLink( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Link * item, QSharedPointer< MD::Document > doc, bool & newLine, PdfFont * footnoteFont,
		float footnoteFontScale, MD::Item * nextItem, int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, float scale, bool inFootnote );
	//! Draw image.
	QPair< QRectF, int > drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Image * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw, float scale );

	//! Item in the table's cell.
	struct CellItem {
		QString word;
		QByteArray image;
		QString url;
		QString footnote;
		QColor color;
		QColor background;
		PdfFont * font = nullptr;
		QSharedPointer< MD::Footnote > footnoteObj;

		//! \return Width of the item.
		double width( PdfAuxData & pdfData ) const;
	}; // struct CellItem

	//! Cell in the table.
	struct CellData {
		double width = 0.0;
		double height = 0.0;
		MD::Table::Alignment alignment;
		QVector< CellItem > items;

		void setWidth( double w ) { width = w; }
		//! Calculate height for the given width.
		void heightToWidth( double lineHeight, double spaceWidth, float scale,
			PdfAuxData & pdfData );
	}; //  struct CellData

	//! \return Height of the row.
	double rowHeight( const QVector< QVector< CellData > > & table, int row );
	//! Create auxiliary table for drawing.
	QVector< QVector< CellData > >
	createAuxTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table * item, QSharedPointer< MD::Document > doc,
		float scale, bool inFootnote );
	//! Calculate size of the cells in the table.
	void calculateCellsSize( PdfAuxData & pdfData, QVector< QVector< CellData > > & auxTable,
		double spaceWidth, double offset, double lineHeight, float scale );
	//! Draw table's row.
	QVector< WhereDrawn > drawTableRow( QVector< QVector< CellData > > & table, int row,
		PdfAuxData & pdfData, double offset, double lineHeight,
		const RenderOpts & renderOpts, QSharedPointer< MD::Document > doc,
		QVector< QSharedPointer< MD::Footnote > > & footnotes,
		float scale, bool inFootnote );
	//! Draw table border.
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

		void clear() { width = 0.0; text.clear(); }
	}; // struct TextToDraw

	//! Draw text line in the cell.
	void drawTextLineInTable( double x, double & y, TextToDraw & text, double lineHeight,
		PdfAuxData & pdfData, QMap< QString, QVector< QPair< QRectF, int > > > & links,
		PdfFont * font, int & currentPage, int & endPage, double & endY,
		QVector< QSharedPointer< MD::Footnote > > & footnotes, bool inFootnote );
	//! Create new page in table.
	void newPageInTable( PdfAuxData & pdfData, int & currentPage, int & endPage,
		double & endY );
	//! Make links in table clickable.
	void processLinksInTable( PdfAuxData & pdfData,
		const QMap< QString, QVector< QPair< QRectF, int > > > & links,
		QSharedPointer< MD::Document > doc );

	//! Draw horizontal line.
	void drawHorizontalLine( PdfAuxData & pdfData, const RenderOpts & renderOpts );

private:
	//! Name of the output file.
	QString m_fileName;
	//! Markdown document.
	QSharedPointer< MD::Document > m_doc;
	//! Render options.
	RenderOpts m_opts;
	//! Mutex.
	QMutex m_mutex;
	//! Termination flag.
	bool m_terminate;
	//! All destinations in the document.
	QMap< QString, PdfDestination > m_dests;
	//! Links that not yet clickable.
	QMultiMap< QString, QVector< QPair< QRectF, int > > > m_unresolvedLinks;
	//! Cache of images.
	QMap< QString, QByteArray > m_imageCache;
	//! Footnote counter.
	int m_footnoteNum;
	//! Footnotes to draw.
	QVector< QSharedPointer< MD::Footnote > > m_footnotes;
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
