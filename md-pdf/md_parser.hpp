
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

#ifndef MD_PDF_MD_PARSER_HPP_INCLUDED
#define MD_PDF_MD_PARSER_HPP_INCLUDED

// md-pdf include.
#include "md_doc.hpp"

// Qt include.
#include <QTextStream>

// C++ include.
#include <stdexcept>


namespace MD {

//
// ParserException
//

class ParserException final
	:	public std::logic_error
{
public:
	explicit ParserException( const QString & reason );

	const QString & reason() const noexcept;

private:
	QString m_what;
}; // class ParserException

static const QChar c_35 = QLatin1Char( '#' );
static const QChar c_46 = QLatin1Char( '.' );
static const QChar c_41 = QLatin1Char( ')' );
static const QChar c_96 = QLatin1Char( '`' );
static const QChar c_126 = QLatin1Char( '~' );
static const QChar c_9 = QLatin1Char( '\t' );
static const QChar c_32 = QLatin1Char( ' ' );
static const QChar c_62 = QLatin1Char( '>' );
static const QChar c_45 = QLatin1Char( '-' );
static const QChar c_43 = QLatin1Char( '+' );
static const QChar c_42 = QLatin1Char( '*' );
static const QChar c_91 = QLatin1Char( '[' );
static const QChar c_94 = QLatin1Char( '^' );
static const QChar c_93 = QLatin1Char( ']' );
static const QChar c_58 = QLatin1Char( ':' );
static const QChar c_124 = QLatin1Char( '|' );
static const QChar c_92 = QLatin1Char( '\\' );
static const QChar c_125 = QLatin1Char( '}' );
static const QChar c_61 = QLatin1Char( '=' );
static const QChar c_95 = QLatin1Char( '_' );
static const QChar c_34 = QLatin1Char( '"' );
static const QChar c_40 = QLatin1Char( '(' );
static const QChar c_33 = QLatin1Char( '!' );
static const QChar c_60 = QLatin1Char( '<' );
static const QChar c_10 = QLatin1Char( '\n' );
static const QChar c_13 = QLatin1Char( '\r' );


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

bool fileExists( const QString & fileName, const QString & workingPath );


//
// Parser
//

//! MD parser.
class Parser final
{
public:
	Parser() = default;
	~Parser() = default;

	QSharedPointer< Document > parse( const QString & fileName, bool recursive = true );

private:
	void parseFile( const QString & fileName, bool recursive, QSharedPointer< Document > doc,
		QStringList * parentLinks = nullptr );
	void clearCache();

	enum class BlockType {
		Unknown,
		Text,
		List,
		CodeIndentedBySpaces,
		Code,
		Blockquote,
		Heading
	}; // enum BlockType

	BlockType whatIsTheLine( QString & str, bool inList = false, qsizetype * indent = nullptr,
		bool calcIndent = false ) const;
	void parseFragment( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseText( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseList( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseCode( QStringList & fr, QSharedPointer< Block > parent, int indent = 0 );
	void parseCodeIndentedBySpaces( QStringList & fr, QSharedPointer< Block > parent,
		int indent = 4, const QString & syntax = QString() );
	void parseListItem( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseHeading( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseFootnote( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseTable( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	void parseParagraph( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	bool parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );

	// Read line from stream.
	template< typename STREAM >
	QString readLine( STREAM & stream, bool & commentFound )
	{
		static const QString c_startComment = QLatin1String( "<!--" );
		static const QString c_endComment = QLatin1String( "-->" );

		auto line = stream.readLine();

		bool searchEndFromBegining = commentFound;

		auto cs = line.indexOf( c_startComment );

		if( !commentFound && cs > -1 )
			commentFound = true;

		if( cs == -1 )
			cs = 0;

		while( commentFound && !stream.atEnd() )
		{
			auto ce = line.indexOf( c_endComment,
				( searchEndFromBegining ? 0 : cs + c_startComment.length() ) );

			searchEndFromBegining = false;

			if( ce > -1 )
			{
				if( cs + c_startComment.length() < ce )
				{
					line.remove( cs, ce + c_endComment.length() - cs );

					commentFound = false;
				}
				else if( commentFound )
				{
					line.remove( 0, ce + c_endComment.length() );

					commentFound = false;
				}
			}
			else if( cs > 0 )
				return line.left( cs );
			else
				return readLine( stream, commentFound );

			cs = line.indexOf( c_startComment );

			if( !commentFound && cs > -1 )
				commentFound = true;
		}

		if( commentFound )
			return QString();
		else
			return line;
	};

	template< typename STREAM >
	void parse( STREAM & stream, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc, QStringList & linksToParse,
		const QString & workingPath, const QString & fileName,
		bool skipSpacesAtStartOfLine = false )
	{
		QStringList fragment;

		BlockType type = BlockType::Unknown;
		bool emptyLineInList = false;
		bool firstLine = true;
		qsizetype spaces = 0;

		// Parse fragment and clear internal cache.
		auto pf = [&]()
			{
				parseFragment( fragment, parent, doc, linksToParse, workingPath, fileName );
				fragment.clear();
				type = BlockType::Unknown;
				emptyLineInList = false;
			};

		bool commentFound = false;

		auto rl = [&]() -> QString
		{
			auto line = readLine( stream, commentFound );

			if( skipSpacesAtStartOfLine )
			{
				line.replace( c_9, QLatin1String( "    " ) );

				if( firstLine )
				{				
					spaces = posOfFirstNonSpace( line );
					if( spaces < 0 ) spaces = 0;

					firstLine = false;
				}

				line = line.right( line.length() - spaces );
			}

			return line;
		};

		// Eat footnote.
		auto eatFootnote = [&]()
			{
				while( !stream.atEnd() )
				{
					auto line = rl();

					if( line.isEmpty() || line.startsWith( QLatin1String( "    " ) ) ||
						line.startsWith( c_9 ) )
					{
						fragment.append( line );
					}
					else
					{
						pf();

						type = whatIsTheLine( line );
						fragment.append( line );

						break;
					}
				}

				if( stream.atEnd() && !fragment.isEmpty() )
					pf();
			};

		qsizetype indent = 0;

		while( !stream.atEnd() )
		{
			auto line = rl();

			const auto ns = posOfFirstNonSpace( line );

			BlockType lineType = whatIsTheLine( line, emptyLineInList, &indent, true );

			// First line of the fragment.
			if( ns != -1 && type == BlockType::Unknown )
			{
				type = lineType;

				fragment.append( line );

				if( type == BlockType::Heading )
					pf();

				continue;
			}
			else if( ns == -1 && type == BlockType::Unknown )
				continue;

			// Got new empty line.
			if( ns == -1 )
			{
				switch( type )
				{
					case BlockType::Text :
					{
						if( isFootnote( fragment.first() ) )
						{
							fragment.append( QString() );

							eatFootnote();
						}
						else
							pf();

						continue;
					}

					case BlockType::Blockquote :
					{
						pf();

						continue;
					}

					case BlockType::CodeIndentedBySpaces :
					{
						if( line.startsWith( QLatin1String( "    " ) ) ||
							line.startsWith( c_9 ) )
						{
							fragment.append( line );
						}
						else
							pf();

						continue;
					}

					case BlockType::Code :
					{
						fragment.append( line );

						continue;
					}

					case BlockType::List :
					{
						emptyLineInList = true;

						continue;
					}

					default :
						break;
				}
			}
			//! Empty new line in list.
			else if( emptyLineInList )
			{
				if( ( indent > 0 && line.startsWith( QString( indent, c_32 ) ) ) ||
					lineType == BlockType::List )
				{
					fragment.append( QString() );
					fragment.append( line );

					emptyLineInList = false;

					continue;
				}
				else
				{
					pf();

					type = lineType;
					fragment.append( line );

					continue;
				}
			}

			// Something new and this is not a code block or a list.
			if( type != lineType && type != BlockType::Code && type != BlockType::List &&
				type != BlockType::Blockquote )
			{
				if( type == BlockType::Text && lineType == BlockType::CodeIndentedBySpaces )
					fragment.append( line );
				else
				{
					pf();

					type = lineType;

					if( !line.isEmpty() )
						fragment.append( line );
				}
			}
			// End of code block.
			else if( type == BlockType::Code && type == lineType )
			{
				fragment.append( line );

				pf();
			}
			else
				fragment.append( line );
		}

		if( !fragment.isEmpty() )
			pf();
	}

	//! Wrapper for QStringList to be behaved like a stream.
	class StringListStream final
	{
	public:
		StringListStream( QStringList & stream )
			:	m_stream( stream )
			,	m_pos( 0 )
		{
		}

		bool atEnd() const { return ( m_pos >= m_stream.size() ); }
		QString readLine() { return m_stream.at( m_pos++ ); }

	private:
		QStringList & m_stream;
		int m_pos;
	}; // class StringListStream

	//! Wrapper for QTextStream.
	class TextStream final
	{
	public:
		TextStream( QTextStream & stream )
			:	m_stream( stream )
			,	m_lastBuf( false )
			,	m_pos( 0 )
		{
		}

		bool atEnd() const { return ( m_lastBuf && m_pos == m_buf.size() ); }

		QString readLine()
		{
			QString line;
			bool rFound = false;

			while( !atEnd() )
			{
				const auto c = getChar();

				if( rFound && c != c_10 )
				{
					--m_pos;

					return line;
				}

				if( c == c_13 )
				{
					rFound = true;

					continue;
				}
				else if( c == c_10 )
					return line;

				if( !c.isNull() )
					line.append( c );
			}

			return line;
		}

	private:
		void fillBuf()
		{
			m_buf = m_stream.read( 512 );

			if( m_stream.atEnd() )
				m_lastBuf = true;

			m_pos = 0;
		}

		QChar getChar()
		{
			if( m_pos < m_buf.size() )
				return m_buf.at( m_pos++ );
			else if( !atEnd() )
			{
				fillBuf();

				return getChar();
			}
			else
				return QChar();
		}

	private:
		QTextStream & m_stream;
		QString m_buf;
		bool m_lastBuf;
		qsizetype m_pos;
	}; // class TextStream

private:
	QStringList m_parsedFiles;

	Q_DISABLE_COPY( Parser )
}; // class Parser

} /* namespace MD */

#endif // MD_PDF_MD_PARSER_HPP_INCLUDED
