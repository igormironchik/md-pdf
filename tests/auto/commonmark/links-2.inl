
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

TEST_CASE( "526" )
{
	const auto doc = load_test( 526 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "foo" ) );
	REQUIRE( doc->labeledLinks().contains( l->url() ) );
	REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/url" ) );
}

TEST_CASE( "527" )
{
	const auto doc = load_test( 527 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link [foo [bar]]" ) );
	REQUIRE( doc->labeledLinks().contains( l->url() ) );
	REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "528" )
{
	const auto doc = load_test( 528 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link [bar" ) );
	REQUIRE( doc->labeledLinks().contains( l->url() ) );
	REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "529" )
{
	const auto doc = load_test( 529 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->img()->isEmpty() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "link *foo **bar** `#`*" ) );
	REQUIRE( doc->labeledLinks().contains( l->url() ) );
	REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "530" )
{
	const auto doc = load_test( 530 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
	auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
	REQUIRE( l->textOptions() == MD::TextWithoutFormat );
	REQUIRE( l->text() == QStringLiteral( "![moon](moon.jpg)" ) );
	REQUIRE( !l->img()->isEmpty() );
	REQUIRE( l->img()->text() == QStringLiteral( "moon" ) );
	REQUIRE( l->img()->url() == QStringLiteral( "moon.jpg" ) );
	REQUIRE( doc->labeledLinks().contains( l->url() ) );
	REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
}

TEST_CASE( "531" )
{
	const auto doc = load_test( 531 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 4 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "[foo" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 1 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "bar" ) );
		REQUIRE( l->url() == QStringLiteral( "/uri" ) );
	}

	{
		REQUIRE( p->items().at( 2 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 2 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "]" ) );
	}

	{
		REQUIRE( p->items().at( 3 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 3 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "ref" ) );
		REQUIRE( doc->labeledLinks().contains( l->url() ) );
		REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
	}
}

TEST_CASE( "532" )
{
	const auto doc = load_test( 532 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 5 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "[foo" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 1 ).data() );
		REQUIRE( t->opts() == MD::ItalicText );
		REQUIRE( t->text() == QStringLiteral( "bar" ) );
	}

	{
		REQUIRE( p->items().at( 2 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 2 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::ItalicText );
		REQUIRE( l->text() == QStringLiteral( "baz" ) );
		REQUIRE( doc->labeledLinks().contains( l->url() ) );
		REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
	}

	{
		REQUIRE( p->items().at( 3 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 3 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "]" ) );
	}

	{
		REQUIRE( p->items().at( 4 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 4 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "ref" ) );
		REQUIRE( doc->labeledLinks().contains( l->url() ) );
		REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
	}
}

TEST_CASE( "533" )
{
	const auto doc = load_test( 533 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 2 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "*" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 1 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "foo*" ) );
		REQUIRE( doc->labeledLinks().contains( l->url() ) );
		REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
	}
}

TEST_CASE( "534" )
{
	const auto doc = load_test( 534 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 2 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Link );
		auto l = static_cast< MD::Link* > ( p->items().at( 0 ).data() );
		REQUIRE( l->img()->isEmpty() );
		REQUIRE( l->textOptions() == MD::TextWithoutFormat );
		REQUIRE( l->text() == QStringLiteral( "foo *bar" ) );
		REQUIRE( doc->labeledLinks().contains( l->url() ) );
		REQUIRE( doc->labeledLinks()[ l->url() ]->url() == QStringLiteral( "/uri" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 1 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "*" ) );
	}
}

TEST_CASE( "535" )
{
	const auto doc = load_test( 535 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "[foo <bar attr=\"][ref]\">" ) );
	}
}
