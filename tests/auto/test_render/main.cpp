
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

#include "test_footnotes_data.hpp"
#include "test_table1_data.hpp"
#include "test_table2_data.hpp"
#include "test_blockquote_data.hpp"

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
}; // class TestRender

namespace /* anonymous */ {

void
testRendering( const QString & fileName, const QVector< DrawPrimitive > & data,
	bool printPrimitives )
{
	try {
		MD::Parser parser;

		auto doc = parser.parse( fileName, false );

		RenderOpts opts;
		opts.m_borderColor = Qt::black;
		opts.m_bottom = 50.0;
		opts.m_codeBackground = Qt::black;
		opts.m_codeColor = Qt::black;
		opts.m_codeFont = QStringLiteral( "Courier New" );
		opts.m_codeFontSize = 8.0;
		opts.m_commentColor = Qt::black;
		opts.m_keywordColor = Qt::black;
		opts.m_left = 50.0;
		opts.m_linkColor = Qt::black;
		opts.m_right = 50.0;
		opts.m_textFont = QStringLiteral( "Droid Serif" );
		opts.m_textFontSize = 8.0;
		opts.m_top = 50.0;

		opts.testData = data;
		opts.printDrawings = printPrimitives;

		auto * render = new PdfRenderer;

		QSignalSpy spy( render, &PdfRenderer::done );

		render->render( QString(), doc, opts );

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

} /* namespace anonymous */

void
TestRender::testFootnotes()
{
	testRendering( c_folder + QStringLiteral( "/../../manual/footnotes.md" ),
		c_testFootnotesData, false );
}

void
TestRender::testTableWithImage()
{
	testRendering( c_folder + QStringLiteral( "/../../manual/table.md" ),
		c_testTableWithImagesData, false );
}

void
TestRender::testTableWithText()
{
	testRendering( c_folder + QStringLiteral( "/../../manual/table2.md" ),
		c_testTableWithTextData, false );
}

void
TestRender::testBlockquote()
{
	testRendering( c_folder + QStringLiteral( "/../../manual/blockquote.md" ),
		c_testBlockquoteData, false );
}


QTEST_GUILESS_MAIN( TestRender )

#include "main.moc"
