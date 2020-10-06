
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

#include <md-pdf/syntax.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// doctest include.
#include <doctest/doctest.h>


TEST_CASE( "simple for" )
{
	static const QStringList code = { QStringLiteral( "for( int i = 0; i < 10; ++i )" ),
		QStringLiteral( "\tdoSomething();" ) };

	auto s = Syntax::createSyntaxHighlighter( QStringLiteral( "cpp" ) );

	const auto w = s->prepare( code );

	REQUIRE( w.size() == 5 );

	REQUIRE( w.at( 0 ).startPos == 0 );
	REQUIRE( w.at( 0 ).endPos == 2 );
	REQUIRE( w.at( 0 ).line == 0 );
	REQUIRE( w.at( 0 ).color == Syntax::ColorRole::Keyword );

	REQUIRE( w.at( 1 ).startPos == 3 );
	REQUIRE( w.at( 1 ).endPos == 4 );
	REQUIRE( w.at( 1 ).line == 0 );
	REQUIRE( w.at( 1 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 2 ).startPos == 5 );
	REQUIRE( w.at( 2 ).endPos == 7 );
	REQUIRE( w.at( 2 ).line == 0 );
	REQUIRE( w.at( 2 ).color == Syntax::ColorRole::Keyword );

	REQUIRE( w.at( 3 ).startPos == 8 );
	REQUIRE( w.at( 3 ).endPos == 28 );
	REQUIRE( w.at( 3 ).line == 0 );
	REQUIRE( w.at( 3 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 4 ).startPos == 0 );
	REQUIRE( w.at( 4 ).endPos == 14 );
	REQUIRE( w.at( 4 ).line == 1 );
	REQUIRE( w.at( 4 ).color == Syntax::ColorRole::Regular );
}

TEST_CASE( "comments" )
{
	static const QStringList code = {
		QStringLiteral( "//comment" ),
		QStringLiteral( "for/*comment*//*comment*/( int i = 0; i < 10; ++i )//comment*/" ),
		QStringLiteral( "/*comment*/ /*comment*/\tdoSomething();/*comment" ),
		QStringLiteral( "comment" ),
		QStringLiteral( "comment*/do{};" ) };

	auto s = Syntax::createSyntaxHighlighter( QStringLiteral( "cpp" ) );

	const auto w = s->prepare( code );

	REQUIRE( w.size() == 17 );

	REQUIRE( w.at( 0 ).startPos == 0 );
	REQUIRE( w.at( 0 ).endPos == 8 );
	REQUIRE( w.at( 0 ).line == 0 );
	REQUIRE( w.at( 0 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 1 ).startPos == 0 );
	REQUIRE( w.at( 1 ).endPos == 2 );
	REQUIRE( w.at( 1 ).line == 1 );
	REQUIRE( w.at( 1 ).color == Syntax::ColorRole::Keyword );

	REQUIRE( w.at( 2 ).startPos == 3 );
	REQUIRE( w.at( 2 ).endPos == 13 );
	REQUIRE( w.at( 2 ).line == 1 );
	REQUIRE( w.at( 2 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 3 ).startPos == 14 );
	REQUIRE( w.at( 3 ).endPos == 24 );
	REQUIRE( w.at( 3 ).line == 1 );
	REQUIRE( w.at( 3 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 4 ).startPos == 25 );
	REQUIRE( w.at( 4 ).endPos == 26 );
	REQUIRE( w.at( 4 ).line == 1 );
	REQUIRE( w.at( 4 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 5 ).startPos == 27 );
	REQUIRE( w.at( 5 ).endPos == 29 );
	REQUIRE( w.at( 5 ).line == 1 );
	REQUIRE( w.at( 5 ).color == Syntax::ColorRole::Keyword );

	REQUIRE( w.at( 6 ).startPos == 30 );
	REQUIRE( w.at( 6 ).endPos == 50 );
	REQUIRE( w.at( 6 ).line == 1 );
	REQUIRE( w.at( 6 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 7 ).startPos == 51 );
	REQUIRE( w.at( 7 ).endPos == 61 );
	REQUIRE( w.at( 7 ).line == 1 );
	REQUIRE( w.at( 7 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 8 ).startPos == 0 );
	REQUIRE( w.at( 8 ).endPos == 10 );
	REQUIRE( w.at( 8 ).line == 2 );
	REQUIRE( w.at( 8 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 9 ).startPos == 11 );
	REQUIRE( w.at( 9 ).endPos == 11 );
	REQUIRE( w.at( 9 ).line == 2 );
	REQUIRE( w.at( 9 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 10 ).startPos == 12 );
	REQUIRE( w.at( 10 ).endPos == 22 );
	REQUIRE( w.at( 10 ).line == 2 );
	REQUIRE( w.at( 10 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 11 ).startPos == 23 );
	REQUIRE( w.at( 11 ).endPos == 37 );
	REQUIRE( w.at( 11 ).line == 2 );
	REQUIRE( w.at( 11 ).color == Syntax::ColorRole::Regular );

	REQUIRE( w.at( 12 ).startPos == 38 );
	REQUIRE( w.at( 12 ).endPos == 46 );
	REQUIRE( w.at( 12 ).line == 2 );
	REQUIRE( w.at( 12 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 13 ).startPos == 0 );
	REQUIRE( w.at( 13 ).endPos == 6 );
	REQUIRE( w.at( 13 ).line == 3 );
	REQUIRE( w.at( 13 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 14 ).startPos == 0 );
	REQUIRE( w.at( 14 ).endPos == 8 );
	REQUIRE( w.at( 14 ).line == 4 );
	REQUIRE( w.at( 14 ).color == Syntax::ColorRole::Comment );

	REQUIRE( w.at( 15 ).startPos == 9 );
	REQUIRE( w.at( 15 ).endPos == 10 );
	REQUIRE( w.at( 15 ).line == 4 );
	REQUIRE( w.at( 15 ).color == Syntax::ColorRole::Keyword );

	REQUIRE( w.at( 16 ).startPos == 11 );
	REQUIRE( w.at( 16 ).endPos == 13 );
	REQUIRE( w.at( 16 ).line == 4 );
	REQUIRE( w.at( 16 ).color == Syntax::ColorRole::Regular );
}

