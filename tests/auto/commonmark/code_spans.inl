
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

// 6 Inlines

TEST_CASE( "327" )
{
	const auto doc = load_test( 327 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 2 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( p->items().at( 0 ).data() );
	REQUIRE( c->inlined() );
	REQUIRE( c->text() == QStringLiteral( "hi" ) );
	REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 1 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "lo`" ) );
}

// 6.1 Code spans

TEST_CASE( "328" )
{
	const auto doc = load_test( 328 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( p->items().at( 0 ).data() );
	REQUIRE( c->inlined() );
	REQUIRE( c->text() == QStringLiteral( "foo" ) );
}

TEST_CASE( "329" )
{
	const auto doc = load_test( 329 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( p->items().at( 0 ).data() );
	REQUIRE( c->inlined() );
	REQUIRE( c->text() == QStringLiteral( "foo ` bar" ) );
}