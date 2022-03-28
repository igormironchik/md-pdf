
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
				fi.absolutePath() + QStringLiteral( "/" ), fi.fileName() );

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
readEscapedSequence( qsizetype i, QStringView & str )
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
		else if( str[ i ].isSpace() )
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

		for( ; p < 3; ++p )
			if( str[ p ] != ( c96 ? c_96 : c_126 ) )
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
		else if( s.startsWith( QLatin1String( "```" ) ) ||
			s.startsWith( QLatin1String( "~~~" ) ) )
		{
			return BlockType::Code;
		}
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
	}
	else
		return BlockType::Unknown;

	return BlockType::Text;
}

void
Parser::parseFragment( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
{
	switch( whatIsTheLine( fr.first() ) )
	{
		case BlockType::Text :
			parseText( fr, parent, doc, linksToParse,
				workingPath, fileName );
			break;

		case BlockType::Blockquote :
			parseBlockquote( fr, parent, doc, linksToParse, workingPath, fileName );
			break;

		case BlockType::Code :
			parseCode( fr, parent );
			break;

		case BlockType::CodeIndentedBySpaces :
		{
			int indent = 1;

			if( fr.first().startsWith( QLatin1String( "    " ) ) )
				indent = 4;

			parseCodeIndentedBySpaces( fr, parent, indent );
		}
			break;

		case BlockType::Heading :
			parseHeading( fr, parent, doc, linksToParse, workingPath, fileName );
			break;

		case BlockType::List :
			parseList( fr, parent, doc, linksToParse, workingPath, fileName );
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
	const QString & workingPath, const QString & fileName )
{
	if( isFootnote( fr.first() ) )
		parseFootnote( fr, parent, doc, linksToParse, workingPath, fileName );
	else if( isTableHeader( fr.first() ) && fr.size() > 1 && isTableAlignment( fr[ 1 ] ) )
		parseTable( fr, parent, doc, linksToParse, workingPath, fileName );
	else
		parseParagraph( fr, parent, doc, linksToParse,
			workingPath, fileName );
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
	const QString & workingPath, const QString & fileName )
{
	if( !fr.isEmpty() )
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

		QSharedPointer< Paragraph > p( new Paragraph() );

		QStringList tmp;
		tmp << fr.first().simplified();

		parseFormattedTextLinksImages( tmp, p, doc, linksToParse, workingPath, fileName );

		fr.removeFirst();

		h->setText( p );

		if( h->isLabeled() )
			doc->insertLabeledHeading( h->label(), h );
		else
		{
			QString label = QStringLiteral( "#" ) + paragraphToLabel( p.data() );

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
	const QString & workingPath, const QString & fileName )
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
			pos = 2;

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

				parse( stream, f, doc, linksToParse, workingPath, fileName, false );

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
	const QString & workingPath, const QString & fileName )
{
	static const QChar sep( '|' );

	if( fr.size() >= 2 )
	{
		QSharedPointer< Table > table( new Table() );

		auto parseTableRow = [&] ( const QString & row )
		{
			auto line = row.simplified();

			if( line.startsWith( sep ) )
				line.remove( 0, 1 );

			if( line.endsWith( sep ) )
				line.remove( line.length() - 1, 1 );

			auto columns = line.split( sep );

			QSharedPointer< TableRow > tr( new TableRow() );

			for( auto it = columns.begin(), last = columns.end(); it != last; ++it )
			{
				QSharedPointer< TableCell > c( new TableCell() );

				if( !it->isEmpty() )
				{
					it->replace( QLatin1String( "&#124;" ), sep );

					QStringList fragment;
					fragment.append( *it );

					parseFormattedTextLinksImages( fragment, c, doc,
						linksToParse, workingPath, fileName );
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

		if( !table->isEmpty() )
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
	const QString & workingPath, const QString & fileName )
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
			for( qsizetype j = 0; j < horLines; ++j )
				parent->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

			fr.remove( 0, horLines );

			QSharedPointer< Heading > h( new Heading() );
			h->setLevel( lvl );

			QSharedPointer< Paragraph > p( new Paragraph() );

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
				workingPath, fileName, true );

			fr.remove( 0, i - horLines + 1 );

			h->setText( p );

			QString label = QStringLiteral( "#" ) + paragraphToLabel( p.data() );

			label += QStringLiteral( "/" ) + workingPath + fileName;

			h->setLabel( label );

			doc->insertLabeledHeading( label, h );

			parent->appendItem( h );
		}
	}

	if( !fr.isEmpty() )
	{
		if( heading )
		{
			StringListStream stream( fr );
			parse( stream, parent, doc, linksToParse, workingPath, fileName );
		}
		else
		{
			QSharedPointer< Paragraph > p( new Paragraph );

			while( !parseFormattedTextLinksImages( fr, p, doc, linksToParse, workingPath, fileName ) )
			{
				if( !p->isEmpty() )
				{
					parent->appendItem( p );

					p.reset( new Paragraph );
				}

				parent->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );
			}

			if( !p->isEmpty() )
				parent->appendItem( p );
		}
	}
}

namespace /* anoymous*/ {

enum class Lex {
	Bold,
	Italic,
	BoldAndItalic,
	Strikethrough,
	Text,
	Link,
	Image,
	ImageInLink,
	StartOfCode,
	FootnoteRef,
	BreakLine
}; // enum class Lex

struct PreparsedData {
	QVector< Lex > lexems;

	QVector< QSharedPointer< Text > > txt;
	QVector< QSharedPointer< Link > > lnk;
	QVector< QSharedPointer< FootnoteRef > > fnref;
	QVector< QSharedPointer< Image > > img;

	int processedText = 0;
	int processedLnk = 0;
	int processedFnRef = 0;
	int processedImg = 0;
}; // struct PreparsedData


// Read URL.
inline QString
readLnk( int & i, const QString & line, bool inSquare = false )
{
	++i;
	i = skipSpaces( i, line );
	const int length = line.length();

	if( i < length )
	{
		QString lnk;

		bool backslash = false;

		if( !inSquare )
		{
			if( line[ i ] == c_60 )
			{
				++i;

				while( i < length )
				{
					if( line[ i ] == c_92 && !backslash )
						backslash = true;
					else if( line[ i ] == c_62 && !backslash  )
						return lnk;
					else
					{
						lnk.append( line[ i ] );
						backslash = false;
					}

					++i;
				}
			}
			else
			{
				qsizetype c = 0;

				while( i < length )
				{
					if( line[ i ].isSpace() )
					{
						if( !c )
							return lnk;
						else
							return QString();
					}
					else if( line[ i ] == c_92 && !backslash )
						backslash = true;
					else if( line[ i ] == c_40 && !backslash )
					{
						++c;
						lnk.append( line[ i ] );
						backslash = false;
					}
					else if( line[ i ] == c_41 && !backslash && c > 0 )
					{
						--c;
						lnk.append( line[ i ] );
						backslash = false;
					}
					else if( line[ i ] == c_41 && !backslash && c == 0 )
						return lnk;
					else
					{
						lnk.append( line[ i ] );
						backslash = false;
					}

					++i;
				}
			}
		}
		else
		{
			while( i < length )
			{
				if( line[ i ] == c_92 && !backslash )
					backslash = true;
				else if( line[ i ] == c_93 && !backslash  )
					return lnk;
				else if( line[ i ] == c_91 && !backslash )
					return QString();
				else
				{
					lnk.append( line[ i ] );
					backslash = false;
				}

				++i;
			}
		}

		return lnk;
	}
	else
		return QString();
}; // readLnk


// Skip link's caption.
inline bool
skipLnkCaption( int & i, const QString & line )
{
	bool quoted = false;

	if( line[ i ] == c_34 )
	{
		quoted = true;
		++i;
	}

	const int length = line.length();

	bool backslash = false;

	while( i < length )
	{
		if( quoted )
		{
			if( line[ i ] == c_92 && !backslash )
				backslash = true;
			if( line[ i ] == c_34 && !backslash )
				break;
			else
				backslash = false;
		}
		else
		{
			if( line[ i ] == c_41 )
				break;
		}

		++i;
	}

	if( i < length )
	{
		if( quoted )
		{
			++i;

			i = skipSpaces( i, line );

			if( i < length )
			{
				if( line[ i ] == c_41 )
				{
					++i;

					return true;
				}
			}
			else
				return false;
		}
		else if( line[ i ] == c_41 )
			return true;
	}

	return false;
}; // skipLnkCaption


// Add footnote ref.
inline void
addFootnoteRef( const QString & lnk, PreparsedData & data, const QString & workingPath,
	const QString & fileName )
{
	QSharedPointer< FootnoteRef > fnr(
		new FootnoteRef( QStringLiteral( "#" ) + lnk +
			QStringLiteral( "/" ) + workingPath + fileName ) );

	data.fnref.append( fnr );
	data.lexems.append( Lex::FootnoteRef );
}; // addFootnoteRef


// Read image.
inline int parseImg( int i, const QString & line, bool & ok,
	bool addLex, PreparsedData & data, const QString & workingPath )
{
	i += 2;
	const int length = line.length();

	QString t = readLinkText( i, line );

	i = skipSpaces( i, line );

	if( i < length && line[ i ] == c_40 )
	{
		QString lnk = readLnk( i, line );

		if( !lnk.isEmpty() && i < length )
		{
			i = skipSpaces( i, line );

			if( i < length )
			{
				if( skipLnkCaption( i, line ) )
				{
					QSharedPointer< Image > img( new Image() );
					img->setText( t.simplified() );

					if( !QUrl( lnk ).isRelative() )
						img->setUrl( lnk );
					else
						img->setUrl( fileExists( lnk, workingPath ) ? workingPath + lnk : lnk );

					data.img.append( img );

					if( addLex )
						data.lexems.append( Lex::Image );

					ok = true;

					return i;
				}
			}
		}
	}

	ok = false;

	return i;
}; // parseImg

// Read link.
inline int
parseLnk( int i, const QString & line, QString & text, PreparsedData & data,
	const QString & workingPath, const QString & fileName, QStringList & linksToParse,
	QSharedPointer< Document > doc )
{
	const int startPos = i;
	bool withImage = false;

	++i;

	i = skipSpaces( i, line );

	const int length = line.length();
	QString lnkText, url;

	if( i < length )
	{
		// Image in link.
		if( i + 1 < length && line[ i ] == c_33 &&
			line[ i + 1 ] == c_91 )
		{
			bool ok = false;

			i = parseImg( i, line, ok, false, data, workingPath ) + 1;

			if( !ok )
			{
				text.append( line.mid( startPos, i - startPos ) );

				return i;
			}
			else
			{
				withImage = true;

				i = skipSpaces( i, line );

				if( i < length && line[ i ] == c_93 )
					++i;
				else
				{
					text.append( line.mid( startPos, i - startPos ) );

					return i;
				}
			}
		}
		// Footnote ref.
		else if( line[ i ] == c_94 )
		{
			auto lnk = readLnk( i, line, true );

			i = skipSpaces( i, line );

			if( i < length && line[ i ] == c_93 )
			{
				if( i + 1 < length )
				{
					if( line[ i + 1 ] != c_58 )
					{
						addFootnoteRef( lnk, data, workingPath, fileName );

						return i + 1;
					}
					else
					{
						text.append( line.mid( startPos, i - startPos + 2 ) );

						return i + 2;
					}
				}
				else
				{
					addFootnoteRef( lnk, data, workingPath, fileName );

					return i + 1;
				}
			}
			else
			{
				text.append( line.mid( startPos, i - startPos ) );

				return i;
			}
		}
		else
			lnkText = readLinkText( i, line ).simplified();
	}

	i = skipSpaces( i, line );

	if( i < length )
	{
		// Labeled link.
		if( line[ i ] == c_58 )
		{
			url = readLnk( i, line );

			if( !url.isEmpty() )
			{
				if( QUrl( url ).isRelative() )
				{
					if( fileExists( url, workingPath ) )
					{
						url = QFileInfo( workingPath + url ).absoluteFilePath();

						linksToParse.append( url );
					}
				}

				QSharedPointer< Link > lnk( new Link() );
				lnk->setUrl( url );

				doc->insertLabeledLink(
					QString::fromLatin1( "#" ) + lnkText +
					QStringLiteral( "/" ) + workingPath + fileName, lnk );

				return length;
			}
			else
			{
				text.append( line.mid( startPos, i - startPos ) );

				return i;
			}
		}
		// Regular link.
		else if( line[ i ] == c_40 )
		{
			url = readLnk( i, line );

			if( !url.isEmpty() && i < length )
			{
				if( !url.startsWith( c_35 ) )
				{
					i = skipSpaces( i, line );

					if( i < length )
					{
						if( skipLnkCaption( i, line ) )
						{
							++i;

							if( QUrl( url ).isRelative() )
							{
								if( fileExists( url, workingPath ) )
								{
									url = QFileInfo( workingPath + url ).absoluteFilePath();

									linksToParse.append( url );
								}
							}
						}
						else
						{
							text.append( line.mid( startPos, i - startPos ) );

							return i;
						}
					}
				}
				else
				{
					++i;

					url = url + QStringLiteral( "/" ) + workingPath + fileName;
				}
			}
			else
			{
				text.append( line.mid( startPos, i - startPos ) );

				return i;
			}
		}
		// Referenced link.
		else if( line[ i ] == c_91 )
		{
			url = readLnk( i, line, true );

			if( !url.isEmpty() )
			{
				i = skipSpaces( i, line );

				if( i < length && line[ i ] == c_93 )
				{
					url = QString::fromLatin1( "#" ) + url +
						QStringLiteral( "/" ) + workingPath + fileName;

					linksToParse.append( url );

					++i;
				}
				else
				{
					text.append( line.mid( startPos, i - startPos ) );

					return i;
				}
			}
		}
		else if( !withImage && !lnkText.isEmpty() )
		{
			url = QString::fromLatin1( "#" ) + lnkText.toLower() +
				QStringLiteral( "/" ) + workingPath + fileName;

			linksToParse.append( url );
		}
		else
		{
			text.append( line.mid( startPos, i - startPos + 1 ) );

			return i + 1;
		}
	}
	else if( !withImage && !lnkText.isEmpty() )
	{
		url = QString::fromLatin1( "#" ) + lnkText.toLower() +
			QStringLiteral( "/" ) + workingPath + fileName;

		linksToParse.append( url );
	}
	else
	{
		text.append( line.mid( startPos, i - startPos ) );

		if( withImage )
			data.img.removeLast();

		return i;
	}

	QSharedPointer< Link > lnk( new Link() );
	lnk->setUrl( url );
	lnk->setText( lnkText );
	data.lnk.append( lnk );

	if( withImage )
		data.lexems.append( Lex::ImageInLink );
	else
		data.lexems.append( Lex::Link );

	return i - 1;
}; // parseLnk

// Create text object.
inline void
createTextObj( const QString & text, PreparsedData & data,
	bool addExtraSpaceAfter = false, bool setSimplified = true )
{
	const auto simplified = text.simplified();

	if( ( setSimplified ? !simplified.isEmpty() : !text.isEmpty() ) )
	{
		QSharedPointer< Text > t( new Text() );
		t->setText( setSimplified ? simplified : text );
		t->setOpts( TextWithoutFormat );
		t->setSpaceBefore( text[ 0 ].isSpace() );
		t->setSpaceAfter( addExtraSpaceAfter ||
			( text.size() > 1 && text[ text.size() - 1 ].isSpace() ) );
		data.txt.append( t );
		data.lexems.append( Lex::Text );
	}
}; // createTextObject


enum class LineParsingState {
	Finished,
	UnfinishedCode,
	UnfinishedQuotedCode
}; // enum class LineParsingState

inline bool
isStyleChar( const QChar & c )
{
	if( c == c_42 || c == c_95 )
		return true;
	else
		return false;
}

struct InlineCodeMark {
	qsizetype m_line;
	qsizetype m_pos;
	qsizetype m_length;
	bool m_backslashed;
};

inline bool
isBeforeEndOfCode( QVector< InlineCodeMark >::const_iterator end,
	qsizetype line, qsizetype pos )
{
	if( end->m_line < line )
		return false;
	else if( end->m_line == line )
		return ( end->m_pos > pos );
	else
		return true;
}

inline QVector< InlineCodeMark >::const_iterator
endOfInlineCode( QVector< InlineCodeMark >::const_iterator it,
	QVector< InlineCodeMark >::const_iterator last )
{
	qsizetype length = 0;

	while( !length && it != last )
	{
		length = it->m_length - ( it->m_backslashed ? 1 : 0 );
		++it;
	}

	return std::find_if( it, last,
		[length] ( const auto & bt ) { return ( bt.m_length == length ); } );
}

inline bool
isInCode( const QVector< InlineCodeMark > & inlineCodeMarks, qsizetype line, qsizetype pos )
{
	bool first = true;
	bool quoted = false;

	for( auto it = inlineCodeMarks.cbegin(), last = inlineCodeMarks.cend(); it != last; ++it )
	{
		if( it->m_line > line )
			return false;
		else if( it->m_line == line && it->m_pos > pos )
			return false;

		const auto eit = endOfInlineCode( it, last );

		if( eit != last )
		{
			if( isBeforeEndOfCode( eit, line, pos ) )
				return true;

			it = eit;
		}
	}

	return false;
}

inline QVector< InlineCodeMark >
collectCodeMarks( int i, QStringList::const_iterator it, const QStringList & fr )
{
	QVector< InlineCodeMark > inlineCodeMarks;

	bool backslash = false;

	qsizetype line = 0;
	qsizetype pos = 0;
	qsizetype length = 0;
	bool firstBacktrick = true;

	for( auto last = fr.cend(), iit = it; iit != last; ++iit )
	{
		for( ; i < iit->length(); ++i )
		{
			bool now = false;

			if( (*iit)[ i ] == c_92 )
			{
				backslash = true;
				now = true;
			}
			else if( (*iit)[ i ] == c_96 )
			{
				pos = i;
				length = 0;

				while( i < iit->length() && (*iit)[ i ] == c_96 )
				{
					++length;
					++i;
				}

				inlineCodeMarks.push_back( { line, pos, length, backslash } );

				--i;
			}

			if( !now )
				backslash = false;
		}

		i = 0;
		++line;
	}

	return inlineCodeMarks;
}

// Check if style closed.
inline bool
isStyleClosed( int i, const QString & style, Lex lexStyle, QStringList::const_iterator it,
	const QStringList & fr )
{
	++i;

	QString s;

	bool backslash = false;

	QVector< InlineCodeMark > inlineCodeMarks = collectCodeMarks( i, it, fr );

	qsizetype line = 0;

	for( auto last = fr.end(); it != last; ++it )
	{
		for( ; i < it->length(); ++i )
		{
			s.clear();

			bool now = false;

			if( (*it)[ i ] == c_92 )
			{
				backslash = !backslash;
				now = backslash;
			}

			if( i + style.length() <= it->length() )
			{
				for( auto p = i; p < i + style.length(); ++p )
				{
					if( !backslash && isStyleChar( (*it)[ p ] ) )
						s.append( (*it)[ p ] );
				}
			}

			if( !now )
				backslash = false;

			if( s.length() == style.length() )
			{
				switch( lexStyle )
				{
					case Lex::Italic :
					{
						if(	s == style )
						{
							if( !isInCode( inlineCodeMarks, line, i ) )
								return true;
						}
					}
						break;

					case Lex::Bold :
					{
						if( s == style )
						{
							if( !isInCode( inlineCodeMarks, line, i ) )
								return true;
						}
					}
						break;

					case Lex::BoldAndItalic :
					{
						if( ( style == QLatin1String( "***" ) || style == QLatin1String( "___" ) )
							&& style == s )
						{
							if( !isInCode( inlineCodeMarks, line, i ) )
								return true;
						}
						else if( style == QLatin1String( "_**" ) && s == QLatin1String( "**_" ) )
						{
							if( !isInCode( inlineCodeMarks, line, i ) )
								return true;
						}
						else if( style == QLatin1String( "__*" ) && s == QLatin1String( "*__" ) )
						{
							if( !isInCode( inlineCodeMarks, line, i ) )
								return true;
						}
					}
						break;

					default :
						break;
				}
			}
		}

		i = 0;
		++line;
		backslash = false;
	}

	return false;
}

inline bool
isStrikethroughClosed( int i, QStringList::iterator it, const QStringList & fr )
{
	i += 2;

	bool backslash = false;

	QVector< InlineCodeMark > inlineCodeMarks = collectCodeMarks( i, it, fr );

	qsizetype line = 0;

	for( auto last = fr.end(); it != last; ++it )
	{
		for( ; i < it->length(); ++i )
		{
			bool now = false;

			if( (*it)[ i ] == c_92 )
			{
				backslash = !backslash;
				now = backslash;
			}

			if( i + 1 < it->length() )
			{
				if( !backslash && (*it)[ i ] == c_126 && (*it)[ i + 1 ] == c_126 )
				{
					if( !isInCode( inlineCodeMarks, line, i ) )
						return true;
				}
			}

			if( !now )
				backslash = false;
		}

		i = 0;
		++line;
		backslash = false;
	}

	return false;
}


inline bool
isStyleLexOdd( const QVector< Lex > & lexems, Lex lex )
{
	return ( lexems.count( lex ) % 2 != 0 );
}


// Read code.
inline void
parseInlineCode( qsizetype pos, QStringList::iterator & it, QVector< InlineCodeMark >::const_iterator end,
	PreparsedData & data )
{
	QString code;

	for( auto l = 0; l <= end->m_line; ++l )
	{
		if( l < end->m_line )
		{
			const auto length = it->length();

			for( qsizetype i = pos; i < length; ++i )
				code.append( (*it)[ i ] );

			code.append( c_32 );

			++it;
		}
		else
		{
			for( qsizetype i = pos; i < end->m_pos; ++i )
				code.append( (*it)[ i ] );
		}

		pos = 0;
	}

	data.lexems.append( Lex::StartOfCode );

	const auto ns = skipSpaces( 0, code );

	if( ns != code.length() )
	{
		if( code.length() > 2 && code.front().isSpace() && code.back().isSpace() )
		{
			code.remove( 0, 1 );
			code.remove( code.length() - 1, 1 );
		}
	}

	createTextObj( code, data, false, false );

	data.lexems.append( Lex::StartOfCode );
}; // parseCode

// Read URL in <...>
inline int
parseUrl( int i, const QString & line, QString & text, PreparsedData & data )
{
	const int start = i;

	++i;

	const int length = line.length();

	bool done = false;
	QString url;

	while( i < length )
	{
		if( line[ i ] != c_62 )
			url.append( line[ i ] );
		else
		{
			done = true;
			++i;
			break;
		}

		++i;
	}

	const auto it = std::find_if( url.cbegin(), url.cend(),
		[] ( const auto & c ) { return c.isSpace(); } );

	if( it != url.cend() )
		done = false;
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
			done = false;
	}

	if( done )
	{
		QSharedPointer< Link > lnk( new Link() );
		lnk->setUrl( url.simplified() );
		data.lnk.append( lnk );
		data.lexems.append( Lex::Link );
	}
	else
		text.append( line.mid( start, i - start ) );

	return i;
}; // parseUrl


// Parse one line in paragraph.
inline QPair< QStringList::iterator, bool >
parseLine( QStringList::iterator it, qsizetype & line, qsizetype pos, PreparsedData & data,
	const QString & workingPath, const QString & fileName, QStringList & linksToParse,
	QSharedPointer< Document > doc, const QStringList & fr, bool ignoreLineBreak )
{
	static const QString specialChars( QLatin1String( "!\"#$%&'()*+,-.\\/:;<=>?@[]^_`{|}~" ) );

	bool hasBreakLine = !ignoreLineBreak && it->endsWith( QLatin1String( "  " ) );

	if( !ignoreLineBreak && it->endsWith( c_92 ) )
	{
		hasBreakLine = true;
		it->remove( it->length() - 1, 1 );
	}

	const auto ns = skipSpaces( 0, *it );

	if( ns > 0 )
		*it = it->sliced( ns );

	const auto isHorLine = isHorizontalLine( *it );

	// Will skip horizontal rules, for now at least...
	if( ns > 3 || !isHorLine )
	{
		QString text;

		for( int i = pos; i < it->length(); ++i )
		{
			const auto length = it->length();

			if( (*it)[ i ] == c_92 && i + 1 < length &&
				specialChars.contains( (*it)[ i + 1 ] ) )
			{
				++i;

				text.append( (*it)[ i ] );
			}
			else if( (*it)[ i ] == c_33 && i + 1 < length &&
				(*it)[ i + 1 ] == c_91 )
			{
				createTextObj( text, data );
				text.clear();

				bool ok = false;

				const int startPos = i;

				i = parseImg( i, *it, ok, true, data, workingPath );

				if( !ok )
					text.append( it->mid( startPos, i - startPos ) );
			}
			else if( (*it)[ i ] == c_91 )
			{
				createTextObj( text, data );
				text.clear();
				i = parseLnk( i, *it, text, data, workingPath, fileName, linksToParse, doc );
			}
			else if( (*it)[ i ] == c_96 )
			{
				const auto backtricks = collectCodeMarks( i, it, fr );
				const auto ceit = endOfInlineCode( backtricks.cbegin(), backtricks.cend() );

				if( ceit != backtricks.cend() )
				{
					createTextObj( text, data );
					text.clear();

					parseInlineCode( backtricks.cbegin()->m_pos + backtricks.cbegin()->m_length,
						it, ceit, data );
					i = ceit->m_pos + ceit->m_length - 1;
					line += ceit->m_line;
				}
				else
					text.append( (*it)[ i ] );
			}
			else if( (*it)[ i ] == c_60 )
			{
				createTextObj( text, data );
				text.clear();
				i = parseUrl( i, *it, text, data ) - 1;
			}
			else if( (*it)[ i ] == c_42 || (*it)[ i ] == c_95 )
			{
				QString style;

				while( i < length && isStyleChar( (*it)[ i ] ) )
				{
					style.append( (*it)[ i ] );
					++i;
				}

				if( !style.isEmpty() )
					--i;

				if( style == QLatin1String( "*" ) || style == QLatin1String( "_" ) )
				{
					if( isStyleLexOdd( data.lexems, Lex::Italic ) ||
						isStyleClosed( i, style, Lex::Italic, it, fr ) )
					{
						createTextObj( text, data );
						text.clear();
						data.lexems.append( Lex::Italic );
					}
					else
						text.append( style );
				}
				else if( style == QLatin1String( "**" ) || style == QLatin1String( "__" ) )
				{
					if( isStyleLexOdd( data.lexems, Lex::Bold ) ||
						isStyleClosed( i, style, Lex::Bold, it, fr ) )
					{
						createTextObj( text, data );
						text.clear();
						data.lexems.append( Lex::Bold );
					}
					else
						text.append( style );
				}
				else if( style == QLatin1String( "***" ) || style == QLatin1String( "___" ) ||
					style == QLatin1String( "_**" ) || style == QLatin1String( "**_" ) ||
					style == QLatin1String( "*__" ) || style == QLatin1String( "__*" ) )
				{
					if( isStyleLexOdd( data.lexems, Lex::BoldAndItalic ) ||
						isStyleClosed( i, style, Lex::BoldAndItalic, it, fr ) )
					{
						createTextObj( text, data );
						text.clear();
						data.lexems.append( Lex::BoldAndItalic );
					}
					else
						text.append( style );
				}
				else
					text.append( style );
			}
			else if( (*it)[ i ] == c_126 && i + 1 < length &&
				(*it)[ i + 1 ] == c_126 )
			{
				if( isStyleLexOdd( data.lexems, Lex::Strikethrough ) ||
					isStrikethroughClosed( i, it, fr ) )
				{
					++i;
					createTextObj( text, data );
					text.clear();
					data.lexems.append( Lex::Strikethrough );
				}
				else
				{
					text.append( QString( 2, c_126 ) );
					++i;
				}
			}
			else
				text.append( (*it)[ i ] );
		}

		createTextObj( text, data, true );
		text.clear();

		if( hasBreakLine )
			data.lexems.append( Lex::BreakLine );
	}
	else if( isHorLine )
		return { ++it, true };

	return { ++it, false };
}; // parseit

// Set flags for all nested items.
inline void
setFlags( Lex lex, QVector< Lex >::iterator it, PreparsedData & data )
{
	static const auto lexToFormat = []( Lex lex ) -> TextOptions
	{
		switch( lex )
		{
			case Lex::Bold :
				return BoldText;

			case Lex::Italic :
				return ItalicText;

			case Lex::BoldAndItalic :
				return ( BoldText | ItalicText );

			case Lex::Strikethrough :
				return StrikethroughText;

			default :
				return TextWithoutFormat;
		}
	};

	auto close = std::find( it + 1, data.lexems.end(), lex );

	if( close != data.lexems.end() )
	{
		int processedText = data.processedText;
		int processedLnk = data.processedLnk;

		for( auto i = it + 1; i != close; ++i )
		{
			switch( *i )
			{
				case Lex::Text :
				{
					data.txt[ processedText ]->setOpts( data.txt[ processedText ]->opts() |
						lexToFormat( lex ) );
					++processedText;
				}
					break;

				case Lex::Link :
				{
					data.lnk[ processedLnk ]->setTextOptions( data.lnk[ processedLnk ]->textOptions() |
						lexToFormat( lex ) );
					++processedLnk;
				}
					break;

				default :
					break;
			}
		}
	}
}; // setFlags

inline void
addItemsToParent( PreparsedData & data, QSharedPointer< Block > parent )
{
	bool addExtraSpace = false;
	auto tPos = data.txt.size();

	// Add real items to paragraph  after pre-parsing. Handle code.
	for( auto it = data.lexems.begin(), last = data.lexems.end(); it != last; ++it )
	{
		switch( *it )
		{
			case Lex::Bold :
			case Lex::Italic :
			case Lex::BoldAndItalic :
			case Lex::Strikethrough :
			{
				setFlags( *it, it, data );
			}
				break;

			case Lex::StartOfCode :
			{
				auto end = std::find( it + 1, data.lexems.end(), *it );

				if( end != data.lexems.end() )
				{
					if( tPos != data.txt.size() )
						data.txt[ tPos ]->setSpaceAfter( true );

					++it;

					QSharedPointer< Code > c( new Code( QString(), true ) );

					for( ; it != end; ++it )
					{
						c->setText( c->text() + data.txt[ data.processedText ]->text() +
							c_32 );

						++data.processedText;
					}

					if( c->text().endsWith( c_32 ) )
						c->setText( c->text().left( c->text().length() - 1 ) );

					parent->appendItem( c );

					addExtraSpace = true;
				}
			}
				break;

			case Lex::Text :
			{
				parent->appendItem( data.txt[ data.processedText ] );

				if( addExtraSpace )
					data.txt[ data.processedText ]->setSpaceBefore( true );

				addExtraSpace = false;

				tPos = data.processedText;

				++data.processedText;
			}
				break;

			case Lex::Link :
			{
				parent->appendItem( data.lnk[ data.processedLnk ] );
				++data.processedLnk;
			}
				break;

			case Lex::Image :
			{
				parent->appendItem( data.img[ data.processedImg ] );
				++data.processedImg;
			}
				break;

			case Lex::BreakLine :
			{
				parent->appendItem( QSharedPointer< Item > ( new LineBreak() ) );
			}
				break;

			case Lex::ImageInLink :
			{
				data.lnk[ data.processedLnk ]->setImg( data.img[ data.processedImg ] );
				++data.processedImg;
				parent->appendItem( data.lnk[ data.processedLnk ] );
				++data.processedLnk;
			}
				break;

			case Lex::FootnoteRef :
			{
				parent->appendItem( data.fnref[ data.processedFnRef ] );
				++data.processedFnRef;
			}
				break;
		}
	}
}

} /* namespace anonymous */

bool
Parser::parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName, bool ignoreLineBreak )

{
	if( fr.isEmpty() )
		return true;

	PreparsedData data;

	qsizetype pos = 0;
	qsizetype line = 0;
	bool breakParagraph = false;

	// Real parsing.
	for( auto it = fr.begin(), last = fr.end(); it != last; )
	{
		std::tie( it, breakParagraph ) =
			parseLine( it, line, pos, data, workingPath, fileName, linksToParse, doc, fr,
				ignoreLineBreak );

		if( breakParagraph )
		{
			addItemsToParent( data, parent );

			fr.erase( fr.begin(), it );

			return false;
		}
	}

	addItemsToParent( data, parent );

	return true;
}

void
Parser::parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
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

		parse( stream, bq, doc, linksToParse, workingPath, fileName );

		if( !bq->isEmpty() )
			parent->appendItem( bq );

		if( horLine )
		{
			parent->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

			++i;

			if( i < fr.size() )
			{
				tmp = fr.sliced( i );

				StringListStream stream( tmp );

				parse( stream, parent, doc, linksToParse, workingPath, fileName );
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
	const QString & workingPath, const QString & fileName )
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
				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName );
				listItem.clear();

				if( !list->isEmpty() )
					parent->appendItem( list );

				list.reset( new List );

				doc->appendItem( QSharedPointer< Item > ( new HorizontalLine ) );

				continue;
			}
			else if( isListItemAndNotNested( *it ) && !listItem.isEmpty() )
			{
				parseListItem( listItem, list, doc, linksToParse, workingPath, fileName );
				listItem.clear();
			}

			listItem.append( *it );
		}

		if( !listItem.isEmpty() )
			parseListItem( listItem, list, doc, linksToParse, workingPath, fileName );

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
	const QString & workingPath, const QString & fileName )
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

			parse( stream, item, doc, linksToParse, workingPath, fileName );

			data.clear();

			QStringList nestedList = fr.sliced( pos );

			parseList( nestedList, item, doc, linksToParse, workingPath, fileName );

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

		parse( stream, item, doc, linksToParse, workingPath, fileName );
	}

	if( !item->isEmpty() )
		parent->appendItem( item );
}

void
Parser::parseCode( QStringList & fr, QSharedPointer< Block > parent, int indent )
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

	parseCodeIndentedBySpaces( fr, parent, indent, syntax );
}

void
Parser::parseCodeIndentedBySpaces( QStringList & fr, QSharedPointer< Block > parent,
	int indent, const QString & syntax )
{
	QString code;

	for( const auto & l : qAsConst( fr ) )
		code.append( ( indent > 0 ? l.right( l.length() - indent ) + c_10 :
			l + c_10 ) );

	if( !code.isEmpty() )
	{
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
