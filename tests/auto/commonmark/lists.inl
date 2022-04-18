
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

// 5.3 Lists

TEST_CASE( "301" )
{
	const auto doc = load_test( 301 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 2 ).data() );
		REQUIRE( l->items().size() == 1 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}
	}
}

TEST_CASE( "302" )
{
	const auto doc = load_test( 302 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Ordered );
			REQUIRE( li->startNumber() == 1 );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Ordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 2 ).data() );
		REQUIRE( l->items().size() == 1 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Ordered );
			REQUIRE( li->startNumber() == 3 );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}
	}
}

TEST_CASE( "303" )
{
	const auto doc = load_test( 303 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "Foo" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 2 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}
	}
}

TEST_CASE( "304" )
{
	const auto doc = load_test( 304 );

	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
	REQUIRE( p->items().size() == 1 );
	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "The number of windows in my house is 14. "
		"The number of doors is 6." ) );
}

TEST_CASE( "305" )
{
	const auto doc = load_test( 305 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "The number of windows in my house is" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 2 ).data() );
		REQUIRE( l->items().size() == 1 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Ordered );
			REQUIRE( li->startNumber() == 1 );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "The number of doors is 6." ) );
		}
	}
}

TEST_CASE( "306" )
{
	const auto doc = load_test( 306 );

	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
	auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
	REQUIRE( l->items().size() == 3 );

	{
		REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
		auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
		REQUIRE( li->items().size() == 1 );
		REQUIRE( li->listType() == MD::ListItem::Unordered );
		REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "foo" ) );
	}

	{
		REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
		auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
		REQUIRE( li->items().size() == 1 );
		REQUIRE( li->listType() == MD::ListItem::Unordered );
		REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "bar" ) );
	}

	{
		REQUIRE( l->items().at( 2 )->type() == MD::ItemType::ListItem );
		auto li = static_cast< MD::ListItem* > ( l->items().at( 2 ).data() );
		REQUIRE( li->items().size() == 1 );
		REQUIRE( li->listType() == MD::ListItem::Unordered );
		REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
		REQUIRE( p->items().size() == 1 );
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "baz" ) );
	}
}

TEST_CASE( "307" )
{
	const auto doc = load_test( 307 );

	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
		REQUIRE( l->items().size() == 1 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 2 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );

			REQUIRE( li->items().at( 1 )->type() == MD::ItemType::List );
			auto l = static_cast< MD::List* > ( li->items().at( 1 ).data() );
			REQUIRE( l->items().size() == 1 );

			{
				REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
				auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
				REQUIRE( li->items().size() == 2 );
				REQUIRE( li->listType() == MD::ListItem::Unordered );
				REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
				auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
				REQUIRE( p->items().size() == 1 );
				REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
				auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
				REQUIRE( t->opts() == MD::TextWithoutFormat );
				REQUIRE( t->text() == QStringLiteral( "bar" ) );

				REQUIRE( li->items().at( 1 )->type() == MD::ItemType::List );
				auto l = static_cast< MD::List* > ( li->items().at( 1 ).data() );
				REQUIRE( l->items().size() == 1 );

				{
					REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
					auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
					REQUIRE( li->items().size() == 2 );
					REQUIRE( li->listType() == MD::ListItem::Unordered );

					{
						REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
						auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
						REQUIRE( p->items().size() == 1 );
						REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
						auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
						REQUIRE( t->opts() == MD::TextWithoutFormat );
						REQUIRE( t->text() == QStringLiteral( "baz" ) );
					}

					{
						REQUIRE( li->items().at( 1 )->type() == MD::ItemType::Paragraph );
						auto p = static_cast< MD::Paragraph* > ( li->items().at( 1 ).data() );
						REQUIRE( p->items().size() == 1 );
						REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
						auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
						REQUIRE( t->opts() == MD::TextWithoutFormat );
						REQUIRE( t->text() == QStringLiteral( "bim" ) );
					}
				}
			}
		}
	}
}

TEST_CASE( "308" )
{
	const auto doc = load_test( 308 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 2 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "bim" ) );
		}
	}
}

TEST_CASE( "309" )
{
	const auto doc = load_test( 309 );

	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );
		auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
		REQUIRE( l->items().size() == 2 );

		{
			REQUIRE( l->items().at( 0 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
			REQUIRE( li->items().size() == 2 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );

			{
				REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
				auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
				REQUIRE( p->items().size() == 1 );
				REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
				auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
				REQUIRE( t->opts() == MD::TextWithoutFormat );
				REQUIRE( t->text() == QStringLiteral( "foo" ) );
			}

			{
				REQUIRE( li->items().at( 1 )->type() == MD::ItemType::Paragraph );
				auto p = static_cast< MD::Paragraph* > ( li->items().at( 1 ).data() );
				REQUIRE( p->items().size() == 1 );
				REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
				auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
				REQUIRE( t->opts() == MD::TextWithoutFormat );
				REQUIRE( t->text() == QStringLiteral( "notcode" ) );
			}
		}

		{
			REQUIRE( l->items().at( 1 )->type() == MD::ItemType::ListItem );
			auto li = static_cast< MD::ListItem* > ( l->items().at( 1 ).data() );
			REQUIRE( li->items().size() == 1 );
			REQUIRE( li->listType() == MD::ListItem::Unordered );
			REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
			auto p = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
			REQUIRE( p->items().size() == 1 );
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
			auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
			REQUIRE( t->opts() == MD::TextWithoutFormat );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );
		}
	}

	REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( doc->items().at( 2 ).data() );
	REQUIRE( !c->inlined() );
	REQUIRE( c->text() == QStringLiteral( "code" ) );
}
