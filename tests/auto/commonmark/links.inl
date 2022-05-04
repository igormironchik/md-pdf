
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

// 6.3 Links

TEST_CASE( "481" )
{
	const auto doc = load_test( 481 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "482" )
{
	const auto doc = load_test( 482 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "483" )
{
	const auto doc = load_test( 483 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text().isEmpty() );
	REQUIRE( l->url() == QStringLiteral( "./target.md" ) );
}

TEST_CASE( "484" )
{
	const auto doc = load_test( 484 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url().isEmpty() );
}

TEST_CASE( "485" )
{
	const auto doc = load_test( 485 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url().isEmpty() );
}

TEST_CASE( "486" )
{
	const auto doc = load_test( 486 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text().isEmpty() );
	REQUIRE( l->url().isEmpty() );
}

TEST_CASE( "487" )
{
	const auto doc = load_test( 487 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](/my uri)" ) );
}

TEST_CASE( "488" )
{
	const auto doc = load_test( 488 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/my uri" ) );
}

TEST_CASE( "489" )
{
	const auto doc = load_test( 489 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](foo bar)" ) );
}

TEST_CASE( "490" )
{
	const auto doc = load_test( 490 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](<foo bar>)" ) );
}

TEST_CASE( "491" )
{
	const auto doc = load_test( 491 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "a" ) );
	REQUIRE( l->url() == QStringLiteral( "b)c" ) );
}

TEST_CASE( "492" )
{
	const auto doc = load_test( 492 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](<foo>)" ) );
}

TEST_CASE( "493" )
{
	const auto doc = load_test( 493 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[a](<b)c [a](<b)c> [a](<b>c)" ) );
}

TEST_CASE( "494" )
{
	const auto doc = load_test( 494 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "(foo)" ) );
}

TEST_CASE( "495" )
{
	const auto doc = load_test( 495 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "foo(and(bar))" ) );
}

TEST_CASE( "496" )
{
	const auto doc = load_test( 496 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](foo(and(bar))" ) );
}

TEST_CASE( "497" )
{
	const auto doc = load_test( 497 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "foo(and(bar)" ) );
}

TEST_CASE( "498" )
{
	const auto doc = load_test( 498 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "foo(and(bar)" ) );
}

TEST_CASE( "499" )
{
	const auto doc = load_test( 499 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "foo):" ) );
}

TEST_CASE( "500" )
{
	const auto doc = load_test( 500 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 4 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );

		const QString fn = QStringLiteral( "/" ) + QDir().absolutePath() +
			QStringLiteral( "/0.30/500.md" );

		REQUIRE( l->url() == QStringLiteral( "#fragment" ) + fn );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );
		REQUIRE( l->url() == QStringLiteral( "http://example.com#fragment" ) );
	}

	{
		REQUIRE( doc->items().at( 3 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 3 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );
		REQUIRE( l->url() == QStringLiteral( "http://example.com?foo=3#frag" ) );
	}
}

TEST_CASE( "501" )
{
	const auto doc = load_test( 501 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "foo\\bar" ) );
}

TEST_CASE( "502" )
{
	MESSAGE( "This test is not strict to CommonMark 0.30." );

	const auto doc = load_test( 502 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	// I don't convert HTML entities.
	REQUIRE( l->url() == QStringLiteral( "foo%20b&auml;" ) );
}

TEST_CASE( "503" )
{
	const auto doc = load_test( 503 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "\"title\"" ) );
}

TEST_CASE( "504" )
{
	const auto doc = load_test( 504 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 3 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );
		REQUIRE( l->url() == QStringLiteral( "/url" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 1 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );
		REQUIRE( l->url() == QStringLiteral( "/url" ) );
	}

	{
		REQUIRE( p->items().at( 2 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 2 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "link" ) );
		REQUIRE( l->url() == QStringLiteral( "/url" ) );
	}
}

TEST_CASE( "505" )
{
	const auto doc = load_test( 505 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/url" ) );
}

TEST_CASE( "506" )
{
	const auto doc = load_test( 506 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/url" ) + QChar( 160 ) + QStringLiteral( "\"title\"" ) );
}

TEST_CASE( "507" )
{
	const auto doc = load_test( 507 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "[link](/url \"title \"and\" title\")" ) );
}

TEST_CASE( "508" )
{
	const auto doc = load_test( 508 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/url" ) );
}

TEST_CASE( "509" )
{
	const auto doc = load_test( 509 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link" ) );
	REQUIRE( l->url() == QStringLiteral( "/uri" ) );
}
