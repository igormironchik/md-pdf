
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

public:
	Renderer() = default;
	~Renderer() override = default;

	virtual void render( const QString & fileName, QSharedPointer< MD::Document > doc,
		const RenderOpts & opts ) = 0;
	virtual void clean() = 0;
}; // class Renderer


static const double c_margin = 25.0;

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
	PdfStreamedDocument * doc = nullptr;
	PdfPainter * painter = nullptr;
	PdfPage * page = nullptr;
	CoordsPageAttribs coords;
}; // struct PdfAuxData;


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

	void render( const QString & fileName, QSharedPointer< MD::Document > doc,
		const RenderOpts & opts ) override;
	void terminate();

private slots:
	void renderImpl();
	void clean() override;

private:
	PdfFont * createFont( const QString & name, bool bold, bool italic, float size,
		PdfStreamedDocument * doc );
	void createPage( PdfAuxData & pdfData );
	PdfString createPdfString( const QString & text );
	QString createQString( const PdfString & str );
	void drawHeading( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Heading * item, QSharedPointer< MD::Document > doc );
	void drawText( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Text * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false );
	void moveToNewLine( PdfAuxData & pdfData, double xOffset, double yOffset,
		double yOffsetMultiplier = 1.0 );
	void drawParagraph( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Paragraph * item, QSharedPointer< MD::Document > doc, double offset = 0.0 );
	void drawInlinedCode( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Code * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph );
	QVector< QPair< QRectF, PdfPage* > > drawString( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		const QString & str, bool bold, bool italic, bool strikethrough,
		QSharedPointer< MD::Document > doc, bool & newLine, double offset,
		bool firstInParagraph );
	void drawLink( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Link * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false );
	QPair< QRectF, PdfPage* > drawImage( PdfAuxData & pdfData, const RenderOpts & renderOpts,
		MD::Image * item, QSharedPointer< MD::Document > doc, bool & newLine, double offset = 0.0,
		bool firstInParagraph = false );
	QImage loadImage( MD::Image * item );

private:
	QString m_fileName;
	QSharedPointer< MD::Document > m_doc;
	RenderOpts m_opts;
	QMutex m_mutex;
	bool m_terminate;
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