
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

#include "load.hpp"

#include <doctest/doctest.h>

// 2.5 Entity and numeric character references

TEST_CASE( "25" )
{
	const auto doc = load_test( 25 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QString( QChar( 0x26 ) ) +
		MD::c_32 + QChar( 0xA9 ) +
		MD::c_32 + QChar( 0xC6 ) +
		MD::c_32 + QChar( 0x10E) +
		MD::c_32 + QChar( 0xBE ) +
		MD::c_32 + QChar( 0x210B ) +
		MD::c_32 + QChar( 0x2146 ) +
		MD::c_32 + QChar( 0x2232 ) +
		MD::c_32 + QChar( 0x2267 ) + QChar( 0x0338 ) );
}

TEST_CASE( "26" )
{
	const auto doc = load_test( 26 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QString( QChar( 35 ) ) +
		MD::c_32 + QChar( 1234 ) +
		MD::c_32 + QChar( 992 ) +
		MD::c_32 + QChar( 0xFFFD) );
}
