
/*!
	\file

	\author Igor Mironchik (igor.mironchik at gmail dot com).

	Copyright (c) 2019 Igor Mironchik

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

// md-pdf include
#include "md_parser.hpp"

// C++ include.
#include <tuple>
#include <algorithm>
#include <stack>

// Qt include.
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QRegularExpression>


namespace MD {

// Skip spaces in line from pos \a i.
qsizetype
skipSpaces( qsizetype i, QStringView line )
{
	const auto length = line.length();

	while( i < length && line[ i ].isSpace() )
		++i;

	return i;
}; // skipSpaces

bool
isFootnote( const QString & s )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	if( s.size() - p < 5 )
		return false;

	if( s[ p++ ] != c_91 )
		return false;

	if( s[ p++ ] != c_94 )
		return false;

	if( s[ p ] == c_93 || s[ p ].isSpace() )
		return false;

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] == c_93 )
			break;
		else if( s[ p ].isSpace() )
			return false;
	}

	++p;

	if( p < s.size() && s[ p ] == c_58 )
		return true;
	else
		return false;
}

QString
startSequence( const QString & line )
{
	auto pos = skipSpaces( 0, line );

	const auto sch = ( pos < line.length() ? line[ pos ] : QChar() );

	QString s = sch;

	++pos;

	while( pos < line.length() )
	{
		if( line[ pos ] == sch )
			s.append( sch );
		else
			break;

		++pos;
	}

	return s;
}

bool
isCodeFences( const QString & s, bool closing )
{
	auto p = skipSpaces( 0, s );

	if( p > 3 )
		return false;

	const auto ch = s[ p ];

	if( ch != c_126 && ch != c_96 )
		return false;

	bool space = false;

	qsizetype c = 1;
	++p;

	for( ; p < s.length(); ++p )
	{
		if( s[ p ].isSpace() )
			space = true;
		else if( s[ p ] == ch )
		{
			if( space && ( closing ? true : ch == c_96 ) )
				return false;

			if( !space )
				++c;
		}
		else if( closing )
			return false;
		else
			break;
	}

	if( c < 3 )
		return false;

	if( ch == c_96 )
	{
		for( ; p < s.length(); ++p )
		{
			if( s[ p ] == c_96 )
				return false;
		}
	}

	return true;
}

bool
isOrderedList( const QString & s, int * num, int * len,
	QChar * delim, bool * isFirstLineEmpty )
{
	qsizetype p = skipSpaces( 0, s );

	qsizetype dp = p;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isDigit() )
			break;
	}

	if( dp != p && p < s.size() )
	{
		const auto digits = s.mid( dp, p - dp );

		if( digits.size() > 9 )
			return false;

		const auto i = digits.toInt();

		if( num )
			*num = i;

		if( len )
			*len = p - dp;

		if( s[ p ] == c_46 || s[ p ] == c_41 )
		{
			if( delim )
				*delim = s[ p ];

			++p;

			qsizetype tmp = skipSpaces( p, s );

			if( isFirstLineEmpty )
				*isFirstLineEmpty = ( tmp == s.size() );

			if( ( p < s.size() && s[ p ] == c_32 ) || p == s.size() )
				return true;
		}
	}

	return false;
}

inline void
collapseStack( std::stack< std::pair< qsizetype, int > > & st, qsizetype v, int type,
	qsizetype idx )
{
	while( !st.empty() && v > 0 )
	{
		if( st.top().second != type )
			break;

		v -= st.top().first;
		st.pop();

		if( v > 0 )
		{
			while( !st.empty() && st.size() > idx + 1 && st.top().second != type )
				st.pop();
		}
	}
}

std::pair< bool, size_t >
checkEmphasisSequence( const std::vector< std::pair< qsizetype, int > > & s, size_t idx )
{
	std::stack< std::pair< qsizetype, int > > st;

	for( size_t i = 0; i <= idx; ++i )
		st.push( s[ i ] );

	size_t ret = 0;
	bool retInit = false;
	qsizetype ii = idx;

	for( size_t i = idx + 1; i < s.size(); ++i )
	{
		if( s[ i ].first > 0 )
			st.push( s[ i ] );
		else
		{
			if( !st.empty() && st.top().second == s[ i ].second )
			{
				auto v = qAbs( s[ i ].first );

				if( st.top().first <= v )
					collapseStack( st, v, s[ i ].second, ii );
				else
					st.top().first += s[ i ].first;
			}
			else
			{
				while( !st.empty() && st.size() > ii + 1 && st.top().second != s[ i ].second )
					st.pop();

				if( !st.empty() && st.top().second == s[ i ].second )
				{
					auto v = qAbs( s[ i ].first );

					if( st.top().first <= v )
						collapseStack( st, v, s[ i ].second, ii );
					else
						st.top().first += s[ i ].first;
				}
			}
		}

		if( st.size() < ii + 1 )
			ii = st.size() - 1;

		if( st.size() <= idx && !retInit )
		{
			ret = i;
			retInit = true;
		}
	}

	return { st.empty(), ret };
}

namespace /* anonymous */ {

inline QString
readEscapedSequence( qsizetype i, QStringView str )
{
	QString ret;
	bool backslash = false;

	while( i < str.length() )
	{
		bool now = false;

		if( str[ i ] == c_92 && !backslash )
		{
			backslash = true;
			now = true;
		}
		else if( str[ i ].isSpace() && !backslash )
			break;
		else
			ret.append( str[ i ] );

		if( !now )
			backslash = false;

		++i;
	}

	return ret;
}

}

bool
isStartOfCode( QStringView str, QString * syntax )
{
	if( str.size() < 3 )
		return false;

	const bool c96 = str[ 0 ] == c_96;
	const bool c126 = str[ 0 ] == c_126;

	if( c96 || c126 )
	{
		qsizetype p = 1;
		qsizetype c = 1;

		while( p < str.length() )
		{
			if( str[ p ] != ( c96 ? c_96 : c_126 ) )
				break;

			++c;
			++p;
		}

		if( c < 3 )
			return false;

		if( syntax )
		{
			p = skipSpaces( p, str );

			if( p < str.size() )
				*syntax = readEscapedSequence( p, str );
		}

		return true;
	}

	return false;
}

bool
isHorizontalLine( QStringView s )
{

	if( s.size() < 3 )
		return false;

	QChar c;

	if( s[ 0 ] == c_42 )
		c = c_42;
	else if( s[ 0 ] == c_45 )
		c = c_45;
	else if( s[ 0 ] == c_95 )
		c = c_95;
	else
		return false;

	qsizetype p = 1;
	qsizetype count = 1;

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] != c && !s[ p ].isSpace() )
			break;
		else if( s[ p ] == c )
			++count;
	}

	if( count < 3 )
		return false;

	if( p == s.size() )
		return true;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			return false;
	}

	return true;
}

bool
isColumnAlignment( const QString & s )
{
	qsizetype p = skipSpaces( 0, s );

	if( p == s.size() )
		return true;

	static const auto c_legitime = QStringLiteral( ":-" );

	if( !c_legitime.contains( s[ p ] ) )
		return false;

	if( s[ p ] == c_58 )
		++p;

	const auto a = p;

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] != c_45 )
			break;
	}

	if( a != p && p - a < 3 )
		return false;

	if( p == s.size() )
		return true;

	if( s[ p ] != c_58 && !s[ p ].isSpace() )
		return false;

	++p;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			return false;
	}

	return true;
}

bool
isTableAlignment( const QString & s )
{
	const auto columns = s.split( c_124, Qt::SkipEmptyParts );

	for( const auto & c : columns )
	{
		if( !isColumnAlignment( c ) )
			return false;
	}

	return true;
}


//
// Parser
//

QSharedPointer< Document >
Parser::parse( const QString & fileName, bool recursive )
{
	QSharedPointer< Document > doc( new Document );

	parseFile( fileName, recursive, doc );

	clearCache();

	return doc;
}

void
Parser::parseFile( const QString & fileName, bool recursive, QSharedPointer< Document > doc,
	QStringList * parentLinks )
{
	QFileInfo fi( fileName );

	if( fi.exists() && ( fi.suffix().toLower() == QLatin1String( "md" ) ||
		fi.suffix().toLower() == QLatin1String( "markdown" ) ) )
	{
		QFile f( fileName );

		if( f.open( QIODevice::ReadOnly ) )
		{
			QStringList linksToParse;

			QTextStream s( &f );

			doc->appendItem( QSharedPointer< Anchor > ( new Anchor( fi.absoluteFilePath() ) ) );

			TextStream stream( s );

			parse( stream, doc, doc, linksToParse,
				fi.absolutePath() + QStringLiteral( "/" ), fi.fileName(), true, true );

			f.close();

			m_parsedFiles.append( fi.absoluteFilePath() );

			// Resolve links.
			for( auto it = linksToParse.begin(), last = linksToParse.end(); it != last; ++it )
			{
				auto nextFileName = *it;

				if( nextFileName.startsWith( c_35 ) )
				{
					if( doc->labeledLinks().contains( nextFileName ) )
						nextFileName = doc->labeledLinks()[ nextFileName ]->url();
					else
						continue;
				}

				QFileInfo nextFile( nextFileName );

				*it = nextFile.absoluteFilePath();
			}

			// Parse all links if parsing is recursive.
			if( recursive && !linksToParse.isEmpty() )
			{
				const auto tmpLinks = linksToParse;

				while( !linksToParse.isEmpty() )
				{
					auto nextFileName = linksToParse.first();
					linksToParse.removeFirst();

					if( parentLinks )
					{
						if( parentLinks->contains( nextFileName ) )
							continue;
					}

					if( nextFileName.startsWith( c_35 ) )
						continue;

					if( !m_parsedFiles.contains( nextFileName ) )
					{
						if( !doc->isEmpty() && doc->items().last()->type() != ItemType::PageBreak )
							doc->appendItem( QSharedPointer< PageBreak > ( new PageBreak() ) );

						parseFile( nextFileName, recursive, doc, &linksToParse );
					}
				}

				if( parentLinks )
					parentLinks->append( tmpLinks );
			}
		}
	}
}

namespace /* anonymous */ {

inline qsizetype
posOfListItem( const QString & s, bool ordered )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	if( ordered )
	{
		for( ; p < s.size(); ++p )
		{
			if( !s[ p ].isDigit() )
				break;
		}
	}

	++p;

	qsizetype sc = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
		else
			++sc;
	}

	if( sc > 4 )
		p = p - sc + 1;

	return p;
}

} /* namespace anonymous */

Parser::BlockType
Parser::whatIsTheLine( QString & str, bool inList, qsizetype * indent, bool calcIndent,
	const std::set< qsizetype > * indents ) const
{
	str.replace( c_9, QString( 4, c_32 ) );

	const auto first = skipSpaces( 0, str );

	if( first < str.length() )
	{
		auto s = QStringView( str ).sliced( first );

		if( s.startsWith( c_62 ) && first < 4 )
			return BlockType::Blockquote;
		else if( s.startsWith( c_35 ) && first < 4 )
		{
			qsizetype c = 0;

			while( c < s.length() && s[ c ] == c_35 )
				++c;

			if( c <= 6 && ( ( c < s.length() && s[ c ].isSpace() ) || c == s.length() ) )
				return BlockType::Heading;
			else
				return BlockType::Text;
		}

		if( first < 4 && isHorizontalLine( s ) )
			return BlockType::Text;

		if( inList )
		{
			bool isFirstLineEmpty = false;
			const auto orderedList = isOrderedList( str, nullptr, nullptr, nullptr,
				&isFirstLineEmpty );

			if( ( ( ( s.startsWith( c_45 ) || s.startsWith( c_43 ) || s.startsWith( c_42 ) ) &&
				( ( s.length() > 1 && s[ 1 ] == c_32 ) || s.length() == 1 ) ) ||
				orderedList ) && ( first < 4  || indentInList( indents, first ) ) )
			{
				if( calcIndent && indent )
					*indent = posOfListItem( str, orderedList );

				if( s.length() == 1 || isFirstLineEmpty )
					return BlockType::ListWithFirstEmptyLine;
				else
					return BlockType::List;
			}
			else if( str.startsWith( QString( ( indent ? *indent : 4 ), c_32 ) ) )
			{
				if( str.startsWith( QStringLiteral( "    " ) ) )
					return BlockType::CodeIndentedBySpaces;
			}
		}
		else
		{
			bool isFirstLineEmpty = false;

			const auto orderedList = isOrderedList( str, nullptr, nullptr, nullptr,
				&isFirstLineEmpty );

			if( ( ( ( s.startsWith( c_45 ) || s.startsWith( c_43 ) || s.startsWith( c_42 ) ) &&
				( ( s.length() > 1 && s[ 1 ] == c_32 ) || s.length() == 1 ) ) ||
				orderedList ) && ( first < 4  || indentInList( indents, first ) ) )
			{
				if( calcIndent && indent )
					*indent = posOfListItem( str, orderedList );

				if( s.length() == 1 || isFirstLineEmpty )
					return BlockType::ListWithFirstEmptyLine;
				else
					return BlockType::List;
			}
			else if( str.startsWith( QLatin1String( "    " ) ) ||
				str.startsWith( c_9 ) )
			{
				return BlockType::CodeIndentedBySpaces;
			}
		}

		if( s.startsWith( QLatin1String( "```" ) ) ||
			s.startsWith( QLatin1String( "~~~" ) ) )
		{
			if( isCodeFences( str ) )
				return BlockType::Code;
			else
				return BlockType::Text;
		}
	}
	else
		return BlockType::Unknown;

	return BlockType::Text;
}

void
Parser::parseFragment( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName, bool collectRefLinks,
	RawHtmlBlock & html )
{
	switch( whatIsTheLine( fr.first() ) )
	{
		case BlockType::Text :
			parseText( fr, parent, doc, linksToParse,
				workingPath, fileName, collectRefLinks, html );
			break;

		case BlockType::Blockquote :
			parseBlockquote( fr, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks, html );
			break;

		case BlockType::Code :
			parseCode( fr, parent, collectRefLinks );
			break;

		case BlockType::CodeIndentedBySpaces :
		{
			int indent = 1;

			if( fr.first().startsWith( QLatin1String( "    " ) ) )
				indent = 4;

			parseCodeIndentedBySpaces( fr, parent, collectRefLinks, indent );
		}
			break;

		case BlockType::Heading :
			parseHeading( fr, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks );
			break;

		case BlockType::List :
		case BlockType::ListWithFirstEmptyLine :
			parseList( fr, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks, html );
			break;

		default :
			break;
	}
}

void
Parser::clearCache()
{
	m_parsedFiles.clear();
}

namespace /* anonymous */ {

inline bool
isTableHeader( const QString & s )
{
	if( s.contains( c_124 ) )
		return true;
	else
		return false;
}

} /* namespace anonymous */

void
Parser::parseText( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks, RawHtmlBlock & html )
{
	if( isFootnote( fr.first() ) )
		parseFootnote( fr, parent, doc, linksToParse, workingPath, fileName,
			collectRefLinks );
	else if( isTableHeader( fr.first() ) && fr.size() > 1 && isTableAlignment( fr[ 1 ] ) )
		parseTable( fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks );
	else
		parseParagraph( fr, parent, doc, linksToParse,
			workingPath, fileName, collectRefLinks, html );
}

namespace /* anonymous */ {

inline QString
findAndRemoveHeaderLabel( QString & s )
{
	const auto start = s.indexOf( QStringLiteral( "{#" ) );

	if( start >= 0 )
	{
		qsizetype p = start + 2;

		for( ; p < s.size(); ++p )
		{
			if( s[ p ] == c_125 )
				break;
		}

		if( p < s.size() && s[ p ] == c_125 )
		{
			const auto label = s.mid( start, p - start + 1 );
			s.remove( start, p - start + 1 );
			return label;
		}
	}

	return QString();
}

inline QString
paragraphToLabel( Paragraph * p )
{
	QString l;

	if( !p )
		return l;

	for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == ItemType::Text )
		{
			if( !l.isEmpty() )
				l.append( QStringLiteral( "-" ) );

			auto t = static_cast< Text* > ( it->data() );

			for( const auto & c : t->text().simplified() )
			{
				if( c.isLetter() || c.isDigit() )
					l.append( c.toLower() );
				else if( c.isSpace() )
					l.append( QStringLiteral( "-" ) );
			}
		}
	}

	return l;
}

inline void
findAndRemoveClosingSequence( QString & s )
{
	qsizetype end = -1;
	qsizetype start = -1;

	for( qsizetype i = s.length() - 1; i >= 0 ; --i )
	{
		if( !s[ i ].isSpace() && s[ i ] != c_35 && end == -1 )
			return;

		if( s[ i ] == c_35 )
		{
			if( end == -1 )
				end = i;

			if( i - 1 >= 0 )
			{
				if( s[ i - 1 ].isSpace() )
				{
					start = i;
					break;
				}
				else if( s[ i - 1 ] != c_35 )
					return;
			}
			else
				start = 0;
		}
	}

	if( start != -1 && end != -1 )
		s.remove( start, end - start + 1 );
}

} /* namespace anonymous */

void
Parser::parseHeading( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	if( !fr.isEmpty() && !collectRefLinks )
	{
		auto line = fr.first();
		qsizetype pos = 0;
		pos = skipSpaces( pos, line );

		if( pos > 0  )
			line = line.sliced( pos );

		pos = 0;
		int lvl = 0;

		while( pos < line.length() && line[ pos ] == c_35 )
		{
			++lvl;
			++pos;
		}

		pos = skipSpaces( pos, line );

		if( pos > 0 )
			fr.first() = line.sliced( pos );

		const auto label = findAndRemoveHeaderLabel( fr.first() );

		findAndRemoveClosingSequence( fr.first() );

		QSharedPointer< Heading > h( new Heading() );
		h->setLevel( lvl );

		if( !label.isEmpty() )
			h->setLabel( label.mid( 1, label.length() - 2 ) + QStringLiteral( "/" ) +
				workingPath + fileName );

		QSharedPointer< Paragraph > p( new Paragraph );

		QStringList tmp;
		tmp << fr.first().simplified();

		RawHtmlBlock html;

		parseFormattedTextLinksImages( tmp, p, doc, linksToParse, workingPath, fileName,
			false, false, html );

		fr.removeFirst();

		if( p->items().size() && p->items().at( 0 )->type() == ItemType::Paragraph )
			h->setText( p->items().at( 0 ).staticCast< Paragraph > () );
		else
			h->setText( p );

		if( h->isLabeled() )
			doc->insertLabeledHeading( h->label(), h );
		else
		{
			QString label = QStringLiteral( "#" ) + paragraphToLabel( h->text().data() );

			label += QStringLiteral( "/" ) + workingPath + fileName;

			h->setLabel( label );

			doc->insertLabeledHeading( label, h );
		}

		parent->appendItem( h );
	}
}

void
Parser::parseTable( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	static const QChar sep( '|' );

	if( fr.size() >= 2 )
	{
		QSharedPointer< Table > table( new Table );

		auto parseTableRow = [&] ( const QString & row )
		{
			auto line = row.simplified();

			if( line.startsWith( sep ) )
				line.remove( 0, 1 );

			if( line.endsWith( sep ) )
				line.remove( line.length() - 1, 1 );

			auto columns = line.split( sep );

			QSharedPointer< TableRow > tr( new TableRow );

			for( auto it = columns.begin(), last = columns.end(); it != last; ++it )
			{
				QSharedPointer< TableCell > c( new TableCell );

				if( !it->isEmpty() )
				{
					it->replace( QLatin1String( "&#124;" ), sep );

					QStringList fragment;
					fragment.append( *it );

					QSharedPointer< Paragraph > p( new Paragraph );

					RawHtmlBlock html;

					parseFormattedTextLinksImages( fragment, p, doc,
						linksToParse, workingPath, fileName, collectRefLinks, false, html );

					if( !p->isEmpty() && p->items().at( 0 )->type() == ItemType::Paragraph )
					{
						const auto pp = p->items().at( 0 ).staticCast< Paragraph > ();

						for( auto it = pp->items().cbegin(), last = pp->items().cend();
							it != last; ++it )
						{
							c->appendItem( (*it) );
						}
					}
				}

				tr->appendCell( c );
			}

			if( !tr->isEmpty() )
				table->appendRow( tr );
		};

		{
			auto fmt = fr.at( 1 );

			auto columns = fmt.split( sep, Qt::SkipEmptyParts );

			for( auto it = columns.begin(), last = columns.end(); it != last; ++it )
			{
				*it = it->simplified();

				if( !it->isEmpty() )
				{
					Table::Alignment a = Table::AlignLeft;

					if( it->endsWith( c_58 ) && it->startsWith( c_58 ) )
						a = Table::AlignCenter;
					else if( it->endsWith( c_58 ) )
						a = Table::AlignRight;

					table->setColumnAlignment( table->columnsCount(), a );
				}
			}
		}

		fr.removeAt( 1 );

		for( const auto & line : qAsConst( fr ) )
			parseTableRow( line );

		if( !table->isEmpty() && !collectRefLinks )
			parent->appendItem( table );
	}
}

namespace /* anonymous */ {

inline bool
isH( const QString & s, const QChar & c )
{
	qsizetype p = skipSpaces( 0, s );

	if( p > 3 )
		return false;

	const auto start = p;

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] != c )
			break;
	}

	if( p - start < 1 )
		return false;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			return false;
	}

	return true;
}

inline bool
isH1( const QString & s )
{
	return isH( s, c_61 );
}

inline bool
isH2( const QString & s )
{
	return isH( s, c_45 );
}

} /* namespace anonymous */

void
Parser::parseParagraph( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks, RawHtmlBlock & html )
{
	bool heading = false;

	// Check for alternative syntax of H1 and H2 headings.
	if( fr.size() >= 2 )
	{
		qsizetype i = 1;
		int lvl = 0;
		qsizetype horLines = 0;

		for( ; i < fr.size(); ++i )
		{
			const auto first = skipSpaces( 0, fr.at( i - 1 ) );

			auto s = QStringView( fr.at( i - 1 ) ).sliced( first );

			const bool prevHorLine = ( first < 4 && isHorizontalLine( s ) );

			if( prevHorLine )
				++horLines;

			if( isH1( fr.at( i ) ) && !prevHorLine && !fr.at( i - 1 ).simplified().isEmpty() )
			{
				lvl = 1;
				heading = true;
				break;
			}
			else if( isH2( fr.at( i ) ) && !prevHorLine && !fr.at( i - 1 ).simplified().isEmpty() )
			{
				lvl = 2;
				heading = true;
				break;
			}
		}

		if( heading )
		{
			if( !collectRefLinks )
				for( qsizetype j = 0; j < horLines; ++j )
					parent->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

			fr.remove( 0, horLines );

			QSharedPointer< Heading > h( new Heading );
			QSharedPointer< Paragraph > p( new Paragraph );

			h->setLevel( lvl );

			QStringList tmp = fr.sliced( 0, i - horLines );

			const auto ns1 = skipSpaces( 0, tmp.first() );

			if( ns1 > 0 && ns1 < tmp.first().length() )
				tmp.first() = tmp.first().sliced( ns1 );

			qsizetype ns2 = tmp.back().length();

			for( qsizetype i = tmp.back().length() - 1; i >= 0; --i )
			{
				if( tmp.back()[ i ].isSpace() )
					ns2 = i;
				else
					break;
			}

			if( ns2 < tmp.back().length() )
				tmp.back() = tmp.back().sliced( 0, ns2 );

			parseFormattedTextLinksImages( tmp, p, doc, linksToParse,
				workingPath, fileName, collectRefLinks, true, html );

			const bool keepHeadingLine = p->isEmpty();

			fr.remove( 0, i - horLines + ( keepHeadingLine ? 0 : 1 ) );

			if( !collectRefLinks && !keepHeadingLine )
			{
				if( p->items().at( 0 )->type() == ItemType::Paragraph )
					h->setText( p->items().at( 0 ).staticCast< Paragraph > () );

				QString label = QStringLiteral( "#" ) + paragraphToLabel( h->text().data() );

				label += QStringLiteral( "/" ) + workingPath + fileName;

				h->setLabel( label );

				doc->insertLabeledHeading( label, h );

				parent->appendItem( h );
			}
		}
	}

	if( !fr.isEmpty() )
	{
		if( heading )
		{
			StringListStream stream( fr );
			parse( stream, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks );
		}
		else
		{
			QSharedPointer< Paragraph > p( new Paragraph );

			parseFormattedTextLinksImages( fr, p, doc, linksToParse, workingPath, fileName,
				collectRefLinks, false, html );

			if( !p->isEmpty() && !collectRefLinks )
			{
				for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
				{
					if( (*it)->type() == MD::ItemType::Paragraph )
					{
						auto p = static_cast< MD::Paragraph* > ( (*it).data() );

						QSharedPointer< Paragraph > pp( new Paragraph );

						for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
						{
							if( (*it)->type() == MD::ItemType::RawHtml &&
								(*it).staticCast< MD::RawHtml > ()->isFreeTag() )
							{
								if( !pp->isEmpty() )
								{
									parent->appendItem( pp );
									pp.reset( new Paragraph );
								}

								parent->appendItem( (*it) );
							}
							else
								pp->appendItem( (*it) );
						}

						if( !pp->isEmpty() )
							parent->appendItem( pp );
					}
					else
						parent->appendItem( (*it) );
				}
			}
		}
	}
}

struct UnprotectedRawHtmlMethods {
	static void setFreeTag( QSharedPointer< RawHtml > html, bool on )
	{
		html->setFreeTag( on );
	}
};

namespace /* anoymous*/ {

struct Delimiter {
	enum DelimiterType {
		// (
		ParenthesesOpen,
		// )
		ParenthesesClose,
		// [
		SquareBracketsOpen,
		// ]
		SquareBracketsClose,
		// ![
		ImageOpen,
		// ~~
		Strikethrough,
		// *
		Emphasis1,
		// _
		Emphasis2,
		// `
		InlineCode,
		// <
		Less,
		// >
		Greater,
		HorizontalLine,
		Unknown
	}; // enum DelimiterType

	DelimiterType m_type = Unknown;
	qsizetype m_line = -1;
	qsizetype m_pos = -1;
	qsizetype m_len = 0;
	bool m_spaceBefore = false;
	bool m_spaceAfter = false;
	bool m_isWordBefore = false;
	bool m_backslashed = false;
	bool m_leftFlanking = false;
	bool m_rightFlanking = false;
}; // struct Delimiter

enum class Style {
	Italic1,
	Italic2,
	Bold1,
	Bold2,
	Strikethrough,
	Unknown
};

using Delims = QList< Delimiter >;

inline Delims
collectDelimiters( const QStringList & fr )
{
	Delims d;

	for( qsizetype line = 0; line < fr.size(); ++line )
	{
		const QString & str = fr.at( line );
		const auto p = skipSpaces( 0, str );
		const auto withoutSpaces = str.sliced( p );

		if( isHorizontalLine( withoutSpaces ) && p < 4 )
			d.push_back( { Delimiter::HorizontalLine, line, 0, str.length(), false, false, false } );
		else
		{
			bool backslash = false;
			bool space = true;
			bool word = false;

			for( qsizetype i = 0; i < str.size(); ++i )
			{
				bool now = false;

				if( str[ i ] == c_92 && !backslash )
				{
					backslash = true;
					now = true;
				}
				else if( str[ i ].isSpace() && !backslash )
				{
					space = true;
					now = true;
				}
				else
				{
					// * or _
					if( ( str[ i ] == c_95 || str[ i ] == c_42 ) && !backslash )
					{
						QString style;

						const bool punctBefore = ( i > 0 ? str[ i - 1 ].isPunct() ||
							str[ i - 1 ] == c_126 : false );
						const bool alNumBefore =
							( i > 0 ? str[ i - 1 ].isLetterOrNumber() : false );

						const auto ch = str[ i ];

						while( i < str.length() && str[ i ] == ch )
						{
							style.append( str[ i ] );
							++i;
						}

						Delimiter::DelimiterType dt = Delimiter::Unknown;

						if( ch == c_42 )
							dt = Delimiter::Emphasis1;
						else
							dt = Delimiter::Emphasis2;

						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );
						const bool punctAfter =
							( i < str.length() ? str[ i ].isPunct() ||
								str[ i ] == c_126 : false );
						const bool leftFlanking =
							( ( space || punctBefore ) && punctAfter ) ||
							( !spaceAfter && !punctAfter );
						const bool rightFlanking =
							( punctBefore && ( spaceAfter || punctAfter ) ) ||
							( !space && !punctBefore );
						const bool disabledEmphasis =
							( ( i < str.length() ? str[ i ].isLetterOrNumber() : false ) &&
							alNumBefore && ch == c_95 );

						if( ( leftFlanking || rightFlanking ) && !disabledEmphasis )
						{
							for( auto j = 0; j < style.length(); ++j )
							{
								d.push_back( { dt, line, i - style.length() + j,
									1, space, spaceAfter, word, false,
									leftFlanking, rightFlanking } );
							}

							word = false;
						}
						else
							word = true;

						--i;
					}
					// ~
					else if( str[ i ] == c_126 && !backslash )
					{
						QString style;

						const bool punctBefore = ( i > 0 ? str[ i - 1 ].isPunct() ||
							str[ i - 1 ] == c_126 : false );

						while( i < str.length() && str[ i ] == c_126 )
						{
							style.append( str[ i ] );
							++i;
						}

						if( style.length() == 2 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ].isSpace() : true );
							const bool punctAfter =
								( i < str.length() ? str[ i ].isPunct() ||
									str[ i ] == c_126 : false );
							const bool leftFlanking =
								( ( space || punctBefore ) && punctAfter ) ||
								( !spaceAfter && !punctAfter );
							const bool rightFlanking =
								( punctBefore && ( spaceAfter || punctAfter ) ) ||
								( !space && !punctBefore );

							if( leftFlanking || rightFlanking )
							{
								d.push_back( { Delimiter::Strikethrough, line, i - style.length(),
									style.length(), space, spaceAfter, word, false,
									leftFlanking, rightFlanking } );

								word = false;
							}
							else
								word = true;
						}
						else
							word = true;

						--i;
					}
					// [
					else if( str[ i ] == c_91 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::SquareBracketsOpen, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// !
					else if( str[ i ] == c_33 && !backslash )
					{
						if( i + 1 < str.length() )
						{
							if( str[ i + 1 ] == c_91 )
							{
								const bool spaceAfter =
									( i < str.length() ? str[ i ].isSpace() : true );

								d.push_back( { Delimiter::ImageOpen, line, i, 2,
									space, spaceAfter, word, false } );

								++i;

								word = false;
							}
							else
								word = true;
						}
						else
							word = true;
					}
					// (
					else if( str[ i ] == c_40 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::ParenthesesOpen, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// ]
					else if( str[ i ] == c_93 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::SquareBracketsClose, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// )
					else if( str[ i ] == c_41 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::ParenthesesClose, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// <
					else if( str[ i ] == c_60 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::Less, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// >
					else if( str[ i ] == c_62 && !backslash )
					{
						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::Greater, line, i, 1,
							space, spaceAfter, word, false } );

						word = false;
					}
					// `
					else if( str[ i ] == c_96 )
					{
						QString code;

						while( i < str.length() && str[ i ] == c_96 )
						{
							code.append( str[ i ] );
							++i;
						}

						if( backslash )
						{
							if( i - code.length() - 2 >= 0 )
							{
								if( str[ i - code.length() - 2 ].isSpace() )
									space = true;
							}
							else
								space = true;
						}

						const bool spaceAfter =
							( i < str.length() ? str[ i ].isSpace() : true );

						d.push_back( { Delimiter::InlineCode,
							line, i - code.length() - ( backslash ? 1 : 0 ),
							code.length() + ( backslash ? 1 : 0 ),
							space, spaceAfter, word, backslash } );

						word = false;

						--i;
					}
					else
						word = true;
				}

				if( !now )
				{
					backslash = false;
					space = false;
				}
			}
		}
	}

	return d;
}

struct TextParsingOpts {
	QStringList & fr;
	QSharedPointer< Block > parent;
	QSharedPointer< Document > doc;
	QStringList & linksToParse;
	QString workingPath;
	QString fileName;
	bool collectRefLinks;
	bool ignoreLineBreak;
	RawHtmlBlock & html;

	bool wasRefLink = false;
	qsizetype line = 0;
	qsizetype pos = 0;
	TextOptions opts = TextWithoutFormat;
	std::vector< std::pair< Style, qsizetype > > styles = {};
}; // struct TextParsingOpts

void
parseFormattedText( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak,
	RawHtmlBlock & html );

inline bool
isLineBreak( const QString & s )
{
	return ( s.endsWith( QStringLiteral( "  " ) ) || s.endsWith( c_92 ) );
}

inline QString
removeLineBreak( const QString & s )
{
	if( s.endsWith( c_92 ) )
		return s.sliced( 0, s.size() - 1 );
	else
		return s;
}

inline void
makeTextObject( const QString & text, bool spaceBefore, bool spaceAfter,
	TextParsingOpts & po )
{
	const auto s = text.simplified();

	if( !s.isEmpty() )
	{
		QSharedPointer< Text > t( new Text() );
		t->setText( s );
		t->setOpts( po.opts );
		t->setSpaceBefore( spaceBefore );
		t->setSpaceAfter( spaceAfter );

		po.wasRefLink = false;
		po.parent->appendItem( t );
	}
}

inline void
makeTextObjectWithLineBreak( const QString & text, bool spaceBefore, bool spaceAfter,
	TextParsingOpts & po )
{
	makeTextObject( text, spaceBefore, true, po );

	QSharedPointer< Item > hr( new LineBreak );
	po.wasRefLink = false;
	po.parent->appendItem( hr );
}

static const QString c_canBeEscaped = QStringLiteral( "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~" );

inline QString
removeBackslashes( const QString & s )
{
	QString r;
	bool backslash = false;

	for( qsizetype i = 0; i < s.size(); ++i )
	{
		bool now = false;

		if( s[ i ] == c_92 && !backslash && i != s.size() - 1 )
		{
			backslash = true;
			now = true;
		}
		else if( c_canBeEscaped.contains( s[ i ] ) && backslash )
			r.append( s[ i ] );
		else if( backslash )
		{
			r.append( c_92 );
			r.append( s[ i ] );
		}
		else
			r.append( s[ i ] );

		if( !now )
			backslash = false;
	}

	return r;
}

inline void
makeText(
	// Inclusive.
	qsizetype lastLine,
	// Not inclusive
	qsizetype lastPos,
	TextParsingOpts & po,
	bool doNotEscape = false )
{
	if( po.line > lastLine )
		return;
	else if( po.line == lastLine && po.pos >= lastPos )
		return;

	QString text;

	bool spaceBefore = ( po.pos > 0 && po.pos < po.fr.at( po.line ).size() ?
		po.fr.at( po.line )[ po.pos - 1 ].isSpace() ||
			po.fr.at( po.line )[ po.pos ].isSpace() :
		true );

	bool lineBreak = ( !po.ignoreLineBreak && po.line != po.fr.size() - 1 && ( po.line == lastLine ?
		( lastPos == po.fr.at( po.line ).size() && isLineBreak( po.fr.at( po.line ) ) ) :
		isLineBreak( po.fr.at( po.line ) ) ) );

	// makeTOWLB
	auto makeTOWLB = [&] () {
		if( po.line != po.fr.size() - 1 )
		{
			makeTextObjectWithLineBreak( text, spaceBefore, true, po );

			text.clear();

			spaceBefore = true;
		}
	}; // makeTOWLB

	if( lineBreak )
	{
		const auto s = removeLineBreak( po.fr.at( po.line ) ).sliced( po.pos );
		text.append( doNotEscape ? s : removeBackslashes( s ) );

		makeTOWLB();
	}
	else
	{
		const auto s = po.fr.at( po.line ).sliced( po.pos,
			( po.line == lastLine ? lastPos - po.pos : po.fr.at( po.line ).size() - po.pos ) );
		text.append( doNotEscape ? s : removeBackslashes( s ) );
	}

	if( po.line != lastLine )
	{
		text.append( c_32 );
		++po.line;

		for( ; po.line < lastLine; ++po.line )
		{
			lineBreak = ( !po.ignoreLineBreak && po.line != po.fr.size() - 1 &&
				isLineBreak( po.fr.at( po.line ) ) );

			const auto s = ( lineBreak ?
				removeLineBreak( po.fr.at( po.line ) ) : po.fr.at( po.line ) );
			text.append( doNotEscape ? s : removeBackslashes( s ) );

			text.append( c_32 );

			if( lineBreak )
				makeTOWLB();
		}

		lineBreak = ( !po.ignoreLineBreak && po.line != po.fr.size() - 1 &&
			lastPos == po.fr.at( po.line ).size() && isLineBreak( po.fr.at( po.line ) ) );

		auto s = po.fr.at( po.line ).sliced( 0, lastPos );

		if( !lineBreak )
			text.append( doNotEscape ? s : removeBackslashes( s ) );
		else
		{
			s = removeLineBreak( s );
			text.append( doNotEscape ? s : removeBackslashes( s ) );

			makeTOWLB();
		}
	}

	po.pos = lastPos;

	makeTextObject( text, spaceBefore,
		( po.pos > 0 ? po.fr.at( po.line )[ po.pos - 1 ].isSpace() : true ), po );
}

inline std::pair< QString, bool >
readHtmlTag( Delims::const_iterator it, TextParsingOpts & po )
{
	QString tag;

	qsizetype i = it->m_pos + 1;

	for( ; i < po.fr[ it->m_line ].size(); ++i )
	{
		const auto ch = po.fr[ it->m_line ][ i ];

		if( !ch.isSpace() && ch != c_62 )
			tag.append( ch );
		else
			break;
	}

	return { tag, i < po.fr[ it->m_line ].size() ? po.fr[ it->m_line ][ i ] == c_62 : false };
}

inline Delims::const_iterator
findIt( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	auto ret = it;

	for( ; it != last; ++it )
	{
		if( ( it->m_line == po.line && it->m_pos < po.pos ) || it->m_line < po.line )
			ret = it;
	}

	return ret;
}

inline void
eatRawHtml( qsizetype line, qsizetype pos, qsizetype toLine, qsizetype toPos,
	TextParsingOpts & po, bool finish, int htmlRule, bool skipLineEnds = false )
{
	QString h = po.html.html->text();

	const auto first = po.fr[ line ].sliced( pos,
		( line == toLine ? ( toPos >= 0 ? toPos - pos : po.fr[ line ].size() - pos ) :
			po.fr[ line ].size() - pos ) );

	if( !h.isEmpty() && !first.isEmpty() && !skipLineEnds )
		h.append( c_10 );

	if( !first.isEmpty() )
		h.append( first );

	++line;

	for( ; line < toLine; ++line )
	{
		h.append( c_10 );
		h.append( po.fr[ line ] );
	}

	if( line == toLine && toPos != 0 )
	{
		h.append( c_10 );
		h.append( po.fr[ line ].sliced( 0, toPos > 0 ? toPos : po.fr[ line ].size() ) );
	}

	po.line = ( toPos >= 0 ? toLine : toLine + 1 );
	po.pos = ( toPos >= 0 ? toPos : 0 );

	po.html.html->setText( h );
	UnprotectedRawHtmlMethods::setFreeTag( po.html.html, htmlRule != 7 );

	if( finish )
	{
		if( !po.collectRefLinks )
			po.parent->appendItem( po.html.html );

		po.html.html.reset( nullptr );
		po.html.htmlBlockType = -1;
	}
}

inline void
finishRule1HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	static const std::set< QString > finish = {
		QStringLiteral( "</pre>" ),
		QStringLiteral( "</script>" ),
		QStringLiteral( "</style>" ),
		QStringLiteral( "</textarea>" )
	};

	const auto start = it;

	for( it = std::next( it ); it != last; ++it )
	{
		bool doBreak = false;

		if( it->m_type == Delimiter::Less )
		{
			QString tag;
			bool closed = false;

			std::tie( tag, closed ) = readHtmlTag( it, po );

			if( closed )
			{
				tag.prepend( c_60 );
				tag.append( c_62 );

				tag = tag.toLower();

				if( finish.find( tag ) != finish.cend() )
				{
					eatRawHtml( start->m_line, start->m_pos, it->m_line, -1, po, true, 1 );

					return;
				}
			}
		}
	}

	eatRawHtml( start->m_line, start->m_pos, po.fr.size() - 1, -1, po, false, 1 );
}

inline void
finishRule2HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	auto start = it;

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::Greater )
		{
			if( it->m_pos > 1 && po.fr[ it->m_line ][ it->m_pos - 1 ] == c_45 &&
				po.fr[ it->m_line ][ it->m_pos - 2 ] == c_45 )
			{
				qsizetype i = it->m_pos + 1;

				for( ; i < po.fr[ it->m_line ].size(); ++i )
				{
					if( po.fr[ it->m_line ][ i ] == c_60 )
						break;
				}

				eatRawHtml( start->m_line, start->m_pos, it->m_line, i , po, true, 2 );

				return;
			}
		}
	}

	eatRawHtml( start->m_line, start->m_pos, po.fr.size() - 1, -1, po, false, 2 );
}

inline void
finishRule3HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	auto start = it;

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::Greater )
		{
			if( it->m_pos > 0 && po.fr[ it->m_line ][ it->m_pos - 1 ] == c_63 )
			{
				qsizetype i = it->m_pos + 1;

				for( ; i < po.fr[ it->m_line ].size(); ++i )
				{
					if( po.fr[ it->m_line ][ i ] == c_60 )
						break;
				}

				eatRawHtml( start->m_line, start->m_pos, it->m_line, i , po, true, 3 );

				return;
			}
		}
	}

	eatRawHtml( start->m_line, start->m_pos, po.fr.size() - 1, -1, po, false, 3 );
}

inline void
finishRule4HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	auto start = it;

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::Greater )
		{
			qsizetype i = it->m_pos + 1;

			for( ; i < po.fr[ it->m_line ].size(); ++i )
			{
				if( po.fr[ it->m_line ][ i ] == c_60 )
					break;
			}

			eatRawHtml( start->m_line, start->m_pos, it->m_line, i , po, true, 4 );

			return;
		}
	}

	eatRawHtml( start->m_line, start->m_pos, po.fr.size() - 1, -1, po, false, 4 );
}

inline void
finishRule5HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	auto start = it;

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::Greater )
		{
			if( it->m_pos > 1 && po.fr[ it->m_line ][ it->m_pos - 1 ] == c_93 &&
				po.fr[ it->m_line ][ it->m_pos - 2 ] == c_93 )
			{
				qsizetype i = it->m_pos + 1;

				for( ; i < po.fr[ it->m_line ].size(); ++i )
				{
					if( po.fr[ it->m_line ][ i ] == c_60 )
						break;
				}

				eatRawHtml( start->m_line, start->m_pos, it->m_line, i , po, true, 5 );

				return;
			}
		}
	}

	eatRawHtml( start->m_line, start->m_pos, po.fr.size() - 1, -1, po, false, 5 );
}

inline void
finishRule6HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	eatRawHtml( po.line, po.pos, po.fr.size() - 1, -1, po, true, 6 );
}

inline Delims::const_iterator
finishRule7HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po );

inline Delims::const_iterator
finishRawHtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	switch( po.html.htmlBlockType )
	{
		case 1 :
			finishRule1HtmlTag( it, last, po );
			break;

		case 2 :
			finishRule2HtmlTag( it, last, po );
			break;

		case 3 :
			finishRule3HtmlTag( it, last, po );
			break;

		case 4 :
			finishRule4HtmlTag( it, last, po );
			break;

		case 5 :
			finishRule5HtmlTag( it, last, po );
			break;

		case 6 :
			finishRule6HtmlTag( it, last, po );
			break;

		case 7 :
			return finishRule7HtmlTag( it, last, po );

		default :
			break;
	}

	return findIt( it, last, po );
}

inline void
skipSpacesInHtml( qsizetype & l, qsizetype & p, const QStringList & fr )
{
	while( l < fr.size() )
	{
		while( p < fr[ l ].size() )
		{
			if( !fr[ l ][ p ].isSpace() )
				return;

			++p;
		}

		p = 0;
		++l;
	}
}

inline std::pair< bool, bool >
readHtmlAttr( qsizetype & l, qsizetype & p, const QStringList & fr )
{
	skipSpacesInHtml( l, p, fr );

	if( l >= fr.size() )
		return { false, false };

	if( p < fr[ l ].size() && fr[ l ][ p ] == c_62 )
		return { false, true };

	for( ; p < fr[ l ].size(); ++p )
	{
		const auto ch = fr[ l ][ p ];

		if( ch.isSpace() || ch == c_62 || ch == c_61 )
			break;
	}

	if( p < fr[ l ].size() && fr[ l ][ p ] == c_62 )
		return { false, false };

	skipSpacesInHtml( l, p, fr );

	if( l >= fr.size() )
		return { false, false };

	if( p < fr[ l ].size() && fr[ l ][ p ] != c_61 )
		return { false, false };

	++p;

	skipSpacesInHtml( l, p, fr );

	if( l >= fr.size() )
		return { false, false };

	if( p < fr[ l ].size() && fr[ l ][ p ] != c_34 && fr[ l ][ p ] != c_39 )
		return { false, false };

	const auto s = fr[ l ][ p ];

	++p;

	if( p >= fr[ l ].size() )
		return { false, false };

	for( ; l < fr.size(); ++l )
	{
		bool doBreak = false;

		for( ; p < fr[ l ].size(); ++p )
		{
			const auto ch = fr[ l ][ p ];

			if( ch == s )
			{
				doBreak = true;

				break;
			}
		}

		if( doBreak )
			break;

		p = 0;
	}

	if( l >= fr.size() )
		return { false, false };

	if( p >= fr[ l ].size() )
		return { false, false };

	if( fr[ l ][ p ] != s )
		return { false, false };

	++p;

	return { true, true };
}

inline std::tuple< bool, qsizetype, qsizetype >
isHtmlTag( Delims::const_iterator it, TextParsingOpts & po )
{
	if( it->m_type == Delimiter::Less )
	{
		qsizetype l = it->m_line;
		qsizetype p = it->m_pos + 1;

		if( p >= po.fr[ l ].size() )
			return { false, -1, -1 };

		for( ; p < po.fr[ l ].size(); ++p )
		{
			const auto ch = po.fr[ l ][ p ];

			if( ch.isSpace() || ch == c_62 )
				break;
		}

		if( p < po.fr[ l ].size() && po.fr[ l ][ p ] == c_62 )
			return { true, l, p };

		skipSpacesInHtml( l, p, po.fr );

		if( l >= po.fr.size() )
			return { false, -1, -1 };

		if( po.fr[ l ][ p ] == c_62 )
			return { true, l, p };

		bool attr = true;

		while( attr )
		{
			bool ok = false;

			std::tie( attr, ok ) = readHtmlAttr( l, p, po.fr );

			if( !ok )
				return { false, -1, -1 };
		}

		skipSpacesInHtml( l, p, po.fr );

		if( l >= po.fr.size() )
			return { false, -1, -1 };

		if( po.fr[ l ][ p ] == c_62 )
			return { true, l, p };
	}

	return { false, -1, -1 };
}

inline int
htmlTagRule( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	QString tag;

	std::tie( tag, std::ignore ) = readHtmlTag( it, po );

	tag = tag.toLower();

	if( tag.isEmpty() )
		return -1;

	static const QString c_validHtmlTagLetters =
		QStringLiteral( "abcdefghijklmnopqrstuvwxyz0123456789/-!?[" );

	for( qsizetype i = 0; i < tag.size(); ++i )
	{
		if( !c_validHtmlTagLetters.contains( tag[ i ] ) )
			return -1;
	}

	static const std::set< QString > rule1 = {
		QStringLiteral( "pre" ), QStringLiteral( "script" ),
		QStringLiteral( "style" ), QStringLiteral( "textarea" )
	};

	if( rule1.find( tag ) != rule1.cend() )
		return 1;
	else if( tag == QStringLiteral( "!--" ) )
		return 2;
	else if( tag == QStringLiteral( "?" ) )
		return 3;
	else if( tag.startsWith( QLatin1Char( '!' ) ) && tag.size() > 1 &&
		( ( tag[ 1 ].unicode() >= 65 && tag[ 1 ].unicode() <= 90 ) ||
			tag[ 1 ].unicode() >= 97 && tag[ 1 ].unicode() <= 122 ) )
	{
		return 4;
	}
	else if( tag == QStringLiteral( "![cdata[" ) )
		return 5;
	else
	{
		if( tag.startsWith( c_47 ) )
			tag.remove( 0, 1 );

		static const std::set< QString > rule6 = {
			QStringLiteral( "address" ), QStringLiteral( "article" ), QStringLiteral( "aside" ),
			QStringLiteral( "base" ), QStringLiteral( "basefont" ), QStringLiteral( "blockquote" ),
			QStringLiteral( "body" ), QStringLiteral( "caption" ), QStringLiteral( "center" ),
			QStringLiteral( "col" ), QStringLiteral( "colgroup" ), QStringLiteral( "dd" ),
			QStringLiteral( "details" ), QStringLiteral( "dialog" ), QStringLiteral( "dir" ),
			QStringLiteral( "div" ), QStringLiteral( "dl" ), QStringLiteral( "dt" ),
			QStringLiteral( "fieldset" ), QStringLiteral( "figcaption" ), QStringLiteral( "figure" ),
			QStringLiteral( "footer" ), QStringLiteral( "form" ), QStringLiteral( "frame" ),
			QStringLiteral( "frameset" ), QStringLiteral( "h1" ), QStringLiteral( "h2" ),
			QStringLiteral( "h3" ), QStringLiteral( "h4" ), QStringLiteral( "h5" ),
			QStringLiteral( "h6" ), QStringLiteral( "head" ), QStringLiteral( "header" ),
			QStringLiteral( "hr" ), QStringLiteral( "html" ), QStringLiteral( "iframe" ),
			QStringLiteral( "legend" ), QStringLiteral( "li" ), QStringLiteral( "link" ),
			QStringLiteral( "main" ), QStringLiteral( "menu" ), QStringLiteral( "menuitem" ),
			QStringLiteral( "nav" ), QStringLiteral( "noframes" ), QStringLiteral( "ol" ),
			QStringLiteral( "optgroup" ), QStringLiteral( "option" ), QStringLiteral( "p" ),
			QStringLiteral( "param" ), QStringLiteral( "section" ), QStringLiteral( "source" ),
			QStringLiteral( "summary" ), QStringLiteral( "table" ), QStringLiteral( "tbody" ),
			QStringLiteral( "td" ), QStringLiteral( "tfoot" ), QStringLiteral( "th" ),
			QStringLiteral( "thead" ), QStringLiteral( "title" ), QStringLiteral( "tr" ),
			QStringLiteral( "track" ), QStringLiteral( "ul" )
		};

		if( rule6.find( tag ) != rule6.cend() )
			return 6;
		else
		{
			bool tag = false;

			std::tie( tag, std::ignore, std::ignore ) = isHtmlTag( it, po );

			if( tag )
				return 7;
		}
	}

	return -1;
}

inline Delims::const_iterator
checkForRawHtml( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto rule = htmlTagRule( it, last, po );

	if( rule == -1 )
	{
		po.html.htmlBlockType = -1;
		po.html.html.reset( nullptr );

		return it;
	}

	po.html.htmlBlockType = rule;
	po.html.html.reset( new RawHtml );

	return finishRawHtmlTag( it, last, po );
}

inline Delims::const_iterator
finishRule7HtmlTag( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	qsizetype l = -1, p = -1;

	std::tie( std::ignore, l, p ) = isHtmlTag( it, po );

	eatRawHtml( po.line, po.pos, l, ++p, po, false, 7 );

	it = findIt( it, last, po );

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::Less )
		{
			const auto rule = htmlTagRule( it, last, po );

			if( rule != -1 && rule != 7 )
			{
				eatRawHtml( po.line, po.pos, it->m_line, it->m_pos, po, true, 7, true );

				return std::prev( it );
			}
		}
	}

	eatRawHtml( po.line, po.pos, po.fr.size() - 1, -1, po, true, 7, true );

	return std::prev( last );
}

inline Delims::const_iterator
checkForAutolinkHtml( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po, bool updatePos )
{
	const auto nit = std::find_if( std::next( it ), last,
		[] ( const auto & d ) { return ( d.m_type == Delimiter::Greater ); } );

	if( nit != last )
	{
		if( !po.collectRefLinks )
		{
			if( nit->m_line == it->m_line )
			{
				const auto url = po.fr.at( it->m_line ).sliced( it->m_pos + 1,
					nit->m_pos - it->m_pos - 1 );

				const auto sit = std::find_if( url.cbegin(), url.cend(),
					[] ( const auto & c ) { return c.isSpace(); } );

				bool isUrl = true;

				if( sit != url.cend() )
					isUrl = false;
				else
				{
					static const QRegularExpression er(
						"^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?"
						"(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$" );

					QRegularExpressionMatch erm;

					if( url.startsWith( QStringLiteral( "mailto:" ), Qt::CaseInsensitive ) )
						erm = er.match( url.right( url.length() - 7 ) );
					else
						erm = er.match( url );

					const QUrl u( url );

					if( ( !u.isValid() || u.isRelative() ) && !erm.hasMatch() )
						isUrl = false;
				}

				if( isUrl )
				{
					QSharedPointer< Link > lnk( new Link );
					lnk->setUrl( url.simplified() );
					lnk->setOpts( po.opts );
					po.parent->appendItem( lnk );
				}
				else
					return checkForRawHtml( it, last, po );
			}
			else
				return checkForRawHtml( it, last, po );
		}

		po.wasRefLink = false;

		if( updatePos )
		{
			po.pos = nit->m_pos + nit->m_len;
			po.line = nit->m_line;
		}

		return nit;
	}
	else
		return checkForRawHtml( it, last, po );
}

inline void
makeInlineCode( qsizetype lastLine, qsizetype lastPos,
	TextParsingOpts & po )
{
	QString c;

	for( ; po.line <= lastLine; ++po.line )
	{
		c.append( po.fr.at( po.line ).sliced( po.pos,
			( po.line == lastLine ? lastPos - po.pos :
				po.fr.at( po.line ).size() - po.pos ) ) );

		if( po.line < lastLine )
			c.append( c_32 );

		po.pos = 0;
	}

	po.line = lastLine;

	if( c.front() == c_32 && c.back() == c_32 && skipSpaces( 0, c ) < c.size() )
	{
		c.remove( 0, 1 );
		c.remove( c.size() - 1, 1 );
	}

	if( !c.isEmpty() )
		po.parent->appendItem( QSharedPointer< Code >( new Code( c, true ) ) );

	po.wasRefLink = false;
}

inline Delims::const_iterator
checkForInlineCode( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto len = it->m_len;
	const auto start = it;

	++it;

	for( ; it != last; ++it )
	{
		if( it->m_type == Delimiter::InlineCode &&
			( it->m_len - ( it->m_backslashed ? 1 : 0 ) ) == len )
		{
			if( !po.collectRefLinks )
			{
				makeText( start->m_line, start->m_pos, po );

				po.pos = start->m_pos + start->m_len;

				makeInlineCode( it->m_line, it->m_pos + ( it->m_backslashed ? 1 : 0 ), po );

				po.line = it->m_line;
				po.pos = it->m_pos + it->m_len;
			}

			po.wasRefLink = false;

			return it;
		}
	}

	if( !po.collectRefLinks )
		makeText( start->m_line, start->m_pos + start->m_len, po );

	po.wasRefLink = false;

	return start;
}

inline QPair< QString, Delims::const_iterator >
readTextBetweenSquareBrackets( Delims::const_iterator start,
	Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po,
	bool doNotCreateTextOnFail )
{
	if( it != last )
	{
		if( start->m_line == it->m_line )
		{
			const auto p = start->m_pos + start->m_len;
			const auto n = it->m_pos - p;

			return { po.fr.at( start->m_line ).sliced( p, n ).simplified(),
				it };
		}
		else
		{
			if( it->m_line - start->m_line < 3 )
			{
				auto text = po.fr.at( start->m_line ).sliced( start->m_pos + start->m_len );

				qsizetype i = start->m_line + 1;

				for( ; i <= it->m_line; ++i )
				{
					text.append( c_32 );

					if( i == it->m_line )
						text.append( po.fr.at( i ).sliced( 0, it->m_pos ) );
					else
						text.append( po.fr.at( i ) );
				}

				return { text.simplified(), it };
			}
			else
			{
				if( !po.collectRefLinks && !doNotCreateTextOnFail )
					makeText( start->m_line, start->m_pos + start->m_len, po );

				return { {}, start };
			}
		}
	}
	else
	{
		if( !po.collectRefLinks && !doNotCreateTextOnFail )
			makeText( start->m_line, start->m_pos + start->m_len, po );

		return { {}, start };
	}
}

inline QPair< QString, Delims::const_iterator >
checkForLinkText( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto start = it;

	qsizetype brackets = 0;

	const bool collectRefLinks = po.collectRefLinks;
	po.collectRefLinks = true;

	for( it = std::next( it ); it != last; ++it )
	{
		bool quit = false;

		switch( it->m_type )
		{
			case Delimiter::SquareBracketsClose :
			{
				if( !brackets )
					quit = true;
				else
					--brackets;
			}
				break;

			case Delimiter::SquareBracketsOpen :
			case Delimiter::ImageOpen :
				++brackets;
				break;

			case Delimiter::InlineCode :
				it = checkForInlineCode( it, last, po );
				break;

			case Delimiter::Less :
				it = checkForAutolinkHtml( it, last, po, false );
				break;

			default :
				break;
		}

		if( quit )
			break;
	}

	const auto r =  readTextBetweenSquareBrackets( start, it, last, po, false );

	po.collectRefLinks = collectRefLinks;

	return r;
}

inline QPair< QString, Delims::const_iterator >
checkForLinkLabel( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto start = it;

	for( it = std::next( it ); it != last; ++it )
	{
		bool quit = false;

		switch( it->m_type )
		{
			case Delimiter::SquareBracketsClose :
			{
				quit = true;
			}
				break;

			case Delimiter::SquareBracketsOpen :
			case Delimiter::ImageOpen :
			{
				it = last;
				quit = true;
			}
				break;

			default :
				break;
		}

		if( quit )
			break;
	}

	return readTextBetweenSquareBrackets( start, it, last, po, true );
}

inline QSharedPointer< Link >
makeLink( const QString & url, const QString & text,
	TextParsingOpts & po,
	bool doNotCreateTextOnFail,
	qsizetype lastLine, qsizetype lastPos )
{
	QString u = ( url.startsWith( c_35 ) ? url : removeBackslashes( url ) );

	if( !u.isEmpty() )
	{
		if( !u.startsWith( c_35 ) )
		{
			if( QUrl( u ).isRelative() )
			{
				if( fileExists( u, po.workingPath ) )
				{
					u = QFileInfo( po.workingPath + u ).absoluteFilePath();

					po.linksToParse.append( u );
				}
			}
		}
		else
			u = u + QStringLiteral( "/" ) + po.workingPath + po.fileName;
	}

	QSharedPointer< Link > link( new Link );
	link->setUrl( u );
	link->setOpts( po.opts );

	QStringList tmp;
	tmp << text;

	QSharedPointer< Paragraph > p( new Paragraph );

	RawHtmlBlock html;

	parseFormattedText( tmp, p, po.doc,
		po.linksToParse, po.workingPath,
		po.fileName, po.collectRefLinks, true, html );

	if( !p->isEmpty() )
	{
		QSharedPointer< Image > img;

		if( p->items().size() == 1 && p->items().at( 0 )->type() == ItemType::Paragraph )
		{
			const auto ip = p->items().at( 0 ).staticCast< Paragraph > ();

			for( auto it = ip->items().cbegin(), last = ip->items().cend(); it != last; ++it )
			{
				switch( (*it)->type() )
				{
					case ItemType::Link :
						return {};

					case ItemType::Image :
					{
						img = (*it).staticCast< Image > ();
					}
						break;

					default :
						break;
				}
			}

			if( !img.isNull() )
				link->setImg( img );

			link->setP( ip );
		}
	}

	link->setText( text );

	return link;
}

inline bool
createShortcutLink( const QString & text,
	TextParsingOpts & po,
	qsizetype lastLine, qsizetype lastPos,
	Delims::const_iterator lastIt,
	const QString & linkText,
	bool doNotCreateTextOnFail )
{
	const auto u = QString::fromLatin1( "#" ) + text.simplified().toCaseFolded().toUpper();
	const auto url = u + QStringLiteral( "/" ) + po.workingPath + po.fileName;

	po.wasRefLink = false;

	if( po.doc->labeledLinks().contains( url ) )
	{
		if( !po.collectRefLinks )
		{
			const auto link = makeLink( u,
				removeBackslashes( linkText.isEmpty() ? text : linkText ), po,
				doNotCreateTextOnFail, lastLine, lastPos );

			if( !link.isNull() )
			{
				po.linksToParse.append( url );
				po.parent->appendItem( link );

				po.line = lastIt->m_line;
				po.pos = lastIt->m_pos + lastIt->m_len;
			}
			else
			{
				if( !po.collectRefLinks && !doNotCreateTextOnFail )
					makeText( lastLine, lastPos, po );

				return false;
			}
		}

		return true;
	}
	else if( !po.collectRefLinks && !doNotCreateTextOnFail )
		makeText( lastLine, lastPos, po );

	return false;
}

inline QSharedPointer< Image >
makeImage( const QString & url, const QString & text,
	TextParsingOpts & po,
	bool doNotCreateTextOnFail,
	qsizetype lastLine, qsizetype lastPos )
{
	QSharedPointer< Image > img( new Image );

	if( !QUrl( url ).isRelative() )
		img->setUrl( url );
	else
		img->setUrl( fileExists( url, po.workingPath ) ? po.workingPath + url : url );

	QStringList tmp;
	tmp << text;

	QSharedPointer< Paragraph > p( new Paragraph );

	RawHtmlBlock html;

	parseFormattedText( tmp, p, po.doc,
		po.linksToParse, po.workingPath,
		po.fileName, po.collectRefLinks, true, html );

	if( !p->isEmpty() )
	{
		if( p->items().size() == 1 && p->items().at( 0 )->type() == ItemType::Paragraph )
			img->setP( p->items().at( 0 ).staticCast< Paragraph > () );
	}

	img->setText( removeBackslashes( text ) );

	return img;
}

inline bool
createShortcutImage( const QString & text,
	TextParsingOpts & po,
	qsizetype lastLine, qsizetype lastPos,
	Delims::const_iterator lastIt,
	const QString & linkText,
	bool doNotCreateTextOnFail )
{
	const auto url = QString::fromLatin1( "#" ) + text.simplified().toCaseFolded().toUpper() +
		QStringLiteral( "/" ) + po.workingPath + po.fileName;

	po.wasRefLink = false;

	if( po.doc->labeledLinks().contains( url ) )
	{
		if( !po.collectRefLinks )
		{
			const auto img = makeImage( po.doc->labeledLinks()[ url ]->url(),
				( linkText.isEmpty() ? text : linkText ), po,
				doNotCreateTextOnFail, lastLine, lastPos );

			po.parent->appendItem( img );

			po.line = lastIt->m_line;
			po.pos = lastIt->m_pos + lastIt->m_len;
		}

		return true;
	}
	else if( !po.collectRefLinks && !doNotCreateTextOnFail )
		makeText( lastLine, lastPos, po );

	return false;
}

inline void
skipSpacesUpTo1Line( qsizetype & line, qsizetype & pos, const QStringList & fr )
{
	pos = skipSpaces( pos, fr.at( line ) );

	if( pos == fr.at( line ).size() && line + 1 < fr.size() )
	{
		++line;
		pos = skipSpaces( 0, fr.at( line ) );
	}
}

inline std::tuple< qsizetype, qsizetype, bool, QString, qsizetype >
readLinkDestination( qsizetype line, qsizetype pos, const QStringList & fr )
{
	skipSpacesUpTo1Line( line, pos, fr );

	const auto destLine = line;
	const auto & s = fr.at( line );
	QString dest;
	bool backslash = false;

	if( pos < s.size() )
	{
		if( s[ pos ] == c_60 )
		{
			++pos;

			while( pos < s.size() )
			{
				bool now = false;

				if( s[ pos ] == c_92 && !backslash )
				{
					backslash = true;
					now = true;
				}
				else if( !backslash && s[ pos ] == c_60 )
					return { line, pos, false, {}, destLine };
				else if( !backslash && s[ pos ] == c_62 )
					break;
				else
				{
					if( backslash )
						dest.append( c_92 );

					dest.append( s[ pos ] );
				}

				if( !now )
					backslash = false;

				++pos;
			}

			if( pos < s.size() && s[ pos ] == c_62 )
				return { line, ++pos, true, dest, destLine };
			else
				return { line, pos, false, {}, destLine };
		}
		else
		{
			qsizetype pc = 0;

			while( pos < s.size() )
			{
				bool now = false;

				if( s[ pos ] == c_92 && !backslash )
				{
					backslash = true;
					now = true;
				}
				else if( !backslash && ( s[ pos ] == c_32 || s[ pos ] == c_9 ) )
				{
					if( !pc )
						return { line, pos, true, dest, destLine };
					else
						return { line, pos, false, {}, destLine };
				}
				else if( !backslash && s[ pos ] == c_40 )
				{
					++pc;
					dest.append( s[ pos ] );
				}
				else if( !backslash && s[ pos ] == c_41 )
				{
					if( !pc )
						return { line, pos, true, dest, destLine };
					else
					{
						dest.append( s[ pos ] );
						--pc;
					}
				}
				else
				{
					if( backslash )
						dest.append( c_92 );

					dest.append( s[ pos ] );
				}

				if( !now )
					backslash = false;

				++pos;
			}

			return { line, pos, true, dest, destLine };
		}
	}
	else
		return { line, pos, false, {}, destLine };
}

inline std::tuple< qsizetype, qsizetype, bool, QString, qsizetype >
readLinkTitle( qsizetype line, qsizetype pos, const QStringList & fr )
{
	const auto space = ( pos < fr.at( line ).size() ? fr.at( line )[ pos ].isSpace() : true );

	const auto firstLine = line;

	skipSpacesUpTo1Line( line, pos, fr );

	if( pos >= fr.at( line ).size() )
		return { line, pos, true, {}, firstLine };

	const auto sc = fr.at( line )[ pos ];

	if( sc != c_34 && sc != c_39 && sc != c_40 && sc != c_41 )
		return { line, pos, ( firstLine != line ), {}, firstLine };
	else if( !space && sc != c_41 )
		return { line, pos, false, {}, firstLine };

	if( sc == c_41 )
		return { line, pos, true, {}, firstLine };

	const auto startLine = line;

	bool backslash = false;

	++pos;

	skipSpacesUpTo1Line( line, pos, fr );

	QString title;

	while( line < fr.size() && pos < fr.at( line ).size() )
	{
		bool now = false;

		if( fr.at( line )[ pos ] == c_92 && !backslash )
		{
			backslash = true;
			now = true;
		}
		else if( sc == c_40 && fr.at( line )[ pos ] == c_41 && !backslash )
			return { line, ++pos, true, title, startLine };
		else if( sc == c_40 && fr.at( line )[ pos ] == c_40 && !backslash )
			return { line, pos, false, {}, startLine };
		else if( sc != c_40 && fr.at( line )[ pos ] == sc && !backslash )
			return { line, ++pos, true, title, startLine };
		else
			title.append( fr.at( line )[ pos ] );

		if( !now )
			backslash = false;

		++pos;

		if( pos == fr.at( line ).size() )
			skipSpacesUpTo1Line( line, pos, fr );
	}

	return { line, pos, false, {}, startLine };
}

inline std::tuple< QString, QString, Delims::const_iterator, bool >
checkForInlineLink( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	qsizetype p = it->m_pos + it->m_len;
	qsizetype l = it->m_line;
	bool ok = false;
	QString dest, title;
	qsizetype destStartLine = 0;

	std::tie( l, p, ok, dest, destStartLine ) = readLinkDestination( l, p, po.fr );

	if( !ok )
		return { {}, {}, it, false };

	qsizetype s = 0;

	std::tie( l, p, ok, title, s ) = readLinkTitle( l, p, po.fr );

	skipSpacesUpTo1Line( l, p, po.fr );

	if( !ok || ( l >= po.fr.size() || p >= po.fr.at( l ).size() ||  po.fr.at( l )[ p ] != c_41 ) )
		return { {}, {}, it, false };

	for( ; it != last; ++it )
	{
		if( it->m_line == l && it->m_pos == p )
			return { dest, title, it, true };
	}

	return { {}, {}, it, false };
}

inline std::tuple< QString, QString, Delims::const_iterator, bool >
checkForRefLink( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	qsizetype p = it->m_pos + it->m_len + 1;
	qsizetype l = it->m_line;
	bool ok = false;
	QString dest, title;
	qsizetype destStartLine = 0;

	std::tie( l, p, ok, dest, destStartLine ) = readLinkDestination( l, p, po.fr );

	if( !ok )
		return { {}, {}, it, false };

	const auto dp = p, dl = l;
	qsizetype titleStartLine = 0;

	std::tie( l, p, ok, title, titleStartLine ) = readLinkTitle( l, p, po.fr );

	if( !ok )
		return { {}, {}, it, false };

	if( !title.isEmpty() )
	{
		p = skipSpaces( p, po.fr.at( l ) );

		if( titleStartLine == destStartLine && p < po.fr.at( l ).size() )
			return { {}, {}, it, false };
		else if( titleStartLine != destStartLine && p < po.fr.at( l ).size() )
		{
			l = destStartLine;
			p = po.fr.at( l ).size();
			title.clear();
		}
	}

	for( ; it != last; ++it )
	{
		if( it->m_line > l || ( it->m_line == l && it->m_pos >= p ) )
			break;
	}

	po.line = l;
	po.pos = p;

	return { dest, title, std::prev( it ), true };
}

inline Delims::const_iterator
checkForImage( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto start = it;

	QString text;

	po.wasRefLink = false;

	std::tie( text, it ) = checkForLinkText( it, last, po );

	if( it != start )
	{
		if( it->m_pos + it->m_len < po.fr.at( it->m_line ).size() )
		{
			// Inline -> (
			if( po.fr.at( it->m_line )[ it->m_pos + it->m_len ] == c_40 )
			{
				QString url, title;
				Delims::const_iterator iit;
				bool ok;

				std::tie( url, title, iit, ok ) = checkForInlineLink( std::next( it ), last, po );

				if( ok )
				{
					if( !po.collectRefLinks )
						po.parent->appendItem( makeImage( url, text, po, false,
							start->m_line, start->m_pos + start->m_len ) );

					po.line = iit->m_line;
					po.pos = iit->m_pos + iit->m_len;

					return iit;
				}
				else if( createShortcutImage( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
				{
					return it;
				}
			}
			// Reference -> [
			else if( po.fr.at( it->m_line )[ it->m_pos + it->m_len ] == c_91 )
			{
				QString label;
				Delims::const_iterator lit;

				std::tie( label, lit ) = checkForLinkLabel( std::next( it ), last, po );

				if( lit != std::next( it ) )
				{
					if( !label.simplified().isEmpty() &&
						createShortcutImage( label.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							lit, text, true ) )
					{
						return lit;
					}
					else if( label.simplified().isEmpty() &&
						createShortcutImage( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							lit, {}, false ) )
					{
						return lit;
					}
				}
				else if( createShortcutImage( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
				{
					return it;
				}
			}
			else
			{
				std::tie( text, it ) = checkForLinkLabel( start, last, po );

				if( it != start && !text.simplified().isEmpty() )
				{
					if( createShortcutImage( text.simplified(),
						po, start->m_line, start->m_pos + start->m_len,
						it, {}, false ) )
					{
						return it;
					}
				}
			}
		}
		// Shortcut
		else
		{
			std::tie( text, it ) = checkForLinkLabel( start, last, po );

			if( it != start && !text.simplified().isEmpty() )
			{
				if( createShortcutImage( text.simplified(),
					po, start->m_line, start->m_pos + start->m_len,
					it, {}, false ) )
				{
					return it;
				}
			}
		}
	}
	else if( !po.collectRefLinks )
		makeText( start->m_line, start->m_pos + start->m_len, po );

	return start;
}

inline Delims::const_iterator
checkForLink( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto start = it;

	QString text;

	const auto wasRefLink = po.wasRefLink;
	po.wasRefLink = false;

	const auto ns = skipSpaces( 0, po.fr.at( po.line ) );

	std::tie( text, it ) = checkForLinkText( it, last, po );

	if( it != start )
	{
		// Footnote reference.
		if( text.startsWith( c_94 ) )
		{
			if( !po.collectRefLinks )
			{
				QSharedPointer< FootnoteRef > fnr(
					new FootnoteRef( QStringLiteral( "#" ) +
						text.simplified().toCaseFolded().toUpper() +
						QStringLiteral( "/" ) + po.workingPath + po.fileName ) );

				po.parent->appendItem( fnr );
			}

			po.line = it->m_line;
			po.pos = it->m_pos + it->m_len;

			return it;
		}
		else if( it->m_pos + it->m_len < po.fr.at( it->m_line ).size() )
		{
			// Reference definition -> :
			if( po.fr.at( it->m_line )[ it->m_pos + it->m_len ] == c_58 )
			{
				// Reference definitions allowed only at start of paragraph.
				if( ( po.line == 0 || wasRefLink ) && ns < 4 && start->m_pos == ns )
				{
					QString url, title;
					Delims::const_iterator iit;
					bool ok;

					std::tie( text, it ) = checkForLinkLabel( start, last, po );

					if( it != start && !text.simplified().isEmpty() )
					{
						std::tie( url, title, iit, ok ) = checkForRefLink( it, last, po );

						if( ok )
						{
							const auto label = QString::fromLatin1( "#" ) +
								text.simplified().toCaseFolded().toUpper() +
								QStringLiteral( "/" ) + po.workingPath + po.fileName;

							QSharedPointer< Link > link( new Link );

							url = removeBackslashes( url );

							if( !url.isEmpty() )
							{
								if( QUrl( url ).isRelative() )
								{
									if( fileExists( url, po.workingPath ) )
										url = QFileInfo( po.workingPath + url ).absoluteFilePath();
								}
							}

							link->setUrl( url );

							po.wasRefLink = true;

							if( !po.doc->labeledLinks().contains( label ) )
								po.doc->insertLabeledLink( label, link );

							return iit;
						}
						else
						{
							if( !po.collectRefLinks )
								makeText( start->m_line, start->m_pos + start->m_len, po );

							return start;
						}
					}
					else
					{
						if( !po.collectRefLinks )
							makeText( start->m_line, start->m_pos + start->m_len, po );

						return start;
					}
				}
				else if( !po.collectRefLinks )
				{
					makeText( start->m_line, start->m_pos + start->m_len, po );

					return start;
				}
			}
			// Inline -> (
			else if( po.fr.at( it->m_line )[ it->m_pos + it->m_len ] == c_40 )
			{
				QString url, title;
				Delims::const_iterator iit;
				bool ok;

				std::tie( url, title, iit, ok ) = checkForInlineLink( std::next( it ), last, po );

				if( ok )
				{
					const auto link = makeLink( url, removeBackslashes( text ), po, false,
						start->m_line, start->m_pos + start->m_len );

					if( !link.isNull() )
					{
						if( !po.collectRefLinks )
							po.parent->appendItem( link );

						po.line = iit->m_line;
						po.pos = iit->m_pos + iit->m_len;

						return iit;
					}
					else
						return start;
				}
				else if( createShortcutLink( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
				{
					return it;
				}
			}
			// Reference -> [
			else if( po.fr.at( it->m_line )[ it->m_pos + it->m_len ] == c_91 )
			{
				QString label;
				Delims::const_iterator lit;

				std::tie( label, lit ) = checkForLinkLabel( std::next( it ), last, po );

				if( lit != std::next( it ) )
				{
					if( !label.simplified().isEmpty() && createShortcutLink( label.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							lit, text, true ) )
					{
						return lit;
					}
					else if( label.simplified().isEmpty() && createShortcutLink( text.simplified(),
								po, start->m_line, start->m_pos + start->m_len,
								it, {}, false ) )
					{
						po.line = lit->m_line;
						po.pos = lit->m_pos + lit->m_len;

						return lit;
					}
				}
				else if( createShortcutLink( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
				{
					return it;
				}
			}
			// Shortcut
			else
			{
				std::tie( text, it ) = checkForLinkLabel( start, last, po );

				if( it != start && !text.simplified().isEmpty() )
				{
					if( createShortcutLink( text.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
					{
						return it;
					}
				}
			}
		}
		// Shortcut
		else
		{
			std::tie( text, it ) = checkForLinkLabel( start, last, po );

			if( it != start && !text.simplified().isEmpty() )
			{
				if( createShortcutLink( text.simplified(),
						po, start->m_line, start->m_pos + start->m_len,
						it, {}, false ) )
				{
					return it;
				}
			}
		}
	}
	else if( !po.collectRefLinks )
		makeText( start->m_line, start->m_pos + start->m_len, po );

	return start;
}

inline bool
isClosingStyle( const std::vector< std::pair< Style, qsizetype > > & styles, Style s )
{
	const auto it = std::find_if( styles.cbegin(), styles.cend(),
		[&] ( const auto & p ) { return ( p.first == s ); } );

	return it != styles.cend();
}

inline void
closeStyle( std::vector< std::pair< Style, qsizetype > > & styles, Style s )
{
	const auto it = std::find_if( styles.crbegin(), styles.crend(),
		[&] ( const auto & p ) { return ( p.first == s ); } );

	if( it != styles.crend() )
		styles.erase( it.base() - 1 );
}

inline void
setStyle( TextOptions & opts, Style s, bool on )
{
	switch( s )
	{
		case Style::Strikethrough :
			opts.setFlag( StrikethroughText, on );
			break;

		case Style::Italic1 :
		case Style::Italic2 :
			opts.setFlag( ItalicText, on );
			break;

		case Style::Bold1 :
		case Style::Bold2 :
			opts.setFlag( BoldText, on );
			break;

		default :
			break;
	}
}

inline void
appendPossibleDelimiter( std::vector< std::vector< std::pair< qsizetype, int > > > & vars,
	qsizetype len, int type )
{
	for( auto & v : vars )
		v.push_back( { len, type } );
}

inline std::vector< std::pair< qsizetype, int > >
longestSequenceWithMoreOpeningsAtStart(
	const std::vector< std::vector< std::pair< qsizetype, int > > > & vars )
{
	size_t max = 0;

	for( const auto & s : vars )
	{
		if( s.size() > max )
			max = s.size();
	}

	std::vector< std::pair< qsizetype, int > > ret;

	size_t maxOp = 0;

	for( const auto & s : vars )
	{
		if( s.size() == max )
		{
			size_t op = 0;

			for( const auto & v : s )
			{
				if( v.first > 0 )
					++op;
				else
					break;
			}

			if( op > maxOp )
			{
				maxOp = op;
				ret = s;
			}
		}
	}

	return ret;
}

inline std::vector< std::vector< std::pair< qsizetype, int > > >
closedSequences( const std::vector< std::vector< std::pair< qsizetype, int > > > & vars,
	size_t idx )
{
	std::vector< std::vector< std::pair< qsizetype, int > > > tmp;

	const auto longest = longestSequenceWithMoreOpeningsAtStart( vars );

	for( const auto & v : vars )
	{
		if( longest.size() == v.size() )
		{
			bool closed = false;
			std::tie( closed, std::ignore ) = checkEmphasisSequence( v, idx );

			if( closed )
				tmp.push_back( v );
		}
	}

	return tmp;
}

inline void
collectDelimiterVariants( std::vector< std::vector< std::pair< qsizetype, int > > > & vars,
	qsizetype itLength, int type, bool leftFlanking, bool rightFlanking )
{
	{
		auto vars1 = vars;
		auto vars2 = vars;

		vars.clear();

		if( leftFlanking )
		{
			appendPossibleDelimiter( vars1, itLength, type );
			std::copy( vars1.cbegin(), vars1.cend(), std::back_inserter( vars ) );
		}

		if( rightFlanking )
		{
			appendPossibleDelimiter( vars2, -itLength, type );
			std::copy( vars2.cbegin(), vars2.cend(), std::back_inserter( vars ) );
		}
	}
}

inline void
createStyles( std::vector< std::pair< Style, qsizetype > > & s, qsizetype l,
	Delimiter::DelimiterType t, qsizetype & count )
{
	if( t != Delimiter::Strikethrough )
	{
		if( l % 2 == 1 )
		{
			s.push_back( { t == Delimiter::Emphasis1 ?
				Style::Italic1 : Style::Italic2, 1 } );
			++count;
		}

		if( l >= 2 )
		{
			s.push_back( { t == Delimiter::Emphasis1 ?
				Style::Bold1 : Style::Bold2, l / 2 } );
			count += ( l / 2 ) * 2;
		}
	}
	else
	{
		s.push_back( { Style::Strikethrough, l / 2 } );
		count += l / 2;
	}
}

inline std::vector< std::pair< Style, qsizetype > >
createStyles( const std::vector< std::pair< qsizetype, int > > & s, size_t i,
	Delimiter::DelimiterType t, qsizetype & count )
{
	std::vector< std::pair< Style, qsizetype > > styles;

	const size_t idx = i;
	qsizetype len = s.at( i ).first;

	size_t closeIdx = 0;
	std::tie( std::ignore, closeIdx ) = checkEmphasisSequence( s, i );

	for( i = closeIdx; i >= 0; --i )
	{
		if( s[ i ].second == s[ idx ].second && s[ i ].first < 0 )
		{
			auto l = qAbs( s[ i ].first );

			createStyles( styles, qMin( l, len ), t, count );

			len -= qMin( l, len );

			if( !len )
				break;
		}
	}

	return styles;
}

inline bool
isSequence( Delims::const_iterator it, qsizetype itLine, qsizetype itPos,
	Delimiter::DelimiterType t )
{
	return ( itLine == it->m_line &&
		itPos + ( it->m_type != Delimiter::Strikethrough ? 1 : 2 ) == it->m_pos &&
		it->m_type == t );
}

inline Delims::const_iterator
readSequence( Delims::const_iterator it, Delims::const_iterator last,
	qsizetype & line, qsizetype & pos, qsizetype & len,
	Delims::const_iterator & current )
{
	line = it->m_line;
	pos = it->m_pos;
	len = ( it->m_type != Delimiter::Strikethrough ? 1 : 2 );
	current = it;
	const auto t = it->m_type;

	it = std::next( it );

	while( it != last && isSequence( it, line, pos, t ) )
	{
		current = it;

		if( it->m_type != Delimiter::Strikethrough )
		{
			++pos;
			++len;
		}
		else
		{
			pos += 2;
			len += 2;
		}

		++it;
	}

	return std::prev( it );
}

inline int
emphasisToInt( Delimiter::DelimiterType t )
{
	switch( t )
	{
		case Delimiter::Strikethrough :
			return 0;

		case Delimiter::Emphasis1 :
			return 1;

		case Delimiter::Emphasis2 :
			return 2;

		default :
			return -1;
	}
}

inline std::tuple< bool, std::vector< std::pair< Style, qsizetype > >, qsizetype, qsizetype >
isStyleClosed( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto open = it;
	auto current =  it;

	std::vector< std::vector< std::pair< qsizetype, int > > > vars, closed;
	vars.push_back( {} );

	qsizetype itLine = open->m_line, itPos = open->m_pos,
		itLength = ( open->m_type != Delimiter::Strikethrough ? 1 : 2 );

	const qsizetype line = po.line, pos = po.pos;
	const bool collectRefLinks = po.collectRefLinks;

	po.collectRefLinks = true;

	bool first = true;

	{
		const auto c = std::count_if( po.styles.cbegin(), po.styles.cend(),
			[] ( const auto & p ) { return ( p.first == Style::Strikethrough ); } );

		if( c )
			vars.front().push_back( { c * 2, 0 } );
	}

	{
		{
			const auto c1 = std::count_if( po.styles.cbegin(), po.styles.cend(),
				[&] ( const auto & p ) { return ( p.first == Style::Italic1 ); } );

			if( c1 )
				vars.front().push_back( { c1, 1 } );

			const auto c2 = std::count_if( po.styles.cbegin(), po.styles.cend(),
				[&] ( const auto & p ) { return ( p.first == Style::Bold1 ); } ) * 2;

			if( c2 )
				vars.front().push_back( { c2, 1 } );
		}

		{
			const auto c1 = std::count_if( po.styles.cbegin(), po.styles.cend(),
				[&] ( const auto & p ) { return ( p.first == Style::Italic2 ); } );

			if( c1 )
				vars.front().push_back( { c1, 2 } );

			const auto c2 = std::count_if( po.styles.cbegin(), po.styles.cend(),
				[&] ( const auto & p ) { return ( p.first == Style::Bold2 ); } ) * 2;

			if( c2 )
				vars.front().push_back( { c2, 2 } );
		}
	}

	const auto idx = vars.front().size();

	for( ; it != last; ++it )
	{
		switch( it->m_type )
		{
			case Delimiter::SquareBracketsOpen :
				it = checkForLink( it, last, po );
				break;

			case Delimiter::ImageOpen :
				it = checkForImage( it, last, po );
				break;

			case Delimiter::Less :
				it = checkForAutolinkHtml( it, last, po, false );
				break;

			case Delimiter::Strikethrough :
			case Delimiter::Emphasis1 :
			case Delimiter::Emphasis2 :
			{
				it = readSequence( it, last, itLine, itPos, itLength, current );

				if( first )
				{
					vars.front().push_back( { itLength, emphasisToInt( open->m_type ) } );
					first = false;
				}
				else
				{
					collectDelimiterVariants( vars, itLength, emphasisToInt( it->m_type ),
						it->m_leftFlanking, it->m_rightFlanking );
				}
			}
				break;

			case Delimiter::InlineCode :
				it = checkForInlineCode( it, last, po );
				break;

			default :
				break;
		}
	}

	po.line = line;
	po.pos = pos;
	po.collectRefLinks = collectRefLinks;

	closed = closedSequences( vars, idx );

	if( !closed.empty() )
	{
		qsizetype itCount = 0;

		return { true, createStyles(
			longestSequenceWithMoreOpeningsAtStart( closed ), idx, open->m_type, itCount ),
			vars.front().at( idx ).first, itCount };
	}
	else
		return { false, { { Style::Unknown, 0 } },
			( open->m_type != Delimiter::Strikethrough ? 1 : 2 ), 1 };
}

inline Delims::const_iterator
checkForStyle( Delims::const_iterator first, Delims::const_iterator it,
	Delims::const_iterator last, TextParsingOpts & po )
{
	qsizetype count = 1;

	po.wasRefLink = false;

	if( it->m_rightFlanking )
	{
		qsizetype line = it->m_line, pos = it->m_pos + it->m_len, ppos = it->m_pos;
		const auto t = it->m_type;
		qsizetype len = it->m_len;

		for( auto j = std::next( it ); j != last; ++j )
		{
			if( j->m_line == line && pos == j->m_pos && j->m_type == t )
			{
				len += j->m_len;
				pos = j->m_pos + j->m_len;
				++count;
			}
			else
				break;
		}

		if( it != first )
		{
			for( auto j = std::prev( it ); ; --j )
			{
				if( j->m_line == line && ppos - j->m_len == j->m_pos && j->m_type == t )
				{
					len += j->m_len;
					ppos = j->m_pos;
					++count;
				}
				else
					break;

				if( j == first )
					break;
			}
		}

		qsizetype opened = 0;

		for( auto it = po.styles.crbegin(), last = po.styles.crend(); it != last; ++it )
		{
			bool doBreak = false;

			switch( t )
			{
				case Delimiter::Emphasis1 :
				{
					if( it->first == Style::Italic1 || it->first == Style::Bold1 )
					{
						opened = it->second;
						doBreak = true;
					}
				}
					break;

				case Delimiter::Emphasis2 :
				{
					if( it->first == Style::Italic2 || it->first == Style::Bold2 )
					{
						opened = it->second;
						doBreak = true;
					}
				}
					break;

				case Delimiter::Strikethrough :
				{
					if( it->first == Style::Strikethrough )
					{
						opened = it->second;
						doBreak = true;
					}
				}
					break;

				default :
					break;
			}

			if( doBreak )
				break;
		}

		const bool sumMult3 = ( it->m_leftFlanking ? ( ( opened + len ) % 3 == 0 ) : false );

		if( count && opened && ( !sumMult3 || ( count % 3 == 0 && opened % 3 == 0 ) ) )
		{
			if( count > opened )
				count = opened;

			if( it->m_type == Delimiter::Strikethrough )
			{
				for( auto i = 0; i < count; ++i )
					closeStyle( po.styles, Style::Strikethrough );

				if( std::find_if( po.styles.cbegin(), po.styles.cend(),
					[] ( const auto & p ) { return ( p.first == Style::Strikethrough ); } ) ==
						po.styles.cend() )
				{
					setStyle( po.opts, Style::Strikethrough, false );
				}
			}
			else
			{
				if( count % 2 == 1 )
				{
					const auto st = ( it->m_type == Delimiter::Emphasis1 ?
						Style::Italic1 : Style::Italic2 );

					closeStyle( po.styles, st );

					if( std::find_if( po.styles.cbegin(), po.styles.cend(),
						[&] ( const auto & p ) { return ( p.first == st ); } ) == po.styles.cend() )
							setStyle( po.opts, st, false );
				}

				if( count >= 2 )
				{
					const auto st = ( it->m_type == Delimiter::Emphasis1 ?
						Style::Bold1 : Style::Bold2 );

					for( auto i = 0; i < count / 2; ++i )
						closeStyle( po.styles, st );

					if( std::find_if( po.styles.cbegin(), po.styles.cend(),
						[&] ( const auto & p ) { return ( p.first == st ); } ) == po.styles.cend() )
							setStyle( po.opts, st, false );
				}
			}

			const auto j = it + ( count - 1 );

			po.pos = j->m_pos + j->m_len;
			po.line = j->m_line;

			return it + ( count - 1 );
		}
	}

	count = 1;

	if( it->m_leftFlanking )
	{
		switch( it->m_type )
		{
			case Delimiter::Strikethrough :
			case Delimiter::Emphasis1 :
			case Delimiter::Emphasis2 :
			{
				bool closed = false;
				std::vector< std::pair< Style, qsizetype > > styles;
				qsizetype len = 0;

				std::tie( closed, styles, len, count ) = isStyleClosed( it, last, po );

				if( closed )
				{
					for( const auto & p : styles )
					{
						setStyle( po.opts, p.first, true );

						for( qsizetype i = 0; i < p.second; ++i )
							po.styles.push_back( { p.first, len } );
					}

					po.pos = it->m_pos + len;
					po.line = it->m_line;
				}
				else if( !po.collectRefLinks )
					makeText( it->m_line, it->m_pos + len, po );
			}
				break;

			default :
			{
				if( !po.collectRefLinks )
					makeText( it->m_line, it->m_pos + it->m_len, po );
			}
				break;
		}
	}

	if( !count )
		count = 1;

	return it + ( count - 1 );
}

inline QSharedPointer< Text >
concatenateText( Block::Items::const_iterator it, Block::Items::const_iterator last )
{
	QSharedPointer< Text > t( new Text );
	t->setOpts( (*it).staticCast< Text > ()->opts() );
	t->setSpaceBefore( (*it).staticCast< Text > ()->isSpaceBefore() );

	QString data;

	for( ; it != last; ++it )
	{
		const auto tt = (*it).staticCast< Text > ();

		if( tt->isSpaceBefore() )
			data.append( c_32 );

		data.append( tt->text() );

		if( tt->isSpaceAfter() )
			data.append( c_32 );
	}

	t->setText( data.simplified() );

	t->setSpaceAfter( ( *std::prev( it ) ).staticCast< Text > ()->isSpaceAfter() );

	return t;
}

inline void
optimizeParagraph( QSharedPointer< Paragraph > & p )
{
	QSharedPointer< Paragraph > np( new Paragraph );

	TextOptions opts = TextWithoutFormat;

	auto start = p->items().cend();

	for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
	{
		if( (*it)->type() == ItemType::Text )
		{
			const auto t = (*it).staticCast< Text > ();

			if( start == last )
			{
				start = it;
				opts = t->opts();
			}
			else if( opts != t->opts() )
			{
				np->appendItem( concatenateText( start, it ) );
				start = it;
				opts = t->opts();
			}
		}
		else
		{
			if( start != last )
			{
				np->appendItem( concatenateText( start, it ) );
				start = last;
				opts = TextWithoutFormat;
			}

			np->appendItem( (*it) );
		}
	}

	if( start != p->items().cend() )
		np->appendItem( concatenateText( start, p->items().cend() ) );

	p = np;
}

void
parseFormattedText( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak,
	RawHtmlBlock & html )

{
	if( fr.isEmpty() )
		return;

	QSharedPointer< Paragraph > p( new Paragraph );

	const auto delims = collectDelimiters( fr );

	TextParsingOpts po = { fr, p, doc, linksToParse, workingPath,
		fileName, collectRefLinks, ignoreLineBreak, html };

	for( auto it = delims.cbegin(), last = delims.cend(); it != last; ++it )
	{
		if( !html.html.isNull() )
			it = finishRawHtmlTag( it, last, po );
		else
		{
			if( it->m_line > po.line || it->m_pos > po.pos )
			{
				if( !collectRefLinks )
					makeText( it->m_line, it->m_pos, po );
				else
				{
					po.line = it->m_line;
					po.pos = it->m_pos;
				}
			}

			switch( it->m_type )
			{
				case Delimiter::SquareBracketsOpen :
					it = checkForLink( it, last, po );
					break;

				case Delimiter::ImageOpen :
					it = checkForImage( it, last, po );
					break;

				case Delimiter::Less :
					it = checkForAutolinkHtml( it, last, po, true );
					break;

				case Delimiter::Strikethrough :
				case Delimiter::Emphasis1 :
				case Delimiter::Emphasis2 :
						it = checkForStyle( delims.cbegin(), it, last, po );
					break;

				case Delimiter::InlineCode :
				{
					if( !it->m_backslashed )
						it = checkForInlineCode( it, last, po );
				}
					break;

				case Delimiter::HorizontalLine :
				{
					if( !collectRefLinks )
					{
						if( !p->isEmpty() )
						{
							optimizeParagraph( p );
							parent->appendItem( p );
						}

						QSharedPointer< Item > hr( new HorizontalLine );
						parent->appendItem( hr );

						p.reset( new Paragraph );

						po.parent = p;
						po.line = it->m_line;
						po.pos = it->m_pos + it->m_len;
					}
				}
					break;

				default :
				{
					if( !collectRefLinks )
						makeText( it->m_line, it->m_pos + it->m_len, po );
					else
					{
						po.line = it->m_line;
						po.pos = it->m_pos + it->m_len;
					}
				}
					break;
			}
		}
	}

	if( !collectRefLinks )
		makeText( fr.size() - 1, fr.back().length(), po );

	if( !p->isEmpty() )
	{
		optimizeParagraph( p );
		parent->appendItem( p );
	}
}

} /* namespace anonymous */

void
Parser::parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak,
	RawHtmlBlock & html )

{
	parseFormattedText( fr, parent, doc, linksToParse, workingPath, fileName,
		collectRefLinks, ignoreLineBreak, html );
}

void
Parser::parseFootnote( QStringList & fr, QSharedPointer< Block >,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	if( !fr.isEmpty() )
	{
		QSharedPointer< Footnote > f( new Footnote() );

		const auto delims = collectDelimiters( fr );

		RawHtmlBlock html;

		TextParsingOpts po = { fr, f, doc, linksToParse, workingPath,
			fileName, collectRefLinks, false, html };

		if( !delims.isEmpty() && delims.cbegin()->m_type == Delimiter::SquareBracketsOpen &&
			!delims.cbegin()->m_isWordBefore )
		{
			QString id;
			Delims::const_iterator it = delims.cend();

			po.line = delims.cbegin()->m_line;
			po.pos = delims.cbegin()->m_pos;

			std::tie( id, it ) = checkForLinkText( delims.cbegin(), delims.cend(), po );

			if( !id.isEmpty() && id.startsWith( c_94 ) && it != delims.cend() &&
				fr.at( it->m_line ).size() > it->m_pos + 1 &&
				fr.at( it->m_line )[ it->m_pos + 1 ] == c_58 )
			{
				fr = fr.sliced( it->m_line );
				fr.first() = fr.first().sliced( it->m_pos + 2 );

				for( auto it = fr.begin(), last = fr.end(); it != last; ++it )
				{
					if( it->startsWith( QLatin1String( "    " ) ) )
						*it = it->mid( 4 );
					else if( it->startsWith( c_9 ) )
						*it = it->mid( 1 );
				}

				StringListStream stream( fr );

				parse( stream, f, doc, linksToParse, workingPath, fileName, collectRefLinks );

				if( !f->isEmpty() )
					doc->insertFootnote( QString::fromLatin1( "#" ) + id +
						QStringLiteral( "/" ) + workingPath + fileName, f );
			}
		}
	}
}

void
Parser::parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks, RawHtmlBlock & html )
{
	const int pos = fr.first().indexOf( c_62 );

	if( pos > -1 )
	{
		qsizetype i = 0, j = 0;

		bool horLine = false;

		BlockType bt = BlockType::Unknown;

		for( auto it = fr.begin(), last = fr.end(); it != last; ++it, ++i )
		{
			const auto ns = skipSpaces( 0, *it );
			const auto gt = ( ns < it->size() ? ( (*it)[ ns ] == c_62 ? ns : -1 ) : -1 );

			if( gt > -1 )
			{
				*it = it->sliced( gt + ( it->size() > gt + 1 ?
					( (*it)[ gt + 1 ] == c_32 ? 1 : 0 ) : 0 ) + 1 );
				bt = whatIsTheLine( *it );
			}
			// Process lazyness...
			else
			{
				if( ns < 4 && isHorizontalLine( it->sliced( ns ) ) )
					break;

				const auto tmpBt = whatIsTheLine( *it );

				if( bt == BlockType::Text )
				{
					if( isH1( *it ) )
					{
						const auto p = (*it).indexOf( c_61 );

						(*it).insert( p, c_92 );

						continue;
					}
					else if( isH2( *it ) )
					{
						const auto p = (*it).indexOf( c_45 );

						(*it).insert( p, c_92 );

						continue;
					}
				}

				if( ( bt == BlockType::Text || bt == BlockType::Blockquote || bt == BlockType::List )
					&& ( tmpBt == BlockType::Text || tmpBt == BlockType::CodeIndentedBySpaces ) )
				{
					continue;
				}
				else
					break;
			}
		}

		QStringList tmp;

		for( ; j < i; ++j )
			tmp.append( fr.at( j ) );

		StringListStream stream( tmp );

		QSharedPointer< Blockquote > bq( new Blockquote() );

		parse( stream, bq, doc, linksToParse, workingPath, fileName, collectRefLinks );

		if( !bq->isEmpty() )
			parent->appendItem( bq );

		if( i < fr.size() )
		{
			tmp = fr.sliced( i );

			StringListStream stream( tmp );

			parse( stream, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks );
		}
	}
}

namespace /* anonymous */ {

inline bool
isListItemAndNotNested( const QString & s, qsizetype indent )
{
	qsizetype p = skipSpaces( 0, s );

	if( p >= indent || p == s.size() )
		return false;

	bool space = false;

	if( p + 1 >= s.size() )
		space = true;
	else
		space = s[ p + 1 ].isSpace();

	if( p < 4 )
	{
		if( s[ p ] == c_42 && space )
			return true;
		else if( s[ p ] == c_45 && space )
			return true;
		else if( s[ p ] == c_43 && space )
			return true;
		else
			return isOrderedList( s );
	}
	else
		return false;
}

inline std::pair< qsizetype, qsizetype >
calculateIndent( const QString & s, qsizetype p )
{
	return { 0, skipSpaces( p, s ) };
}

inline std::tuple< bool, qsizetype, QChar >
listItemData( const QString & s )
{
	qsizetype p = skipSpaces( 0, s );

	if( p == s.size() )
		return { false, 0, QChar() };

	bool space = false;

	if( p + 1 >= s.size() )
		space = true;
	else
		space = s[ p + 1 ].isSpace();

	if( p < 4 )
	{
		if( s[ p ] == c_42 && space )
			return { true, p + 2, c_42 };
		else if( s[ p ] == c_45 && space )
			return { true, p + 2, c_45 };
		else if( s[ p ] == c_43 && space )
			return { true, p + 2, c_43 };
		else
		{
			int d = 0, l = 0;
			QChar c;

			if( isOrderedList( s, &d, &l, &c ) )
				return { true, p + l + 2, c };
			else
				return { false, 0, QChar() };
		}
	}
	else
		return { false, 0, QChar() };
}

} /* namespace anonymous */

void
Parser::parseList( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks, RawHtmlBlock & html )
{
	for( auto it = fr.begin(), last = fr.end(); it != last; ++it )
		it->replace( c_9, QLatin1String( "    " ) );

	const auto p = skipSpaces( 0, fr.first() );

	if( p != fr.first().length() )
	{
		QSharedPointer< List > list( new List );

		QStringList listItem;
		auto it = fr.begin();
		listItem.append( *it );
		++it;

		bool ok = false;
		qsizetype indent = 0;
		QChar marker;

		std::tie( ok, indent, marker ) = listItemData( listItem.first() );

		bool updateIndent = false;

		for( auto last = fr.end(); it != last; ++it )
		{
			if( updateIndent )
			{
				std::tie( ok, indent, marker ) = listItemData( *it );
				updateIndent = false;
			}

			const auto ns = skipSpaces( 0, *it );

			if( isHorizontalLine( it->sliced( ns ) ) && !listItem.isEmpty() &&
				( ns == indent ? !isH2( it->sliced( ns ) ) : true ) )
			{
				updateIndent = true;

				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
					collectRefLinks, html );
				listItem.clear();

				if( !list->isEmpty() )
					parent->appendItem( list );

				list.reset( new List );

				if( !collectRefLinks )
					doc->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

				continue;
			}
			else if( isListItemAndNotNested( *it, indent ) && !listItem.isEmpty() )
			{
				QChar tmpMarker;
				std::tie( ok, indent, tmpMarker ) = listItemData( *it );

				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
					collectRefLinks, html );
				listItem.clear();

				if( tmpMarker != marker )
				{
					if( !list->isEmpty() )
						parent->appendItem( list );

					list.reset( new List );

					marker = tmpMarker;
				}
			}

			listItem.append( *it );
		}

		if( !listItem.isEmpty() )
			parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
				collectRefLinks, html );

		if( !list->isEmpty() )
			parent->appendItem( list );
	}
}

void
Parser::parseListItem( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks, RawHtmlBlock & html )
{
	QSharedPointer< ListItem > item( new ListItem() );

	int i = 0;

	if( isOrderedList( fr.first(), &i ) )
	{
		item->setListType( ListItem::Ordered );
		item->setStartNumber( i );
	}
	else
		item->setListType( ListItem::Unordered );

	if( item->listType() == ListItem::Ordered )
		item->setOrderedListPreState( i == 1 ? ListItem::Start : ListItem::Continue );

	QStringList data;

	auto it = fr.begin();
	++it;

	int pos = 1;

	qsizetype indent = 0;
	bool ok = false;

	std::tie( ok, indent, std::ignore ) = listItemData( fr.first() );

	const auto firstNonSpacePos = calculateIndent( fr.first(), indent ).second;
	if( firstNonSpacePos - indent < 4 ) indent = firstNonSpacePos;

	if( indent < fr.first().length() )
		data.append( fr.first().right( fr.first().length() - indent ) );

	for( auto last = fr.end(); it != last; ++it, ++pos )
	{
		std::tie( ok, std::ignore, std::ignore ) = listItemData( *it );

		if( ok )
		{
			StringListStream stream( data );

			parse( stream, item, doc, linksToParse, workingPath, fileName,
				collectRefLinks );

			data.clear();

			QStringList nestedList;
			nestedList << *it;
			++it;

			for( ; it != last; ++it )
			{
				const auto ns = skipSpaces( 0, *it );
				std::tie( ok, std::ignore, std::ignore ) = listItemData( *it );

				if( ok || ns > indent || ns == it->length() )
					nestedList << *it;
				else
					break;
			}

			for( auto it = nestedList.begin(), last = nestedList.end(); it != last; ++it )
			{
				if( it->startsWith( QString( indent, c_32 ) ) )
					*it = it->sliced( indent );
			}

			parseList( nestedList, item, doc, linksToParse, workingPath, fileName,
				collectRefLinks, html );

			for( ; it != last; ++it )
			{
				if( it->startsWith( QString( indent, c_32 ) ) )
					*it = it->sliced( indent );

				data.append( *it );
			}

			break;
		}
		else
		{
			if( it->startsWith( QString( indent, c_32 ) ) )
				*it = it->sliced( indent );

			data.append( *it );
		}
	}

	if( !data.isEmpty() )
	{
		StringListStream stream( data );

		parse( stream, item, doc, linksToParse, workingPath, fileName, collectRefLinks );
	}

	if( !item->isEmpty() )
		parent->appendItem( item );
}

void
Parser::parseCode( QStringList & fr, QSharedPointer< Block > parent,
	bool collectRefLinks, int indent )
{
	if( !collectRefLinks )
	{
		const auto i = skipSpaces( 0, fr.first() );

		if( i != fr.first().length() )
			indent += i;

		QString syntax;
		isStartOfCode( fr.constFirst(), &syntax );

		fr.removeFirst();
		fr.removeLast();

		parseCodeIndentedBySpaces( fr, parent, collectRefLinks, indent, syntax );
	}
}

void
Parser::parseCodeIndentedBySpaces( QStringList & fr, QSharedPointer< Block > parent,
	bool collectRefLinks, int indent, const QString & syntax )
{
	if( !collectRefLinks )
	{
		QString code;

		for( const auto & l : qAsConst( fr ) )
		{
			const auto ns = skipSpaces( 0, l );

			code.append( ( indent > 0 ? l.right( l.length() - ( ns < indent ? ns : indent ) ) + c_10 :
				l + c_10 ) );
		}

		if( !code.isEmpty() )
			code = code.left( code.length() - 1 );

		QSharedPointer< Code > codeItem( new Code( code ) );
		codeItem->setSyntax( syntax );
		parent->appendItem( codeItem );
	}
}

bool
fileExists( const QString & fileName, const QString & workingPath )
{
	return QFileInfo::exists( workingPath + fileName );
}

} /* namespace MD */
