
/*!
	\file

	\author Igor Mironchik (igor.mironchik at gmail dot com).

	Copyright (c) 2022 Igor Mironchik

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

#include <md-pdf/md_parser.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// doctest include.
#include <doctest/doctest.h>


QSharedPointer< MD::Document > load_test( int n )
{
	auto fileName = QString::number( n );

	if( fileName.size() < 3 )
		fileName.prepend( QString( 3 - fileName.size(), QLatin1Char( '0' ) ) );

	fileName.prepend( QStringLiteral( "0.30/" ) );
	fileName.append( QStringLiteral( ".md" ) );

	MD::Parser p;

	return p.parse( fileName );
}

TEST_CASE( "001" )
{
	const auto doc = load_test( 1 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Code );

	auto c = static_cast< MD::Code* > ( doc->items().at( 1 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "foo    baz        bim" ) );
}
