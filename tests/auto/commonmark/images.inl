
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

// 6.4 Images

TEST_CASE( "571" )
{
	const auto doc = load_test( 571 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Image );
	auto i = static_cast< MD::Image* > ( p->items().at( 0 ).data() );
	REQUIRE( !i->isEmpty() );
	REQUIRE( i->text() == QStringLiteral( "foo" ) );
	REQUIRE( i->url() == QStringLiteral( "/url" ) );
}

TEST_CASE( "572" )
{
	const auto doc = load_test( 572 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Image );
	auto i = static_cast< MD::Image* > ( p->items().at( 0 ).data() );
	REQUIRE( !i->isEmpty() );
	REQUIRE( i->text() == QStringLiteral( "foo *bar*" ) );
	REQUIRE( i->url() == QStringLiteral( "train.jpg" ) );
}

TEST_CASE( "573" )
{
	const auto doc = load_test( 573 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Image );
	auto i = static_cast< MD::Image* > ( p->items().at( 0 ).data() );
	REQUIRE( !i->isEmpty() );
	REQUIRE( i->text() == QStringLiteral( "foo ![bar](/url)" ) );
	REQUIRE( i->url() == QStringLiteral( "/url2" ) );
}
