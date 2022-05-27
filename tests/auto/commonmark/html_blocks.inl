
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

// 4.6 HTML blocks

TEST_CASE( "148" )
{
	const auto doc = load_test( 148 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 4 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<table><tr><td>\n<pre>\n**Hello**," ) );
	}

	REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
	REQUIRE( p->items().size() == 3 );

	{
		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::ItalicText );
		REQUIRE( t->text() == QStringLiteral( "world" ) );
	}

	{
		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 1 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "." ) );
	}

	{
		REQUIRE( p->items().at( 2 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( p->items().at( 2 ).data() );
		REQUIRE( h->text() == QStringLiteral( "</pre>" ) );
	}

	{
		REQUIRE( doc->items().at( 3 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 3 ).data() );
		REQUIRE( h->text() == QStringLiteral( "</td></tr></table>" ) );
	}
}

TEST_CASE( "149" )
{
	const auto doc = load_test( 149 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<table>\n  <tr>\n    <td>\n"
			"           hi\n    </td>\n  </tr>\n</table>" ) );
	}

	REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::TextWithoutFormat );
	REQUIRE( t->text() == QStringLiteral( "okay." ) );
}

TEST_CASE( "150" )
{
	const auto doc = load_test( 150 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div>\n  *hello*\n         <foo><a>" ) );
	}
}

TEST_CASE( "151" )
{
	const auto doc = load_test( 151 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "</div>\n*foo*" ) );
	}
}

TEST_CASE( "152" )
{
	const auto doc = load_test( 152 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 4 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<DIV CLASS=\"foo\">" ) );
	}

	REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::ItalicText );
	REQUIRE( t->text() == QStringLiteral( "Markdown" ) );

	{
		REQUIRE( doc->items().at( 3 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 3 ).data() );
		REQUIRE( h->text() == QStringLiteral( "</DIV>" ) );
	}
}

TEST_CASE( "153" )
{
	const auto doc = load_test( 153 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div id=\"foo\"\n  class=\"bar\">\n</div>" ) );
	}
}

TEST_CASE( "154" )
{
	const auto doc = load_test( 154 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div id=\"foo\" class=\"bar\n  baz\">\n</div>" ) );
	}
}

TEST_CASE( "155" )
{
	const auto doc = load_test( 155 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div>\n*foo*" ) );
	}

	REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
	auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
	REQUIRE( p->items().size() == 1 );

	REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
	auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
	REQUIRE( t->opts() == MD::ItalicText );
	REQUIRE( t->text() == QStringLiteral( "bar" ) );
}

TEST_CASE( "156" )
{
	const auto doc = load_test( 156 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div id=\"foo\"\n*hi*" ) );
	}
}

TEST_CASE( "157" )
{
	const auto doc = load_test( 157 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div class\nfoo" ) );
	}
}

TEST_CASE( "158" )
{
	const auto doc = load_test( 158 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div *?\?\?-&&&-<---\n*foo*" ) );
	}
}

TEST_CASE( "159" )
{
	const auto doc = load_test( 159 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div><a href=\"bar\">*foo*</a></div>" ) );
	}
}

TEST_CASE( "160" )
{
	const auto doc = load_test( 160 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<table><tr><td>\nfoo\n</td></tr></table>" ) );
	}
}

TEST_CASE( "161" )
{
	const auto doc = load_test( 161 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<div></div>\n``` c\nint x = 33;\n```" ) );
	}
}

TEST_CASE( "162" )
{
	const auto doc = load_test( 162 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<a href=\"foo\">\n*bar*\n</a>" ) );
}

TEST_CASE( "163" )
{
	const auto doc = load_test( 163 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<Warning>\n*bar*\n</Warning>" ) );
}

TEST_CASE( "164" )
{
	const auto doc = load_test( 164 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<i class=\"foo\">\n*bar*\n</i>" ) );
}

TEST_CASE( "165" )
{
	const auto doc = load_test( 165 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "</ins>\n*bar*" ) );
}

TEST_CASE( "166" )
{
	const auto doc = load_test( 166 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<del>\n*foo*\n</del>" ) );
}

TEST_CASE( "167" )
{
	const auto doc = load_test( 167 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 4 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<del>" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
		REQUIRE( p->items().size() == 1 );

		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::ItalicText );
		REQUIRE( t->text() == QStringLiteral( "foo" ) );
	}

	{
		REQUIRE( doc->items().at( 3 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 3 ).data() );
		REQUIRE( h->text() == QStringLiteral( "</del>" ) );
	}
}

TEST_CASE( "168" )
{
	const auto doc = load_test( 168 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 1 ).data() );
		REQUIRE( p->items().size() == 3 );

		{
			REQUIRE( p->items().at( 0 )->type() == MD::ItemType::RawHtml );
			auto h = static_cast< MD::RawHtml* > ( p->items().at( 0 ).data() );
			REQUIRE( h->text() == QStringLiteral( "<del>" ) );
		}

		REQUIRE( p->items().at( 1 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 1 ).data() );
		REQUIRE( t->opts() == MD::ItalicText );
		REQUIRE( t->text() == QStringLiteral( "foo" ) );

		{
			REQUIRE( p->items().at( 2 )->type() == MD::ItemType::RawHtml );
			auto h = static_cast< MD::RawHtml* > ( p->items().at( 2 ).data() );
			REQUIRE( h->text() == QStringLiteral( "</del>" ) );
		}
	}
}

TEST_CASE( "169" )
{
	const auto doc = load_test( 169 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<pre language=\"haskell\"><code>\n"
			"import Text.HTML.TagSoup\n\n"
			"main :: IO ()\n"
			"main = print $ parseTags tags\n"
			"</code></pre>" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
		REQUIRE( p->items().size() == 1 );

		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "okay" ) );
	}
}

TEST_CASE( "170" )
{
	const auto doc = load_test( 170 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<script type=\"text/javascript\">\n"
			"// JavaScript example\n\n"
			"document.getElementById(\"demo\").innerHTML = \"Hello JavaScript!\";\n"
			"</script>" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
		REQUIRE( p->items().size() == 1 );

		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "okay" ) );
	}
}

TEST_CASE( "171" )
{
	const auto doc = load_test( 171 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<textarea>\n\n"
										  "*foo*\n\n"
										  "_bar_\n\n"
										  "</textarea>" ) );
}

TEST_CASE( "172" )
{
	const auto doc = load_test( 172 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 3 );

	{
		REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
		auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
		REQUIRE( h->text() == QStringLiteral( "<style\n"
											"  type=\"text/css\">\n"
											"h1 {color:red;}\n\n"
											"p {color:blue;}\n"
											"</style>" ) );
	}

	{
		REQUIRE( doc->items().at( 2 )->type() == MD::ItemType::Paragraph );
		auto p = static_cast< MD::Paragraph* > ( doc->items().at( 2 ).data() );
		REQUIRE( p->items().size() == 1 );

		REQUIRE( p->items().at( 0 )->type() == MD::ItemType::Text );
		auto t = static_cast< MD::Text* > ( p->items().at( 0 ).data() );
		REQUIRE( t->opts() == MD::TextWithoutFormat );
		REQUIRE( t->text() == QStringLiteral( "okay" ) );
	}
}

TEST_CASE( "173" )
{
	const auto doc = load_test( 173 );

	REQUIRE( doc->isEmpty() == false );
	REQUIRE( doc->items().size() == 2 );

	REQUIRE( doc->items().at( 1 )->type() == MD::ItemType::RawHtml );
	auto h = static_cast< MD::RawHtml* > ( doc->items().at( 1 ).data() );
	REQUIRE( h->text() == QStringLiteral( "<style\n"
										"  type=\"text/css\">\n\n"
										"foo" ) );
}