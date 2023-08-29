
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

// md4qt include.
#define MD4QT_QT_SUPPORT
#include <md4qt/traits.hpp>
#include <md4qt/doc.hpp>

// Qt include.
#include <QColor>
#include <QObject>
#include <QMutex>
#include <QImage>
#include <QNetworkReply>
#include <QStack>
#include <QByteArray>

#ifdef MD_PDF_TESTING
#include <QFile>
#include <QTextStream>
#endif // MD_PDF_TESTING

// podofo include.
#include <podofo/podofo.h>

// C++ include.
#include <memory>
#include <string_view>

// nd-pdf include.
#include "syntax.hpp"

using namespace PoDoFo;

//! Footnote scale.
static const double c_footnoteScale = 0.75;


#ifdef MD_PDF_TESTING
struct DrawPrimitive {
	enum class Type {
		Text = 0,
		Line,
		Rectangle,
		Image,
		MultilineText,
		Unknown
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
	//! Math font.
	QString m_mathFont;
	//! Math font size.
	int m_mathFontSize;
	//! Links color.
	QColor m_linkColor;
	//! Borders color.
	QColor m_borderColor;
	//! Left margin.
	double m_left;
	//! Right margin.
	double m_right;
	//! Top margin.
	double m_top;
	//! Bottom margin.
	double m_bottom;
	//! DPI.
	quint16 m_dpi;
	//! Syntax highlighter.
	std::shared_ptr< Syntax > m_syntax;

#ifdef MD_PDF_TESTING
	bool printDrawings = false;
	QVector< DrawPrimitive > testData;
	QString testDataFileName;
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

	virtual void render( const QString & fileName,
			std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		const RenderOpts & opts, bool testing = false ) = 0;
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
	//! Anchors in document.
	QStringList anchors;
	//! Reserved spaces on the pages for footnotes.
	QMap< unsigned int, double > reserved;
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
	QStack< QColor > colorsStack;
	//! DPI.
	quint16 dpi;
	//! Markdown document.
	std::shared_ptr< MD::Document< MD::QStringTrait > > md;
	//! Syntax highlighter.
	std::shared_ptr< Syntax > syntax;

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
	void drawText( double x, double y, const char * text, PdfFont * font, double size,
		double scale, bool strikeout );
	//! Draw image.
	void drawImage( double x, double y, PdfImage * img, double xScale, double yScale );
	//! Draw line.
	void drawLine( double x1, double y1, double x2, double y2 );
	//! Save document.
	void save( const QString & fileName );
	//! Draw rectangle.
	void drawRectangle( double x, double y, double width, double height, PdfPathDrawMode m );

	//! Set color.
	void setColor( const QColor & c );
	//! Restore color.
	void restoreColor();
	//! Repeat color (needed after new page creation).
	void repeatColor();
}; // struct PdfAuxData;

//! Where was the item drawn?
struct WhereDrawn {
	int pageIdx = -1;
	double y = 0.0;
	double height = 0.0;
}; // struct WhereDrawn

struct Utf8String {
	QByteArray data;

	operator const char * () const { return data.data(); };
	operator std::string_view () const { return data.data(); };
}; // struct Utf8String


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

	//! Convert QString to UTF-8.
	static Utf8String createPdfString( const QString & text );
	//! Convert UTF-8 to QString.
	static QString createQString( const char * str );

#ifdef MD_PDF_TESTING
	bool isError() const;
#endif

public slots:
	//! Render document. \note Document can be changed during rendering.
	//! Don't reuse the same document twice.
	//! Renderer will delete himself on job finish.
	void render( const QString & fileName, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		const RenderOpts & opts, bool testing = false ) override;
	//! Terminate rendering.
	void terminate();

private slots:
	//! Real rendering.
	void renderImpl();
	//! Clean render.
	void clean() override;

protected:
	friend struct CellItem;
	friend struct CellData;
#ifdef MD_PDF_TESTING
	friend struct TestRendering;
#endif

	//! Create font.
	PdfFont * createFont( const QString & name, bool bold, bool italic, double size,
		PdfMemDocument * doc, double scale, const PdfAuxData & pdfData );

private:
	//! Create new page.
	void createPage( PdfAuxData & pdfData );

	//! Draw empty line.
	void moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
		double yOffsetMultiplier, double yOffsetOnNewPage );
	//! Load image.
	QByteArray loadImage( MD::Image< MD::QStringTrait > * item );
	//! Make all links clickable.
	void resolveLinks( PdfAuxData & pdfData );
	//! Max width of numbered list bullet.
	int maxListNumberWidth( MD::List< MD::QStringTrait > * list ) const;

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
	QPair< QVector< WhereDrawn >, WhereDrawn > drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Heading< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
		double nextItemMinHeight, CalcHeightOpt heightCalcOpt,
		double scale, bool withNewLine = true );
	//! Draw paragraph.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Paragraph< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
		bool withNewLine, CalcHeightOpt heightCalcOpt,
		double scale, bool inFootnote );
	//! Draw block of code.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
		CalcHeightOpt heightCalcOpt,
		double scale );
	//! Draw blockquote.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawBlockquote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Blockquote< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
		CalcHeightOpt heightCalcOpt, double scale, bool inFootnote );
	//! Draw list.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawList( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::List< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, int bulletWidth,
		double offset = 0.0, CalcHeightOpt heightCalcOpt = CalcHeightOpt::Unknown,
		double scale = 1.0, bool inFootnote = false, bool nested = false );
	//! Draw table.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, double offset,
		CalcHeightOpt heightCalcOpt, double scale, bool inFootnote );

	//! \return Minimum necessary height to draw item, meant at least one line.
	double minNecessaryHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		std::shared_ptr< MD::Item< MD::QStringTrait > > item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		double offset, double scale, bool inFootnote );
	//! \return Height of the footnote.
	QVector< WhereDrawn > drawFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		const QString & footnoteRefId, MD::Footnote< MD::QStringTrait > * note,
		CalcHeightOpt heightCalcOpt, double * lineHeight = nullptr );
	//! \return Height of the footnote.
	QVector< WhereDrawn > footnoteHeight( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		MD::Footnote< MD::QStringTrait > * note, double * lineHeight );
	//! Reserve space for footnote.
	void reserveSpaceForFootnote( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		const QVector< WhereDrawn > & h, const double & currentY, int currentPage,
		double lineHeight, bool addExtraLine = false );
	//! Add footnote.
	void addFootnote( const QString & refId,
		std::shared_ptr< MD::Footnote< MD::QStringTrait > > f, PdfAuxData & pdfData,
		const RenderOpts & renderOpts, std::shared_ptr< MD::Document< MD::QStringTrait > > doc );

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
	QPair< QVector< WhereDrawn >, WhereDrawn > drawListItem( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::ListItem< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, int & idx,
		ListItemType & prevListItemType, int bulletWidth, double offset,
		CalcHeightOpt heightCalcOpt, double scale, bool inFootnote, bool firstInList );

	//! Auxiliary struct for calculation of spaces scales to shrink text to width.
	struct CustomWidth {
		//! Item on line.
		struct Width {
			double width = 0.0;
			double height = 0.0;
			double descent = 0.0;
			bool isSpace = false;
			bool isNewLine = false;
			bool shrink = true;
			QString word;
		}; // struct Width

		//! Append new item.
		void append( const Width & w ) { m_width.append( w ); }
		//! \return Scale of space at line.
		double scale() { return m_scale.at( m_pos ); }
		//! \return Height of the line.
		double height() { return m_height.at( m_pos ); }
		//! \return Descent of the line.
		double descent() { return m_descent.at( m_pos ); }
		//! Move to next line.
		void moveToNextLine() { ++m_pos; }
		//! Is drawing? This struct can be used to precalculate widthes and for actual drawing.
		bool isDrawing() const { return m_drawing; }
		//! Set drawing.
		void setDrawing( bool on = true ) { m_drawing = on; }
		//! \return Is empty?
		bool isEmpty() const { return m_width.isEmpty(); }

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
		//! Heights of lines.
		QVector< double > m_height;
		//! Descents of lines.
		QVector< double > m_descent;
		//! Position of current line.
		int m_pos = 0;
	}; // struct CustomWidth

	//! Draw text.
	QVector< QPair< QRectF, unsigned int > > drawText( PdfAuxData & pdfData,
		const RenderOpts & renderOpts, MD::Text< MD::QStringTrait > * item,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine,
		PdfFont * footnoteFont, double footnoteFontSize, double footnoteFontScale,
		MD::Item< MD::QStringTrait > * nextItem, int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, double scale, bool inFootnote );
	//! Draw inlined code.
	QVector< QPair< QRectF, unsigned int > > drawInlinedCode( PdfAuxData & pdfData,
		const RenderOpts & renderOpts, MD::Code< MD::QStringTrait > * item,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw, double scale, bool inFootnote );
	//! Draw string.
	QVector< QPair< QRectF, unsigned int > > drawString( PdfAuxData & pdfData,
		const RenderOpts & renderOpts, const QString & str,
		PdfFont * spaceFont, double spaceFontSize, double spaceFontScale,
		PdfFont * font, double fontSize, double fontScale,
		double lineHeight,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine,
		PdfFont * footnoteFont, double footnoteFontSize, double footnoteFontScale,
		MD::Item< MD::QStringTrait > * nextItem,
		int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, const QColor & background,
		bool inFootnote, bool strikeout );
	//! Draw link.
	QVector< QPair< QRectF, unsigned int > > drawLink( PdfAuxData & pdfData,
		const RenderOpts & renderOpts, MD::Link< MD::QStringTrait > * item,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine,
		PdfFont * footnoteFont, double footnoteFontSize, double footnoteFontScale,
		MD::Item< MD::QStringTrait > * nextItem, int footnoteNum, double offset,
		bool firstInParagraph, CustomWidth * cw, double scale, bool inFootnote );
	//! Draw image.
	QPair< QRectF, unsigned int > drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Image< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc, bool & newLine, double offset,
		bool firstInParagraph, CustomWidth * cw, double scale );
	//! Draw math expression.
	QPair< QRectF, unsigned int > drawMathExpr( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Math< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		bool & newLine, double offset, bool hasNext,
		bool firstInParagraph, CustomWidth * cw, double scale );

	//! Font in table.
	struct Font {
		QString family;
		bool bold;
		bool italic;
		bool strikethrough;
		int size;
	}; // struct Font

	friend bool operator != ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 );
	friend bool operator == ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 );

	//! Item in the table's cell.
	struct CellItem {
		QString word;
		QByteArray image;
		QString url;
		QString footnote;
		QString footnoteRef;
		QColor color;
		QColor background;
		std::shared_ptr< MD::Footnote< MD::QStringTrait > > footnoteObj;
		Font font;

		//! \return Width of the item.
		double width( PdfAuxData & pdfData, PdfRenderer * render, double scale ) const;
	}; // struct CellItem

	//! Cell in the table.
	struct CellData {
		double width = 0.0;
		double height = 0.0;
		MD::Table< MD::QStringTrait >::Alignment alignment;
		QVector< CellItem > items;

		void setWidth( double w ) { width = w; }
		//! Calculate height for the given width.
		void heightToWidth( double lineHeight, double spaceWidth, double scale,
			PdfAuxData & pdfData, PdfRenderer * render );
	}; //  struct CellData

	//! \return Height of the row.
	double rowHeight( const QVector< QVector< CellData > > & table, int row );
	//! Create auxiliary table for drawing.
	QVector< QVector< CellData > >
	createAuxTable( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Table< MD::QStringTrait > * item, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		double scale, bool inFootnote );
	//! Calculate size of the cells in the table.
	void calculateCellsSize( PdfAuxData & pdfData, QVector< QVector< CellData > > & auxTable,
		double spaceWidth, double offset, double lineHeight, double scale );
	//! Draw table's row.
	QPair< QVector< WhereDrawn >, WhereDrawn > drawTableRow(
		QVector< QVector< CellData > > & table, int row,
		PdfAuxData & pdfData, double offset, double lineHeight,
		const RenderOpts & renderOpts, std::shared_ptr< MD::Document< MD::QStringTrait > > doc,
		QVector< QPair< QString, std::shared_ptr< MD::Footnote< MD::QStringTrait > > > > & footnotes,
		double scale, bool inFootnote );
	//! Draw table border.
	void drawRowBorder( PdfAuxData & pdfData, int startPage, QVector< WhereDrawn > & ret,
		const RenderOpts & renderOpts, double offset, const QVector< QVector< CellData > > & table,
		double startY, double endY );

	// Holder of single line in table.
	struct TextToDraw {
		double width = 0.0;
		double availableWidth = 0.0;
		double lineHeight = 0.0;
		MD::Table< MD::QStringTrait >::Alignment alignment;
		QVector< CellItem > text;

		void clear() { width = 0.0; text.clear(); }
	}; // struct TextToDraw

	//! Draw text line in the cell.
	void drawTextLineInTable( double x, double & y, TextToDraw & text, double lineHeight,
		PdfAuxData & pdfData, QMap< QString, QVector< QPair< QRectF, unsigned int > > > & links,
		PdfFont * font, int & currentPage, int & endPage, double & endY,
		QVector< QPair< QString, std::shared_ptr< MD::Footnote< MD::QStringTrait > > > > & footnotes,
		bool inFootnote, double scale );
	//! Create new page in table.
	void newPageInTable( PdfAuxData & pdfData, int & currentPage, int & endPage,
		double & endY );
	//! Make links in table clickable.
	void processLinksInTable( PdfAuxData & pdfData,
		const QMap< QString, QVector< QPair< QRectF, unsigned int > > > & links,
		std::shared_ptr< MD::Document< MD::QStringTrait > > doc );

	//! Draw horizontal line.
	void drawHorizontalLine( PdfAuxData & pdfData, const RenderOpts & renderOpts );

	//! Handle rendering exception.
	void handleException( PdfAuxData & pdfData, const QString & msg );

private:
	//! Name of the output file.
	QString m_fileName;
	//! Markdown document.
	std::shared_ptr< MD::Document< MD::QStringTrait > > m_doc;
	//! Render options.
	RenderOpts m_opts;
	//! Mutex.
	QMutex m_mutex;
	//! Termination flag.
	bool m_terminate;
	//! All destinations in the document.
	QMap< QString, std::shared_ptr< PdfDestination > > m_dests;
	//! Links that not yet clickable.
	QMultiMap< QString, QVector< QPair< QRectF, unsigned int > > > m_unresolvedLinks;
	//! Footnotes links.
	QMap< QString, QPair< QRectF, unsigned int > > m_unresolvedFootnotesLinks;
	//! Cache of images.
	QMap< QString, QByteArray > m_imageCache;
	//! Footnote counter.
	int m_footnoteNum;
	//! Footnotes to draw.
	QVector< QPair< QString, std::shared_ptr< MD::Footnote< MD::QStringTrait > > > > m_footnotes;
#ifdef MD_PDF_TESTING
	bool m_isError;
#endif
}; // class Renderer


bool operator != ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 );
bool operator == ( const PdfRenderer::Font & f1, const PdfRenderer::Font & f2 );


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
