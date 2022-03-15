
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

TEST_CASE( "002" ) // Not strict to CommonMark
{
	const auto doc = load_test( 2 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Code );

	auto c = static_cast< MD::Code* > ( doc->items().at( 1 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "  foo    baz        bim" ) );
}

TEST_CASE( "003" )
{
	const auto doc = load_test( 3 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Code );

	auto c = static_cast< MD::Code* > ( doc->items().at( 1 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "a    a\nὐ    a" ) );
}

TEST_CASE( "004" )
{
	const auto doc = load_test( 4 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );

	auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
	REQUIRE( l->items().size() == 1 );

	auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
	REQUIRE( li->listType() == MD::ListItem::Unordered );
	REQUIRE( li->items().size() == 2 );

	REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
	auto p1 = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
	REQUIRE( p1->items().size() == 1 );
	REQUIRE( p1->items().at( 0 )->type() == MD::ItemType::Text );
	auto t1 = static_cast< MD::Text* > ( p1->items().at( 0 ).data() );
	REQUIRE( t1->opts() == MD::TextWithoutFormat );
	REQUIRE( t1->text() == QStringLiteral( "foo" ) );

	REQUIRE( li->items().at( 1 )->type() == MD::ItemType::Paragraph );
	auto p2 = static_cast< MD::Paragraph* > ( li->items().at( 1 ).data() );
	REQUIRE( p2->items().size() == 1 );
	REQUIRE( p2->items().at( 0 )->type() == MD::ItemType::Text );
	auto t2 = static_cast< MD::Text* > ( p2->items().at( 0 ).data() );
	REQUIRE( t2->opts() == MD::TextWithoutFormat );
	REQUIRE( t2->text() == QStringLiteral( "bar" ) );
}

TEST_CASE( "005" )
{
	const auto doc = load_test( 5 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );

	auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
	REQUIRE( l->items().size() == 1 );

	auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
	REQUIRE( li->listType() == MD::ListItem::Unordered );
	REQUIRE( li->items().size() == 2 );

	REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Paragraph );
	auto p1 = static_cast< MD::Paragraph* > ( li->items().at( 0 ).data() );
	REQUIRE( p1->items().size() == 1 );
	REQUIRE( p1->items().at( 0 )->type() == MD::ItemType::Text );
	auto t1 = static_cast< MD::Text* > ( p1->items().at( 0 ).data() );
	REQUIRE( t1->opts() == MD::TextWithoutFormat );
	REQUIRE( t1->text() == QStringLiteral( "foo" ) );

	REQUIRE( li->items().at( 1 )->type() == MD::ItemType::Code );
	auto c2 = static_cast< MD::Code* > ( li->items().at( 1 ).data() );
	REQUIRE( c2->inlined() == false );
	REQUIRE( c2->syntax().isEmpty() );
	REQUIRE( c2->text() == QStringLiteral( "  bar" ) );
}

TEST_CASE( "006" ) // Not strict to CommonMark
{
	const auto doc = load_test( 6 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Blockquote );

	auto q = static_cast< MD::Blockquote* > ( doc->items().at( 1 ).data() );
	REQUIRE( q->items().size() == 1 );

	REQUIRE( q->items().at( 0 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( q->items().at( 0 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "    foo" ) );
}

TEST_CASE( "007" ) // Not strict to CommonMark.
{
	const auto doc = load_test( 7 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::List );

	auto l = static_cast< MD::List* > ( doc->items().at( 1 ).data() );
	REQUIRE( l->items().size() == 1 );

	auto li = static_cast< MD::ListItem* > ( l->items().at( 0 ).data() );
	REQUIRE( li->listType() == MD::ListItem::Unordered );
	REQUIRE( li->items().size() == 1 );

	REQUIRE( li->items().at( 0 )->type() == MD::ItemType::Code );
	auto c = static_cast< MD::Code* > ( li->items().at( 0 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "   foo" ) );
}

TEST_CASE( "008" )
{
	const auto doc = load_test( 8 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );
	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Code );

	auto c = static_cast< MD::Code* > ( doc->items().at( 1 ).data() );
	REQUIRE( c->inlined() == false );
	REQUIRE( c->syntax().isEmpty() );
	REQUIRE( c->text() == QStringLiteral( "foo\nbar" ) );
}
