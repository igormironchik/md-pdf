
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

inline qsizetype
posOfFirstNonSpace( const QString & s )
{
	for( qsizetype p = 0; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			return p;
	}

	return -1;
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
			for( ; p < str.size(); ++p )
			{
				if( !str[ p ].isSpace() )
					break;
			}

			if( p < str.size() )
				*syntax = str.mid( p ).toString();
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

Parser::BlockType
Parser::whatIsTheLine( QString & str, bool inList, qsizetype * indent, bool calcIndent ) const
{
	str.replace( c_9, QString( 4, c_32 ) );

	auto first = posOfFirstNonSpace( str );
	if( first < 0 ) first = 0;

	auto s = QStringView( str ).sliced( first );

	if( s.startsWith( c_62 ) )
		return BlockType::Blockquote;
	else if( s.startsWith( QLatin1String( "```" ) ) ||
		s.startsWith( QLatin1String( "~~~" ) ) )
	{
		return BlockType::Code;
	}
	else if( s.isEmpty() )
		return BlockType::Unknown;
	else if( s.startsWith( c_35 ) )
		return BlockType::Heading;

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
			parseText( fr, parent, doc, linksToParse, workingPath, fileName );
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

inline bool
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
		parseParagraph( fr, parent, doc, linksToParse, workingPath, fileName );
}

namespace /* anonymous */ {

// Skip spaces in line from pos \a i.
int
skipSpaces( int i, const QString & line )
{
	const int length = line.length();

	while( i < length && line[ i ].isSpace() )
		++i;

	return i;
}; // skipSpaces

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

} /* namespace anonymous */

void
Parser::parseHeading( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
{
	if( !fr.isEmpty() )
	{
		auto line = fr.first();
		int pos = 0;
		pos = skipSpaces( pos, line );

		if( pos > 0 )
			line = line.mid( pos );

		pos = 0;
		int lvl = 0;

		while( pos < line.length() && line[ pos ] == c_35 )
		{
			++lvl;
			++pos;
		}

		pos = skipSpaces( pos, line );

		fr.first() = line.mid( pos );

		const auto label = findAndRemoveHeaderLabel( fr.first() );

		QSharedPointer< Heading > h( new Heading() );
		h->setLevel( lvl );

		if( !label.isEmpty() )
			h->setLabel( label.mid( 1, label.length() - 2) + QStringLiteral( "/" ) +
				workingPath + fileName );

		QSharedPointer< Paragraph > p( new Paragraph() );

		QStringList tmp;
		tmp << fr.first();

		parseFormattedTextLinksImages( tmp, p, doc, linksToParse, workingPath, fileName );

		fr.removeFirst();

		if( !p->isEmpty() )
		{
			QString text;

			for( auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it )
			{
				if( (*it)->type() == ItemType::Text )
				{
					auto t = static_cast< Text* > ( it->data() );

					text.append( t->text() + c_32 );
				}
			}

			text = text.simplified();

			if( !text.isEmpty() )
			{
				h->setText( text );

				if( h->isLabeled() )
					doc->insertLabeledHeading( h->label(), h );
				else
				{
					QString label = QStringLiteral( "#" );

					for( const auto & c : qAsConst( text ) )
					{
						if( c.isLetter() || c.isDigit() )
							label.append( c.toLower() );
						else if( c.isSpace() )
							label.append( QStringLiteral( "-" ) );
					}

					label += QStringLiteral( "/" ) + workingPath + fileName;

					h->setLabel( label );

					doc->insertLabeledHeading( label, h );
				}

				parent->appendItem( h );
			}
		}
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
			line = line.mid( pos );

		if( line.startsWith( QLatin1String( "[^" ) ) )
		{
			pos = 2;

			QString id = readLinkText( pos, line );

			if( !id.isEmpty() && line[ pos ] == c_58 )
			{
				++pos;

				line = line.mid( pos );

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

inline bool
isH( const QString & s, const QChar & c )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

	const auto start = p;

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] != c )
			break;
	}

	if( p - start < 3 )
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

void
Parser::parseParagraph( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
{
	auto ph = [&]( const QString & label )
	{
		QStringList tmp = fr.mid( 0, 1 );
		tmp.first().prepend( label );

		parseHeading( tmp, parent, doc, linksToParse, workingPath, fileName );

		for( int idx = 0; idx < 2; ++idx )
			fr.removeFirst();

		parseParagraph( fr, parent, doc, linksToParse, workingPath, fileName );
	};
	\
	// Check for alternative syntax of H1 and H2 headings.
	if( fr.size() >= 2 )
	{
		if( isH1( fr[ 1 ] ) )
		{
			ph( QLatin1String( "# " ) );

			return;
		}
		else if( isH2( fr[ 1 ] ) )
		{
			ph( QLatin1String( "## " ) );

			return;
		}
	}

	QSharedPointer< Paragraph > p( new Paragraph() );

	parseFormattedTextLinksImages( fr, p, doc, linksToParse, workingPath, fileName );

	if( !p->isEmpty() )
		parent->appendItem( p );
}

inline bool
isHorizontalLine( const QString & s )
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

	for( ; p < s.size(); ++p )
	{
		if( s[ p ] != c )
			break;
	}

	if( p < 3 )
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

void
Parser::parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse, const QString & workingPath,
	const QString & fileName )

{
	static const QString specialChars( QLatin1String( "\\`*_{}[]()#+-.!|~<>" ) );

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
		StartOfQuotedCode,
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

	PreparsedData data;

	// Read URL.
	auto readLnk = [&]( int & i, const QString & line ) -> QString
	{
		++i;
		i = skipSpaces( i, line );
		const int length = line.length();

		if( i < length )
		{
			QString lnk;

			while( i < length && !line[ i ].isSpace() &&
				( line[ i ] != c_41 && line[ i - 1 ] != c_92 )
				&& line[ i ] != c_93 )
			{
				lnk.append( line[ i ] );
				++i;
			}

			return lnk;
		}
		else
			return QString();
	}; // readLnk

	// Skip link's caption.
	auto skipLnkCaption = [&]( int & i, const QString & line ) -> bool
	{
		bool quoted = false;

		if( line[ i ] == c_34 )
		{
			quoted = true;
			++i;
		}

		const int length = line.length();

		while( i < length &&
			( quoted ?
				( line[ i ] != c_34 && line[ i - 1 ] != c_92 ) :
				( line[ i ] != c_41 ) ) )
		{
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
	auto addFootnoteRef = [&]( const QString & lnk )
	{
		QSharedPointer< FootnoteRef > fnr(
			new FootnoteRef( QStringLiteral( "#" ) + lnk +
				QStringLiteral( "/" ) + workingPath + fileName ) );

		data.fnref.append( fnr );
		data.lexems.append( Lex::FootnoteRef );
	}; // addFootnoteRef

	// Read image.
	auto parseImg = [&]( int i, const QString & line, bool * ok = nullptr,
		bool addLex = true ) -> int
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

						if( ok )
							*ok = true;

						return i;
					}
				}
			}
		}

		if( ok )
			*ok = false;

		return i;
	}; // parseImg

	// Read link.
	auto parseLnk = [&]( int i, const QString & line, QString & text ) -> int
	{
		const int startPos = i;
		bool withImage = false;

		++i;

		i = skipSpaces( i, line );

		const int length = line.length();
		QString lnkText, url;

		if( i < length )
		{
			if( i + 1 < length && line[ i ] == c_33 &&
				line[ i + 1 ] == c_91 )
			{
				bool ok = false;

				i = parseImg( i, line, &ok, false ) + 1;

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
			else if( line[ i ] == c_94 )
			{
				auto lnk = readLnk( i, line );

				i = skipSpaces( i, line );

				if( i < length && line[ i ] == c_93 )
				{
					if( i + 1 < length )
					{
						if( line[ i + 1 ] != c_58 )
						{
							addFootnoteRef( lnk );

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
						addFootnoteRef( lnk );

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
						url = url + QStringLiteral( "/" ) + workingPath + fileName;
				}
				else
				{
					text.append( line.mid( startPos, i - startPos ) );

					return i;
				}
			}
			else if( line[ i ] == c_91 )
			{
				url = readLnk( i, line );

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
			else
			{
				text.append( line.mid( startPos, i - startPos + 1 ) );

				return i + 1;
			}
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

		return i;
	}; // parseLnk

	enum class LineParsingState {
		Finished,
		UnfinishedCode,
		UnfinishedQuotedCode
	}; // enum class LineParsingState

	// Create text object.
	auto createTextObj = [&]( const QString & text )
	{
		if( !text.isEmpty() )
		{
			QSharedPointer< Text > t( new Text() );
			t->setText( text );
			t->setOpts( TextWithoutFormat );
			data.txt.append( t );
			data.lexems.append( Lex::Text );
		}
	}; // createTextObject

	// Read code.
	auto parseCode = [&]( int i, const QString & line, LineParsingState & prevAndNext ) -> int
	{
		const int length = line.length();

		bool quoted = false;

		if( prevAndNext != LineParsingState::Finished )
			quoted = ( prevAndNext == LineParsingState::UnfinishedQuotedCode );
		else
		{
			if( i + 1 < length && line[ i + 1 ] == c_96 )
			{
				quoted = true;

				data.lexems.append( Lex::StartOfQuotedCode );

				i += 2;
			}
			else
			{
				data.lexems.append( Lex::StartOfCode );

				++i;
			}
		}

		QString code;
		bool finished = false;

		while( i < length )
		{
			if( line[ i ] == c_96 )
			{
				if( !quoted )
				{
					finished = true;

					++i;

					break;
				}
				else if( i + 1 < length && line[ i + 1 ] == c_96 )
				{
					if( i + 2 < length && line[ i + 2 ] == c_96 )
					{
						code.append( line[ i ] );

						++i;

						continue;
					}
					else
					{
						finished = true;

						i += 2;

						break;
					}
				}
			}

			code.append( line[ i ] );

			++i;
		}

		createTextObj( code );

		if( finished )
		{
			data.lexems.append( quoted ? Lex::StartOfQuotedCode : Lex::StartOfCode );
			prevAndNext = LineParsingState::Finished;
		}
		else
			prevAndNext = ( quoted ? LineParsingState::UnfinishedQuotedCode :
				LineParsingState::UnfinishedCode );

		return i;
	}; // parseCode

	// Read URL in <...>
	auto parseUrl = [&]( int i, const QString & line, QString & text ) -> int
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
	auto parseLine = [&]( QString & line, LineParsingState prev ) -> LineParsingState
	{
		bool hasBreakLine = line.endsWith( QLatin1String( "  " ) );

		int pos = 0;

		if( prev != LineParsingState::Finished )
		{
			pos = parseCode( 0, line, prev );

			if( prev != LineParsingState::Finished )
				return prev;
		}

		const auto ns = posOfFirstNonSpace( line );

		if( ns > 0 )
			line = line.right( line.length() - ns );

		// Will skip horizontal rules, for now at least...
		if( !isHorizontalLine( line ) )
		{
			QString text;

			for( int i = pos, length = line.length(); i < length; ++i )
			{
				if( line[ i ] == c_92 && i + 1 < length &&
					specialChars.contains( line[ i + 1 ] ) )
				{
					++i;

					text.append( line[ i ] );
				}
				else if( line[ i ] == c_33 && i + 1 < length &&
					line[ i + 1 ] == c_91 )
				{
					createTextObj( text.simplified() );
					text.clear();

					bool ok = false;

					const int startPos = i;

					i = parseImg( i, line, &ok );

					if( !ok )
						text.append( line.mid( startPos, i - startPos ) );
				}
				else if( line[ i ] == c_91 )
				{
					createTextObj( text.simplified() );
					text.clear();
					i = parseLnk( i, line, text );
				}
				else if( line[ i ] == c_96 )
				{
					createTextObj( text.simplified() );
					text.clear();
					i = parseCode( i, line, prev ) - 1;

					if( prev != LineParsingState::Finished )
						return prev;
				}
				else if( line[ i ] == c_60 )
				{
					createTextObj( text.simplified() );
					text.clear();
					i = parseUrl( i, line, text ) - 1;
				}
				else if( line[ i ] == c_42 || line[ i ] == c_95 )
				{
					QString style;

					while( i < length &&
						( line[ i ] == c_42 || line[ i ] == c_95 ) )
					{
						style.append( line[ i ] );
						++i;
					}

					if( !style.isEmpty() )
						--i;

					if( style == QLatin1String( "*" ) || style == QLatin1String( "_" ) )
					{
						createTextObj( text.simplified() );
						text.clear();
						data.lexems.append( Lex::Italic );
					}
					else if( style == QLatin1String( "**" ) || style == QLatin1String( "__" ) )
					{
						createTextObj( text.simplified() );
						text.clear();
						data.lexems.append( Lex::Bold );
					}
					else if( style == QLatin1String( "***" ) || style == QLatin1String( "___" ) ||
						style == QLatin1String( "_**" ) || style == QLatin1String( "**_" ) ||
						style == QLatin1String( "*__" ) || style == QLatin1String( "__*" ) )
					{
						createTextObj( text.simplified() );
						text.clear();
						data.lexems.append( Lex::BoldAndItalic );
					}
					else
						text.append( style );
				}
				else if( line[ i ] == c_126 && i + 1 < length &&
					line[ i + 1 ] == c_126 )
				{
					++i;
					createTextObj( text.simplified() );
					text.clear();
					data.lexems.append( Lex::Strikethrough );
				}
				else
					text.append( line[ i ] );
			}

			createTextObj( text.simplified() );
			text.clear();

			if( hasBreakLine )
				data.lexems.append( Lex::BreakLine );
		}

		return LineParsingState::Finished;
	}; // parseLine

	LineParsingState state = LineParsingState::Finished;

	// Real parsing.
	for( auto it = fr.begin(), last = fr.end(); it != last; ++it )
		state = parseLine( *it, state );

	// Set flags for all nested items.
	auto setFlags = [&]( Lex lex, QVector< Lex >::iterator it )
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
				setFlags( *it, it );
			}
				break;

			case Lex::StartOfCode :
			case Lex::StartOfQuotedCode :
			{
				auto end = std::find( it + 1, data.lexems.end(), *it );

				if( end != data.lexems.end() )
				{
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
				}
				else
				{
					auto text = data.txt[ data.processedText ]->text();
					text.prepend( *it == Lex::StartOfCode ?
						QLatin1String( "`" ) : QLatin1String( "``" ) );
					data.txt[ data.processedText ]->setText( text );
				}
			}
				break;

			case Lex::Text :
			{
				parent->appendItem( data.txt[ data.processedText ] );
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

void
Parser::parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
{
	QSharedPointer< Blockquote > bq( new Blockquote() );

	const int pos = fr.first().indexOf( c_62 );

	StringListStream stream( fr );

	if( pos > -1 )
	{
		for( auto it = fr.begin(), last = fr.end(); it != last; ++it )
			*it = it->mid( it->indexOf( c_62 ) + 1 );

		parse( stream, bq, doc, linksToParse, workingPath, fileName );
	}

	if( !bq->isEmpty() )
		parent->appendItem( bq );
}

inline bool
isListItemAndNotNested( const QString & s )
{
	qsizetype p = 0;

	for( ; p < s.size(); ++p )
	{
		if( !s[ p ].isSpace() )
			break;
	}

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

void
Parser::parseList( QStringList & fr, QSharedPointer< Block > parent,
	QSharedPointer< Document > doc, QStringList & linksToParse,
	const QString & workingPath, const QString & fileName )
{
	for( auto it = fr.begin(), last  = fr.end(); it != last; ++it )
		it->replace( c_9, QLatin1String( "    " ) );

	const auto indent = posOfFirstNonSpace( fr.first() );

	if( indent > -1 )
	{
		QSharedPointer< List > list( new List() );

		QStringList listItem;
		auto it = fr.begin();

		*it = it->right( it->length() - indent );

		listItem.append( *it );

		++it;

		for( auto last = fr.end(); it != last; ++it )
		{
			auto s = posOfFirstNonSpace( *it );
			s = ( s > indent ? indent : ( s >= 0 ? s : 0 ) );

			*it = it->right( it->length() - s );

			if( isListItemAndNotNested( *it ) )
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
		return calculateIndent( s, p + 2 );
	else if( s[ p ] == c_45 && s[ p + 1 ].isSpace() )
		return calculateIndent( s, p + 2 );
	else if( s[ p ] == c_43 && s[ p + 1 ].isSpace() )
		return calculateIndent( s, p + 2 );
	else
	{
		int d = 0;

		if( isOrderedList( s, &d ) )
			return calculateIndent( s, p + QString::number( d ).size() + 2 );
		else
			return { -1, 0 };
	}
}

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

	data.append( fr.first().right( fr.first().length() - indent ) );

	for( auto last = fr.end(); it != last; ++it, ++pos )
	{
		const auto i = listItemData( *it ).first;

		if( i > -1 )
		{
			StringListStream stream( data );

			parse( stream, item, doc, linksToParse, workingPath, fileName, true );

			data.clear();

			QStringList nestedList = fr.mid( pos );

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

		parse( stream, item, doc, linksToParse, workingPath, fileName, true );
	}

	if( !item->isEmpty() )
		parent->appendItem( item );
}

void
Parser::parseCode( QStringList & fr, QSharedPointer< Block > parent, int indent )
{
	const auto i = posOfFirstNonSpace( fr.first() );

	if( i > -1 )
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
Parser::fileExists( const QString & fileName, const QString & workingPath ) const
{
	return QFileInfo::exists( workingPath + fileName );
}

} /* namespace MD */
