
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <md-pdf/md_parser.hpp>
#include <QDir>


inline QSharedPointer< MD::Document >
load_test( int n )
{
	auto fileName = QString::number( n );

	if( fileName.size() < 3 )
		fileName.prepend( QString( 3 - fileName.size(), QLatin1Char( '0' ) ) );

	fileName.prepend( QStringLiteral( "data/" ) );
	fileName.append( QStringLiteral( ".md" ) );

	MD::Parser p;

	return p.parse( fileName, false );
}


TEST_CASE( "198" )
{
	const auto doc = load_test( 198 );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Table );
	const auto t = static_cast< MD::Table* > ( doc->items().at( 1 ).data() );
	REQUIRE( t->columnsCount() == 2 );
	REQUIRE( t->rows().size() == 2 );

	REQUIRE( t->columnAlignment( 0 ) == MD::Table::AlignLeft );
	REQUIRE( t->columnAlignment( 1 ) == MD::Table::AlignLeft );

	{
		REQUIRE( t->rows().at( 0 )->type() == MD::ItemType::TableRow );
		const auto r = static_cast< MD::TableRow* > ( t->rows().at( 0 ).data() );
		REQUIRE( r->cells().size() == 2 );

		{
			REQUIRE( r->cells().at( 0 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 0 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "foo" ) );
		}

		{
			REQUIRE( r->cells().at( 1 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 1 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}
	}

	{
		REQUIRE( t->rows().at( 1 )->type() == MD::ItemType::TableRow );
		const auto r = static_cast< MD::TableRow* > ( t->rows().at( 1 ).data() );
		REQUIRE( r->cells().size() == 2 );

		{
			REQUIRE( r->cells().at( 0 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 0 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}

		{
			REQUIRE( r->cells().at( 1 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 1 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "bim" ) );
		}
	}
}

TEST_CASE( "199" )
{
	const auto doc = load_test( 199 );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Table );
	const auto t = static_cast< MD::Table* > ( doc->items().at( 1 ).data() );
	REQUIRE( t->columnsCount() == 2 );
	REQUIRE( t->rows().size() == 2 );

	REQUIRE( t->columnAlignment( 0 ) == MD::Table::AlignCenter );
	REQUIRE( t->columnAlignment( 1 ) == MD::Table::AlignRight );

	{
		REQUIRE( t->rows().at( 0 )->type() == MD::ItemType::TableRow );
		const auto r = static_cast< MD::TableRow* > ( t->rows().at( 0 ).data() );
		REQUIRE( r->cells().size() == 2 );

		{
			REQUIRE( r->cells().at( 0 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 0 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "abc" ) );
		}

		{
			REQUIRE( r->cells().at( 1 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 1 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "defghi" ) );
		}
	}

	{
		REQUIRE( t->rows().at( 1 )->type() == MD::ItemType::TableRow );
		const auto r = static_cast< MD::TableRow* > ( t->rows().at( 1 ).data() );
		REQUIRE( r->cells().size() == 2 );

		{
			REQUIRE( r->cells().at( 0 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 0 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "bar" ) );
		}

		{
			REQUIRE( r->cells().at( 1 )->type() == MD::ItemType::TableCell );
			const auto c = static_cast< MD::TableCell* > ( r->cells().at( 1 ).data() );
			REQUIRE( c->items().at( 0 )->type() == MD::ItemType::Text );
			const auto t = static_cast< MD::Text* > ( c->items().at( 0 ).data() );
			REQUIRE( t->text() == QStringLiteral( "baz" ) );
		}
	}
}
