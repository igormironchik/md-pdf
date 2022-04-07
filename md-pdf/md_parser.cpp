
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

// Qt include.
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QRegularExpression>


namespace MD {

//
// ParserException
//

ParserException::ParserException( const QString & reason )
	:	std::logic_error( "" )
	,	m_what( reason )
{
}

const QString &
ParserException::reason() const noexcept
{
	return m_what;
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
				fi.absolutePath() + QStringLiteral( "/" ), fi.fileName(), true, false, true );

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

inline bool
isOrderedList( const QString & s, int * num = nullptr )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	qsizetype dp = p;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isDigit() )
			break;
	}

	if( dp != p && p < s.size() )
	{
		if( num )
			*num = s.mid( dp, p - dp ).toInt();

		if( s[ p ] == c_46 || s[ p ] == c_41 )
		{
			if( ++p < s.size() && s[ p ].isSpace() )
				return true;
		}
	}

	return false;
}

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


inline bool
isStartOfCode( QStringView str, QString * syntax = nullptr )
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
	else
		++p;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	return p;
}

inline bool
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

} /* namespace anonymous */

Parser::BlockType
Parser::whatIsTheLine( QString & str, bool inList, qsizetype * indent, bool calcIndent ) const
{
	str.replace( c_9, QString( 4, c_32 ) );

	const auto first = skipSpaces( 0, str );

	if( first < str.length() )
	{
		auto s = QStringView( str ).sliced( first );

		if( s.startsWith( c_62 ) )
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
			if( ( ( s.startsWith( c_45 ) ||
				s.startsWith( c_43 ) ||
				s.startsWith( c_42 ) ) && s.length() > 1 && s[ 1 ].isSpace() ) ||
					isOrderedList( str ) )
			{
				return BlockType::List;
			}
			else if( str.startsWith( QString( ( indent ? *indent : 4 ), c_32 ) ) )
			{
				if( str.startsWith( QString( ( indent ? *indent : 4 ), c_32 ) +
					QLatin1String( "    " ) ) )
				{
					return BlockType::CodeIndentedBySpaces;
				}
			}
		}
		else
		{
			const auto orderedList = isOrderedList( str );

			if( ( ( s.startsWith( c_45 ) ||
				s.startsWith( c_43 ) ||
				s.startsWith( c_42 ) ) && s.length() > 1 && s[ 1 ].isSpace() ) ||
					orderedList )
			{
				if( calcIndent && indent )
					*indent = posOfListItem( str, orderedList );

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
	const QString & workingPath, const QString & fileName, bool collectRefLinks )
{
	switch( whatIsTheLine( fr.first() ) )
	{
		case BlockType::Text :
			parseText( fr, parent, doc, linksToParse,
				workingPath, fileName, collectRefLinks );
			break;

		case BlockType::Blockquote :
			parseBlockquote( fr, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks );
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
			parseList( fr, parent, doc, linksToParse, workingPath, fileName,
				collectRefLinks );
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

inline bool
isColumnAlignment( const QString & s )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

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

inline bool
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

} /* namespace anonymous */

void
Parser::parseText( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	if( isFootnote( fr.first() ) )
		parseFootnote( fr, parent, doc, linksToParse, workingPath, fileName,
			collectRefLinks );
	else if( isTableHeader( fr.first() ) && fr.size() > 1 && isTableAlignment( fr[ 1 ] ) )
		parseTable( fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks );
	else
		parseParagraph( fr, parent, doc, linksToParse,
			workingPath, fileName, collectRefLinks );
}

namespace /* anonymous */ {

// Read text of the link. I.e. in [...]
QString readLinkText( int & i, const QString & line )
{
	const int length = line.length();
	QString t;
	bool first = true;
	bool skipped = false;

	while( i < length )
	{
		if( !first && !skipped && line[ i - 1 ] == c_92 )
		{
			t.append( line[ i ] );

			skipped = true;
			first = false;
			++i;

			continue;
		}
		else if( line[ i ] != c_93 && line[ i ] != c_92 )
			t.append( line[ i ] );
		else if( line[ i ] == c_93 )
			break;

		first = false;
		skipped = false;
		++i;
	}

	++i;

	if( i - 1 < length && line[ i - 1 ] == c_93 )
		return t;
	else
		return QString();
}; // readLinkText

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

		parseFormattedTextLinksImages( tmp, p, doc, linksToParse, workingPath, fileName,
			false, false );

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
Parser::parseFootnote( QStringList & fr, QSharedPointer< Block >,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	if( !fr.isEmpty() )
	{
		QSharedPointer< Footnote > f( new Footnote() );

		QString line = fr.first();
		fr.removeFirst();

		int pos = skipSpaces( 0, line );

		if( pos > 0 )
			line = line.sliced( pos );

		if( line.startsWith( QLatin1String( "[^" ) ) )
		{
			pos = 1;

			QString id = readLinkText( pos, line );

			if( !id.isEmpty() && line[ pos ] == c_58 )
			{
				++pos;

				line = line.sliced( pos );

				for( auto it = fr.begin(), last = fr.end(); it != last; ++it )
				{
					if( it->startsWith( QLatin1String( "    " ) ) )
						*it = it->mid( 4 );
					else if( it->startsWith( c_9 ) )
						*it = it->mid( 1 );
				}

				fr.prepend( line );

				StringListStream stream( fr );

				parse( stream, f, doc, linksToParse, workingPath, fileName, collectRefLinks, false );

				if( !f->isEmpty() )
					doc->insertFootnote( QString::fromLatin1( "#" ) + id +
						QStringLiteral( "/" ) + workingPath + fileName, f );
			}
		}
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

					parseFormattedTextLinksImages( fragment, p, doc,
						linksToParse, workingPath, fileName, collectRefLinks, false );

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
	bool collectRefLinks )
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

			QSharedPointer< Heading > h;
			QSharedPointer< Paragraph > p;

			if( !collectRefLinks )
			{
				h.reset( new Heading );
				h->setLevel( lvl );

				p.reset( new Paragraph );

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
					workingPath, fileName, collectRefLinks, true );
			}

			fr.remove( 0, i - horLines + 1 );

			if( !collectRefLinks )
			{
				if( !p->items().isEmpty() && p->items().at( 0 )->type() == ItemType::Paragraph )
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
				collectRefLinks, false );
		}
		else
		{
			QSharedPointer< Paragraph > p( new Paragraph );

			parseFormattedTextLinksImages( fr, p, doc, linksToParse, workingPath, fileName,
				collectRefLinks, false );

			if( !p->isEmpty() && !collectRefLinks )
			{
				for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
					parent->appendItem( (*it) );
			}
		}
	}
}

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
		Italic1,
		// _
		Italic2,
		// **
		Bold1,
		// __
		Bold2,
		// ***
		BoldItalic1,
		// ___
		BoldItalic2,
		// _**
		BoldItalic3Open,
		// **_
		BoldItalic3Close,
		// __*
		BoldItalic4Open,
		// *__
		BoldItalic4Close,
		// `
		InlineCode,
		// <
		Less,
		// >
		Greater,
		HorizontalLine,
		Unknown
	}; // enum DelimiterType

	DelimiterType m_type;
	qsizetype m_line;
	qsizetype m_pos;
	qsizetype m_len;
	bool m_spaceBefore;
	bool m_spaceAfter;
	bool m_isWordBefore;
}; // struct Delimiter

inline Delimiter::DelimiterType
styleType( const QString & s )
{
	if( s == QStringLiteral( "*" ) )
		return Delimiter::Italic1;
	else if( s == QStringLiteral( "_" ) )
		return Delimiter::Italic2;
	else if( s == QStringLiteral( "**" ) )
		return Delimiter::Bold1;
	else if( s == QStringLiteral( "__" ) )
		return Delimiter::Bold2;
	else if( s == QStringLiteral( "***" ) )
		return Delimiter::BoldItalic1;
	else if( s == QStringLiteral( "___" ) )
		return Delimiter::BoldItalic2;
	else if( s == QStringLiteral( "_**" ) )
		return Delimiter::BoldItalic3Open;
	else if( s == QStringLiteral( "**_" ) )
		return Delimiter::BoldItalic3Close;
	else if( s == QStringLiteral( "__*" ) )
		return Delimiter::BoldItalic4Open;
	else if( s == QStringLiteral( "*__" ) )
		return Delimiter::BoldItalic4Close;
	else
		return Delimiter::Unknown;
}

using Delims = QList< Delimiter >;

inline Delims
collectDelimiters( const QStringList & fr )
{
	Delims d;

	for( qsizetype line = 0; line < fr.size(); ++line )
	{
		const QString & str = fr.at( line );
		const auto p = skipSpaces( 0, str );
		const QStringView s( str.sliced( p ) );

		if( isHorizontalLine( s ) && p < 4 )
			d.push_back( { Delimiter::HorizontalLine, line, 0, str.length(), false, false, false } );
		else
		{
			bool backslash = false;
			bool space = false;
			bool word = false;

			for( qsizetype i = 0; i < str.size(); ++i )
			{
				bool now = false;

				if( str[ i ] == c_92 && !backslash )
				{
					backslash = true;
					now = true;
				}
				else if( str[ i ] == c_32 && !backslash )
				{
					space = true;
					now = true;
				}
				else
				{
					if( !backslash )
					{
						// * or _
						if( str[ i ] == c_95 || str[ i ] == c_42 )
						{
							QString style;

							while( i < str.length() && ( str[ i ] == c_95 || str[ i ] == c_42 ) )
							{
								style.append( str[ i ] );
								++i;
							}

							if( style.length() <= 3 )
							{
								const auto dt = styleType( style );

								if( dt != Delimiter::Unknown )
								{
									const bool spaceAfter =
										( i < str.length() ? str[ i ] == c_32 : false );

									d.push_back( { dt, line, i - style.length(), style.length(),
										space, spaceAfter, word } );

									word = false;
								}
								else
									word = true;
							}
							else
								word = true;

							--i;
						}
						// ~
						else if( str[ i ] == c_126 )
						{
							QString style;

							while( i < str.length() && str[ i ] == c_126 )
							{
								style.append( str[ i ] );
								++i;
							}

							if( style.length() == 2 )
							{
								const bool spaceAfter =
									( i < str.length() ? str[ i ] == c_32 : false );

								d.push_back( { Delimiter::Strikethrough, line, i - style.length(),
									style.length(), space, spaceAfter, word } );

								word = false;
							}
							else
								word = true;

							--i;
						}
						// [
						else if( str[ i ] == c_91 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::SquareBracketsOpen, line, i, 1,
								space, spaceAfter, word } );

							word = false;
						}
						// !
						else if( str[ i ] == c_33 )
						{
							if( i + 1 < str.length() )
							{
								if( str[ i + 1 ] == c_91 )
								{
									const bool spaceAfter =
										( i < str.length() ? str[ i ] == c_32 : false );

									d.push_back( { Delimiter::ImageOpen, line, i, 2,
										space, spaceAfter, word } );

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
						else if( str[ i ] == c_40 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::ParenthesesOpen, line, i, 1,
								space, spaceAfter, word } );

							word = false;
						}
						// ]
						else if( str[ i ] == c_93 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::SquareBracketsClose, line, i, 1,
								space, spaceAfter, word } );

							word = false;
						}
						// )
						else if( str[ i ] == c_41 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::ParenthesesClose, line, i, 1,
								space, spaceAfter, word } );

							word = false;
						}
						// <
						else if( str[ i ] == c_60 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::Less, line, i, 1,
								space, spaceAfter, word } );

							word = false;
						}
						// >
						else if( str[ i ] == c_62 )
						{
							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::Greater, line, i, 1,
								space, spaceAfter, word } );

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

							const bool spaceAfter =
								( i < str.length() ? str[ i ] == c_32 : false );

							d.push_back( { Delimiter::InlineCode, line, i - code.length(),
								code.length(), space, spaceAfter, word } );

							word = false;

							--i;
						}
						else
							word = true;
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

	qsizetype line = 0;
	qsizetype pos = 0;
	TextOptions opts = TextWithoutFormat;
	QVector< Delimiter::DelimiterType > styles;
}; // struct TextParsingOpts

void
parseFormattedText( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak );

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
makeTextObject( const QString & text, const TextOptions & opts,
	bool spaceBefore, bool spaceAfter, QSharedPointer< Block > parent )
{
	const auto s = text.simplified();

	if( !s.isEmpty() )
	{
		QSharedPointer< Text > t( new Text() );
		t->setText( s );
		t->setOpts( opts );
		t->setSpaceBefore( spaceBefore );
		t->setSpaceAfter( spaceAfter );

		parent->appendItem( t );
	}
}

inline void
makeTextObjectWithLineBreak( const QString & text, const TextOptions & opts,
	bool spaceBefore, bool spaceAfter, QSharedPointer< Block > parent )
{
	makeTextObject( text, opts, spaceBefore, true, parent );

	QSharedPointer< Item > hr( new LineBreak );
	parent->appendItem( hr );
}

inline QString
removeBackslashes( const QString & s )
{
	static const QString canBeEscaped = QStringLiteral( "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~" );

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
		else if( canBeEscaped.contains( s[ i ] ) && backslash )
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

	bool lineBreak = ( !po.ignoreLineBreak && ( po.line == lastLine ?
		( lastPos == po.fr.at( po.line ).size() && isLineBreak( po.fr.at( po.line ) ) ) :
		isLineBreak( po.fr.at( po.line ) ) ) );

	// makeTOWLB
	auto makeTOWLB = [&] () {
		makeTextObjectWithLineBreak( text, po.opts, spaceBefore, true, po.parent );

		text.clear();

		spaceBefore = true;
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
			lineBreak = ( !po.ignoreLineBreak && isLineBreak( po.fr.at( po.line ) ) );

			const auto s = ( lineBreak ?
				removeLineBreak( po.fr.at( po.line ) ) : po.fr.at( po.line ) );
			text.append( doNotEscape ? s : removeBackslashes( s ) );

			text.append( c_32 );

			if( lineBreak )
				makeTOWLB();
		}

		lineBreak = ( !po.ignoreLineBreak && lastPos == po.fr.at( po.line ).size() &&
			isLineBreak( po.fr.at( po.line ) ) );

		if( !lineBreak )
		{
			const auto s = po.fr.at( po.line ).sliced( 0, lastPos );
			text.append( doNotEscape ? s : removeBackslashes( s ) );
		}
		else
			makeTOWLB();
	}

	po.pos = lastPos;

	makeTextObject( text, po.opts, spaceBefore,
		( po.pos > 0 ? po.fr.at( po.line )[ po.pos - 1 ].isSpace() : true ), po.parent );
}

inline Delims::const_iterator
checkForAutolinkHtml( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto nit = std::find_if( std::next( it ), last,
		[] ( const auto & d ) { return ( d.m_type == Delimiter::Greater ); } );

	if( nit != last && nit->m_line == it->m_line )
	{
		if( !po.collectRefLinks )
		{
			const auto url = po.fr.at( po.line ).sliced( po.pos + 1, nit->m_pos - po.pos - 1 );

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
				lnk->setTextOptions( po.opts );
				po.parent->appendItem( lnk );
			}
			else
				makeText( nit->m_line, nit->m_pos + nit->m_len, po, true );
		}

		po.pos = nit->m_pos + nit->m_len;
		po.line = nit->m_line;

		return nit;
	}
	else if( !po.collectRefLinks )
		makeText( it->m_line, it->m_pos + it->m_len, po );

	po.pos = it->m_pos + it->m_len;
	po.line = it->m_line;

	return it;
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

	if( c.front().isSpace() && c.back().isSpace() && skipSpaces( 0, c ) < c.size() )
	{
		c.remove( 0, 1 );
		c.remove( c.size() - 1, 1 );
	}

	if( !c.isEmpty() )
		po.parent->appendItem( QSharedPointer< Code >( new Code( c, true ) ) );
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
		if( it->m_type == Delimiter::InlineCode && it->m_len == len )
		{
			if( !po.collectRefLinks )
			{
				makeText( start->m_line, start->m_pos, po );

				po.pos = start->m_pos + start->m_len;

				makeInlineCode( it->m_line, it->m_pos, po );
			}

			po.line = it->m_line;
			po.pos = it->m_pos + it->m_len;

			return it;
		}
	}

	if( !po.collectRefLinks )
		makeText( start->m_line, start->m_pos + start->m_len, po );

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

			return { removeBackslashes( po.fr.at( start->m_line ).sliced( p, n ).simplified() ),
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

				return { removeBackslashes( text.simplified() ), it };
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
				it = checkForAutolinkHtml( it, last, po );
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
	QString u = removeBackslashes( url );

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

	QSharedPointer< Link > link( new Link );
	link->setUrl( u );
	link->setTextOptions( po.opts );

	QStringList tmp;
	tmp << text;

	QSharedPointer< Paragraph > p( new Paragraph );

	parseFormattedText( tmp, p, po.doc,
		po.linksToParse, po.workingPath,
		po.fileName, po.collectRefLinks, true );

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
					{
						if( !po.collectRefLinks && !doNotCreateTextOnFail )
							makeText( lastLine, lastPos, po );

						return {};
					}
						break;

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
	const auto u = QString::fromLatin1( "#" ) + text.simplified().toLower();
	const auto url = u + QStringLiteral( "/" ) + po.workingPath + po.fileName;

	if( po.doc->labeledLinks().contains( url ) )
	{
		if( !po.collectRefLinks )
		{
			const auto link = makeLink( u, ( linkText.isEmpty() ? text : linkText ), po,
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

	parseFormattedText( tmp, p, po.doc,
		po.linksToParse, po.workingPath,
		po.fileName, po.collectRefLinks, true );

	if( !p->isEmpty() )
	{
		if( p->items().size() == 1 && p->items().at( 0 )->type() == ItemType::Paragraph )
			img->setP( p->items().at( 0 ).staticCast< Paragraph > () );
	}

	img->setText( text );

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
	const auto url = QString::fromLatin1( "#" ) + text.simplified().toLower() +
		QStringLiteral( "/" ) + po.workingPath + po.fileName;

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

inline std::tuple< qsizetype, qsizetype, bool, QString >
readLinkDestination( qsizetype line, qsizetype pos, const QStringList & fr )
{
	skipSpacesUpTo1Line( line, pos, fr );

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
					return { line, pos, false, {} };
				else if( !backslash && s[ pos ] == c_62 )
				{
					++pos;
					break;
				}
				else
					dest.append( s[ pos ] );

				if( !now )
					backslash = false;

				++pos;
			}

			return { line, pos, true, dest };
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
				else if( !backslash && s[ pos ].isSpace() )
				{
					if( !pc )
						return { line, pos, true, dest };
					else
						return { line, pos, false, {} };
				}
				else if( !backslash && s[ pos ] == c_40 )
				{
					++pc;
					dest.append( s[ pos ] );
				}
				else if( !backslash && s[ pos ] == c_41 )
				{
					if( !pc )
						return { line, pos, true, dest };
					else
					{
						dest.append( s[ pos ] );
						--pc;
					}
				}
				else
					dest.append( s[ pos ] );

				if( !now )
					backslash = false;

				++pos;
			}

			return { line, pos, true, dest };
		}
	}
	else
		return { line, pos, false, {} };
}

inline std::tuple< qsizetype, qsizetype, bool, QString >
readLinkTitle( qsizetype line, qsizetype pos, const QStringList & fr )
{
	const auto space = ( pos < fr.at( line ).size() ? fr.at( line )[ pos ].isSpace() : true );

	skipSpacesUpTo1Line( line, pos, fr );

	if( pos >= fr.at( line ).size() )
		return { line, pos, false, {} };

	const auto sc = fr.at( line )[ pos ];

	if( sc != c_34 && sc != c_39 && sc != c_40 )
		return { line, pos, false, {} };
	else if( !space )
		return { line, pos, false, {} };

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
			return { line, ++pos, true, title };
		else if( sc == c_40 && fr.at( line )[ pos ] == c_40 && !backslash )
			return { line, pos, false, {} };
		else if( sc != c_40 && fr.at( line )[ pos ] == sc && !backslash )
			return { line, ++pos, true, title };
		else
			title.append( fr.at( line )[ pos ] );

		if( !now )
			backslash = false;

		++pos;

		if( pos == fr.at( line ).size() )
			skipSpacesUpTo1Line( line, pos, fr );
	}

	return { line, pos, false, {} };
}

inline std::tuple< QString, QString, Delims::const_iterator, bool >
checkForInlineLink( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	qsizetype p = it->m_pos + it->m_len;
	qsizetype l = it->m_line;
	bool ok = false;
	QString dest, title;

	std::tie( l, p, ok, dest ) = readLinkDestination( l, p, po.fr );

	if( !ok )
		return { {}, {}, it, false };

	std::tie( l, p, ok, title ) = readLinkTitle( l, p, po.fr );

	skipSpacesUpTo1Line( l, p, po.fr );

	if( !ok && po.fr.at( l )[ p ] != c_41 )
		return { {}, {}, it, false };

	for( ; it != last; ++it )
	{
		if( it->m_line == l && it->m_pos == p )
		{
			po.line = it->m_line;
			po.pos = it->m_pos + it->m_len;

			return { dest, title, it, true };
		}
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

	std::tie( l, p, ok, dest ) = readLinkDestination( l, p, po.fr );

	if( !ok )
		return { {}, {}, it, false };

	std::tie( l, p, ok, title ) = readLinkTitle( l, p, po.fr );

	p = skipSpaces( p, po.fr.at( l ) );

	if( !ok && p < po.fr.at( l ).size() )
		return { {}, {}, it, false };

	for( ; it != last; ++it )
	{
		if( it->m_line > l || ( it->m_line == l && it->m_pos > p ) )
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
				else if( createShortcutImage( text.simplified().toLower(),
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
					if( createShortcutImage( label.simplified().toLower(),
							po, start->m_line, start->m_pos + start->m_len,
							lit, text, true ) )
					{
						return lit;
					}
					else if( createShortcutImage( text.simplified().toLower(),
								po, start->m_line, start->m_pos + start->m_len,
								it, {}, false ) )
					{
						if( label.isEmpty() )
						{
							po.line = lit->m_line;
							po.pos = lit->m_pos + lit->m_line;

							return lit;
						}
						else
							return it;
					}
				}
				else if( createShortcutImage( text.simplified().toLower(),
							po, start->m_line, start->m_pos + start->m_len,
							it, {}, false ) )
				{
					return it;
				}
			}
			// Shortcut
			else if( createShortcutImage( text.simplified().toLower(),
						po, start->m_line, start->m_pos + start->m_len,
						it, {}, false ) )
			{
				return it;
			}
		}
		// Shortcut
		else if( createShortcutImage( text.simplified().toLower(),
					po, start->m_line, start->m_pos + start->m_len,
					it, {}, false ) )
		{
			return it;
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
					new FootnoteRef( QStringLiteral( "#" ) + text.simplified().toLower() +
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
				if( po.line == 0 && ns < 4 && start->m_pos == ns )
				{
					QString url, title;
					Delims::const_iterator iit;
					bool ok;

					std::tie( url, title, iit, ok ) = checkForRefLink( it, last, po );

					if( ok )
					{
						const auto label = QString::fromLatin1( "#" ) + text.simplified().toLower() +
							QStringLiteral( "/" ) + po.workingPath + po.fileName;

						QSharedPointer< Link > link( new Link );

						if( QUrl( url ).isRelative() )
						{
							if( fileExists( url, po.workingPath ) )
								url = QFileInfo( po.workingPath + url ).absoluteFilePath();
						}

						link->setUrl( removeBackslashes( url ) );

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
					const auto link = makeLink( url, text, po, false,
						start->m_line, start->m_pos + start->m_len );

					if( !link.isNull() )
					{
						if( !po.collectRefLinks )
							po.parent->appendItem( link );

						return iit;
					}
					else
						return it;
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
					if( createShortcutLink( label.simplified(),
							po, start->m_line, start->m_pos + start->m_len,
							lit, text, true ) )
					{
						return lit;
					}
					else if( createShortcutLink( text.simplified(),
								po, start->m_line, start->m_pos + start->m_len,
								it, {}, false ) )
					{
						if( label.isEmpty() )
						{
							po.line = lit->m_line;
							po.pos = lit->m_pos + lit->m_line;

							return lit;
						}
						else
							return it;
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
			else if( createShortcutLink( text.simplified(),
						po, start->m_line, start->m_pos + start->m_len,
						it, {}, false ) )
			{
				return it;
			}
		}
		// Shortcut
		else if( createShortcutLink( text.simplified(),
					po, start->m_line, start->m_pos + start->m_len,
					it, {}, false ) )
		{
			return it;
		}
	}
	else if( !po.collectRefLinks )
		makeText( start->m_line, start->m_pos + start->m_len, po );

	return start;
}

inline bool
isClosingStyle( const QVector< Delimiter::DelimiterType > & styles,
	Delimiter::DelimiterType s )
{
	switch( s )
	{
		case Delimiter::Strikethrough :
		case Delimiter::Italic1 :
		case Delimiter::Italic2 :
		case Delimiter::Bold1 :
		case Delimiter::Bold2 :
		case Delimiter::BoldItalic1 :
		case Delimiter::BoldItalic2 :
			return styles.contains( s );

		case Delimiter::BoldItalic3Close :
			return styles.contains( Delimiter::BoldItalic3Open );

		case Delimiter::BoldItalic4Close :
			return styles.contains( Delimiter::BoldItalic4Open );

		default :
			return false;
	}
}

inline void
closeStyle( QVector< Delimiter::DelimiterType > & styles,
	Delimiter::DelimiterType s )
{
	switch( s )
	{
		case Delimiter::Strikethrough :
		case Delimiter::Italic1 :
		case Delimiter::Italic2 :
		case Delimiter::Bold1 :
		case Delimiter::Bold2 :
		case Delimiter::BoldItalic1 :
		case Delimiter::BoldItalic2 :
			styles.removeOne( s );
			break;

		case Delimiter::BoldItalic3Close :
			styles.removeOne( Delimiter::BoldItalic3Open );
			break;

		case Delimiter::BoldItalic4Close :
			styles.removeOne( Delimiter::BoldItalic4Open );
			break;

		default :
			break;
	}
}

inline void
setStyle( TextOptions & opts, Delimiter::DelimiterType s, bool on )
{
	switch( s )
	{
		case Delimiter::Strikethrough :
			opts.setFlag( StrikethroughText, on );
			break;

		case Delimiter::Italic1 :
		case Delimiter::Italic2 :
			opts.setFlag( ItalicText, on );
			break;

		case Delimiter::Bold1 :
		case Delimiter::Bold2 :
			opts.setFlag( BoldText, on );
			break;

		case Delimiter::BoldItalic1 :
		case Delimiter::BoldItalic2 :
		case Delimiter::BoldItalic3Open :
		case Delimiter::BoldItalic4Open :
		case Delimiter::BoldItalic3Close :
		case Delimiter::BoldItalic4Close :
			opts.setFlag( BoldText, on );
			opts.setFlag( ItalicText, on );
			break;

		default :
			break;
	}
}

inline bool
isClosingStyle( Delimiter::DelimiterType open,
	Delimiter::DelimiterType close )
{
	switch( open )
	{
		case Delimiter::Strikethrough :
		case Delimiter::Italic1 :
		case Delimiter::Italic2 :
		case Delimiter::Bold1 :
		case Delimiter::Bold2 :
		case Delimiter::BoldItalic1 :
		case Delimiter::BoldItalic2 :
			return ( open == close );

		case Delimiter::BoldItalic3Open :
			return ( close == Delimiter::BoldItalic3Close );

		case Delimiter::BoldItalic4Open :
			return ( close == Delimiter::BoldItalic4Close );

		default :
			return false;
	}
}

inline bool
isStyleClosed( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	const auto open = it->m_type;

	const qsizetype line = po.line, pos = po.pos;
	const bool collectRefLinks = po.collectRefLinks;

	po.collectRefLinks = true;

	for( it = std::next( it ); it != last; ++it )
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
				it = checkForAutolinkHtml( it, last, po );
				break;

			case Delimiter::Strikethrough :
			case Delimiter::Italic1 :
			case Delimiter::Italic2 :
			case Delimiter::Bold1 :
			case Delimiter::Bold2 :
			case Delimiter::BoldItalic1 :
			case Delimiter::BoldItalic2 :
			case Delimiter::BoldItalic3Close :
			case Delimiter::BoldItalic4Close :
			{
				if( isClosingStyle( open, it->m_type ) )
				{
					po.collectRefLinks = collectRefLinks;
					po.line = line;
					po.pos = pos;

					return true;
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

	po.collectRefLinks = collectRefLinks;
	po.line = line;
	po.pos = pos;

	return false;
}

inline Delims::const_iterator
checkForStyle( Delims::const_iterator it, Delims::const_iterator last,
	TextParsingOpts & po )
{
	if( isClosingStyle( po.styles, it->m_type ) )
	{
		closeStyle( po.styles, it->m_type );
		setStyle( po.opts, it->m_type, false );

		po.pos = it->m_pos + it->m_len;
		po.line = it->m_line;
	}
	else
	{
		switch( it->m_type )
		{
			case Delimiter::Strikethrough :
			case Delimiter::Italic1 :
			case Delimiter::Italic2 :
			case Delimiter::Bold1 :
			case Delimiter::Bold2 :
			case Delimiter::BoldItalic1 :
			case Delimiter::BoldItalic2 :
			case Delimiter::BoldItalic3Open :
			case Delimiter::BoldItalic4Open :
			{
				if( isStyleClosed( it, last, po ) )
				{
					setStyle( po.opts, it->m_type, true );
					po.styles.append( it->m_type );

					po.pos = it->m_pos + it->m_len;
					po.line = it->m_line;
				}
				else if( !po.collectRefLinks )
					makeText( it->m_line, it->m_pos + it->m_len, po );
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

	return it;
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
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak )

{
	if( fr.isEmpty() )
		return;

	QSharedPointer< Paragraph > p( new Paragraph );

	const auto delims = collectDelimiters( fr );

	TextParsingOpts po = { fr, p, doc, linksToParse, workingPath,
		fileName, collectRefLinks, ignoreLineBreak };

	for( auto it = delims.cbegin(), last = delims.cend(); it != last; ++it )
	{
		if( it->m_line > po.line || it->m_pos > po.pos )
		{
			if( !collectRefLinks )
				makeText( it->m_line, it->m_pos, po );
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
				it = checkForAutolinkHtml( it, last, po );
				break;

			case Delimiter::Strikethrough :
			case Delimiter::Italic1 :
			case Delimiter::Italic2 :
			case Delimiter::Bold1 :
			case Delimiter::Bold2 :
			case Delimiter::BoldItalic1 :
			case Delimiter::BoldItalic2 :
			case Delimiter::BoldItalic3Open :
			case Delimiter::BoldItalic4Open :
			case Delimiter::BoldItalic3Close :
			case Delimiter::BoldItalic4Close :
				it = checkForStyle( it, last, po );
				break;

			case Delimiter::InlineCode :
				it = checkForInlineCode( it, last, po );
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
			}
				break;
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

bool
Parser::parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool collectRefLinks, bool ignoreLineBreak )

{
	parseFormattedText( fr, parent, doc, linksToParse, workingPath, fileName,
		collectRefLinks, ignoreLineBreak );

	return true;
}

void
Parser::parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	const int pos = fr.first().indexOf( c_62 );

	if( pos > -1 )
	{
		qsizetype i = 0;

		bool horLine = false;
		qsizetype j = i;

		for( auto it = fr.begin(), last = fr.end(); it != last; ++it, ++i )
		{
			const auto first = skipSpaces( 0, *it );

			if( first < 4 && isHorizontalLine( (*it).sliced( first ) ) )
			{
				horLine = true;
				break;
			}

			if( isH1( *it ) )
			{
				const auto p = (*it).indexOf( c_35 );

				(*it).insert( p, c_92 );
			}

			*it = it->sliced( it->indexOf( c_62 ) + 1 );
		}

		QStringList tmp;

		for( ; j < i; ++j )
			tmp.append( fr.at( j ) );

		StringListStream stream( tmp );

		QSharedPointer< Blockquote > bq( new Blockquote() );

		parse( stream, bq, doc, linksToParse, workingPath, fileName, collectRefLinks, false );

		if( !bq->isEmpty() )
			parent->appendItem( bq );

		if( horLine )
		{
			if( !collectRefLinks )
				parent->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

			++i;

			if( i < fr.size() )
			{
				tmp = fr.sliced( i );

				StringListStream stream( tmp );

				parse( stream, parent, doc, linksToParse, workingPath, fileName,
					collectRefLinks, false );
			}
		}
	}
}

namespace /* anonymous */ {

inline bool
isListItemAndNotNested( const QString & s )
{
	qsizetype p = skipSpaces( 0, s );

	if( p > 0 )
		return false;

	if( p + 1 >= s.size() )
		return false;

	if( s[ p ] == c_42 && s[ p + 1 ].isSpace() )
		return true;
	else if( s[ p ] == c_45 && s[ p + 1 ].isSpace() )
		return true;
	else if( s[ p ] == c_43 && s[ p + 1 ].isSpace() )
		return true;
	else
		return isOrderedList( s );
}

} /* namespace anonymous */

void
Parser::parseList( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	for( auto it = fr.begin(), last  = fr.end(); it != last; ++it )
		it->replace( c_9, QLatin1String( "    " ) );

	const auto indent = skipSpaces( 0, fr.first() );

	if( indent != fr.first().length() )
	{
		QSharedPointer< List > list( new List );

		QStringList listItem;
		auto it = fr.begin();

		*it = it->right( it->length() - indent );

		listItem.append( *it );

		++it;

		for( auto last = fr.end(); it != last; ++it )
		{
			auto s = skipSpaces( 0, *it );
			s = ( s > indent ? indent : ( s != (*it).length() ? s : 0 ) );

			*it = it->right( it->length() - s );

			if( isHorizontalLine( *it ) && !listItem.isEmpty() )
			{
				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
					collectRefLinks );
				listItem.clear();

				if( !list->isEmpty() )
					parent->appendItem( list );

				list.reset( new List );

				if( !collectRefLinks )
					doc->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

				continue;
			}
			else if( isListItemAndNotNested( *it ) && !listItem.isEmpty() )
			{
				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
					collectRefLinks );
				listItem.clear();
			}

			listItem.append( *it );
		}

		if( !listItem.isEmpty() )
			parseListItem( listItem, list, doc, linksToParse, workingPath, fileName,
				collectRefLinks );

		if( !list->isEmpty() )
			parent->appendItem( list );
	}
}

namespace /* anonymous */ {

inline std::pair< qsizetype, qsizetype >
calculateIndent( const QString & s, qsizetype p )
{
	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	return { 0, p };
}

inline std::pair< qsizetype, qsizetype >
listItemData( const QString & s )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	if( p + 1 >= s.size() )
		return { -1, 0 };

	if( s[ p ] == c_42 && s[ p + 1 ].isSpace() )
		return { 0, p + 2 };
	else if( s[ p ] == c_45 && s[ p + 1 ].isSpace() )
		return { 0, p + 2 };
	else if( s[ p ] == c_43 && s[ p + 1 ].isSpace() )
		return { 0, p + 2 };
	else
	{
		int d = 0;

		if( isOrderedList( s, &d ) )
			return { 0, p + QString::number( d ).size() + 2 };
		else
			return { -1, 0 };
	}
}

} /* namespace anonymous */

void
Parser::parseListItem( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName,
	bool collectRefLinks )
{
	QSharedPointer< ListItem > item( new ListItem() );

	int i = 0;

	if( isOrderedList( fr.first(), &i ) )
		item->setListType( ListItem::Ordered );
	else
		item->setListType( ListItem::Unordered );

	if( item->listType() == ListItem::Ordered )
		item->setOrderedListPreState( i == 1 ? ListItem::Start : ListItem::Continue );

	QStringList data;

	auto it = fr.begin();
	++it;

	int pos = 1;

	auto indent = listItemData( fr.first() ).second;
	if( indent < 0 ) indent = 0;
	const auto firstNonSpacePos = calculateIndent( fr.first(), indent ).second;
	if( firstNonSpacePos - indent < 4 ) indent = firstNonSpacePos;

	data.append( fr.first().right( fr.first().length() - indent ) );

	for( auto last = fr.end(); it != last; ++it, ++pos )
	{
		const auto i = listItemData( *it ).first;

		if( i > -1 )
		{
			StringListStream stream( data );

			parse( stream, item, doc, linksToParse, workingPath, fileName,
				collectRefLinks, false );

			data.clear();

			QStringList nestedList = fr.sliced( pos );

			parseList( nestedList, item, doc, linksToParse, workingPath, fileName,
				collectRefLinks );

			break;
		}
		else
		{
			if( it->startsWith( QString( indent, c_32 ) ) )
				*it = it->right( it->length() - indent );

			data.append( *it );
		}
	}

	if( !data.isEmpty() )
	{
		StringListStream stream( data );

		parse( stream, item, doc, linksToParse, workingPath, fileName, collectRefLinks, false );
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

		if( fr.size() < 2 )
			throw ParserException( QString(
				"We found code block started with \"%1\" that doesn't finished." ).arg( fr.first() ) );

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
