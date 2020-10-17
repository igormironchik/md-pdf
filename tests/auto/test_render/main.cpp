
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

#include <md-pdf/renderer.hpp>
#include <md-pdf/md_parser.hpp>
#include <md-pdf/md_doc.hpp>

#include <test_const.hpp>

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QVector>

//
// TestRender
//

class TestRender final
	:	public QObject
{
	Q_OBJECT

private slots:
	//! Test footnotes rendering.
	void testFootnotes();
	//! Test table with images.
	void testTableWithImage();
	//! Test table with text.
	void testTableWithText();
	//! Test blockquotes.
	void testBlockquote();
	//! Complex test.
	void testComplex();
	//! Test image in text.
	void testImageInText();

	//! Test footnotes rendering.
	void testFootnotesBigFont();
	//! Test table with images.
	void testTableWithImageBigFont();
	//! Test table with text.
	void testTableWithTextBigFont();
	//! Test blockquotes.
	void testBlockquoteBigFont();
	//! Complex test.
	void testComplexBigFont();
	//! Test image in text.
	void testImageInTextBigFont();

	//! Test code.
	void testCode();
	//! Test complex 2.
	void testComplex2();
}; // class TestRender

namespace /* anonymous */ {

//! Prepare test data or do actual test?
static const bool c_printData = false;

void
testRendering( const QString & fileName, const QString & suffix,
	const QVector< DrawPrimitive > & data, double textFontSize, double codeFontSize )
{
	try {
		MD::Parser parser;

		auto doc = parser.parse( c_folder + QStringLiteral( "/../../manual/" ) + fileName, false );

		RenderOpts opts;
		opts.m_borderColor = QColor( 81, 81, 81 );
		opts.m_bottom = 50.0;
		opts.m_codeBackground = QColor( 222, 222, 222 );
		opts.m_codeColor = QColor( 0, 0, 0 );
		opts.m_codeFont = QStringLiteral( "Courier New" );
		opts.m_codeFontSize = codeFontSize;
		opts.m_commentColor = QColor( 0, 128, 0 );
		opts.m_keywordColor = QColor( 128, 128, 0 );
		opts.m_left = 50.0;
		opts.m_linkColor = QColor( 33, 122, 255 );
		opts.m_right = 50.0;
		opts.m_textFont = QStringLiteral( "Droid Serif" );
		opts.m_textFontSize = textFontSize;
		opts.m_top = 50.0;

		opts.testData = data;
		opts.printDrawings = c_printData;
		opts.testDataFileName = c_folder + QStringLiteral( "/" ) + fileName + suffix +
			QStringLiteral( ".data" );

		auto * render = new PdfRenderer;

		QSignalSpy spy( render, &PdfRenderer::done );

		render->render( QStringLiteral( "./" ) + fileName + suffix + QStringLiteral( ".pdf" ),
			doc, opts );

		int i = 0;

		while( !spy.count() && i++ < 150 )
			QTest::qWait( 250 );

		if( !spy.count() )
			QFAIL( "Render didn't send done() signal. Test aborted." );
	}
	catch( const MD::ParserException & )
	{
		QFAIL( "Parsing of Markdown failed. Test aborted." );
	}
}

DrawPrimitive::Type
toType( const QString & t )
{
	if( t == QStringLiteral( "Text" ) )
		return DrawPrimitive::Type::Text;
	else if( t == QStringLiteral( "MultilineText" ) )
		return DrawPrimitive::Type::MultilineText;
	else if( t == QStringLiteral( "Line" ) )
		return DrawPrimitive::Type::Line;
	else if( t == QStringLiteral( "Rectangle" ) )
		return DrawPrimitive::Type::Rectangle;
	else if( t == QStringLiteral( "Image" ) )
		return DrawPrimitive::Type::Image;
	else
		return DrawPrimitive::Type::Unknown;
}

QString
readQuotedString( QTextStream & s, int length )
{
	QString ret;
	QChar c;

	while( c != QLatin1Char( '"' ) )
		s >> c;

	for( int i = 0; i < length; ++i )
	{
		s >> c;
		ret.append( c );
	}

	s >> c;

	return ret;
}

QVector< DrawPrimitive >
loadTestData( const QString & fileName, const QString & suffix )
{
	QVector< DrawPrimitive > data;

	QFile file( c_folder + QStringLiteral( "/" ) + fileName + suffix + QStringLiteral( ".data" ) );

	if( !file.open( QIODevice::ReadOnly ) )
		return data;

	QTextStream stream( &file );
	stream.setCodec( QTextCodec::codecForName( "UTF-8" ) );

	while( !stream.atEnd() )
	{
		auto str = stream.readLine();

		if( str.isEmpty() )
			break;

		QTextStream s( &str );
		DrawPrimitive d;

		QString type;
		s >> type;
		d.type = toType( type );

		int length = 0;
		s >> length;

		d.text = readQuotedString( s, length );

		s >> d.x;
		s >> d.y;
		s >> d.x2;
		s >> d.y2;
		s >> d.width;
		s >> d.height;
		s >> d.xScale;
		s >> d.yScale;

		data.append( d );
	}

	file.close();

	return data;
}

void
doTest( const QString & fileName, const QString & suffix,
	double textFontSize, double codeFontSize )
{
	QVector< DrawPrimitive > data;

	if( !c_printData )
	{
		data = loadTestData( fileName, suffix );

		if( data.isEmpty() )
			QFAIL( "Failed to load test data." );
	}

	testRendering( fileName, suffix, data, textFontSize, codeFontSize );
}

} /* namespace anonymous */

void
TestRender::testFootnotes()
{
	doTest( QStringLiteral( "footnotes.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testTableWithImage()
{
	doTest( QStringLiteral( "table.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testTableWithText()
{
	doTest( QStringLiteral( "table2.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testBlockquote()
{
	doTest( QStringLiteral( "blockquote.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testComplex()
{
	doTest( QStringLiteral( "complex.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testImageInText()
{
	doTest( QStringLiteral( "image_in_text.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testFootnotesBigFont()
{
	doTest( QStringLiteral( "footnotes.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testTableWithImageBigFont()
{
	doTest( QStringLiteral( "table.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testTableWithTextBigFont()
{
	doTest( QStringLiteral( "table2.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testBlockquoteBigFont()
{
	doTest( QStringLiteral( "blockquote.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testComplexBigFont()
{
	doTest( QStringLiteral( "complex.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testImageInTextBigFont()
{
	doTest( QStringLiteral( "image_in_text.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}

void
TestRender::testCode()
{
	doTest( QStringLiteral( "code.md" ), QString(), 8.0, 8.0 );
}

void
TestRender::testComplex2()
{
	doTest( QStringLiteral( "complex2.md" ), QStringLiteral( "_big" ), 16.0, 14.0 );
}


QTEST_GUILESS_MAIN( TestRender )

#include "main.moc"
