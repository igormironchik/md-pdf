
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

