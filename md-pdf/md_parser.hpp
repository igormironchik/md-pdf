
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
#include <set>
#include <vector>


namespace MD {

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
static const QChar c_39 = QLatin1Char( '\'' );


//! Skip spaces in line from pos \a i.
qsizetype
skipSpaces( qsizetype i, QStringView line );

//! \return Is string a footnote?
bool
isFootnote( const QString & s );

//! \return Starting sequence of the same characters.
QString
startSequence( const QString & line );

//! \return Is string a code fences?
bool
isCodeFences( const QString & s, bool closing = false );

//! \return Is file exist?
bool
fileExists( const QString & fileName, const QString & workingPath );

//! \return Is string an ordered list.
bool
isOrderedList( const QString & s, int * num = nullptr, int * len = nullptr,
	QChar * delim = nullptr, bool * isFirstLineEmpty = nullptr );

inline bool
indentInList( const std::set< qsizetype > * indents, qsizetype indent )
{
	if( indents )
		return ( indents->find( indent ) != indents->cend() );
	else
		return false;
};

// \return Is sequence of emphasis closed, and closing index of the sequence?
std::pair< bool, size_t >
checkEmphasisSequence( const std::vector< std::pair< qsizetype, int > > & s, size_t idx );

//! \return Is string a start of code?
bool
isStartOfCode( QStringView str, QString * syntax = nullptr );

//! \return Is string a horizontal line?
bool
isHorizontalLine( QStringView s );

//! \return Is string a column alignment?
bool
isColumnAlignment( const QString & s );

//! \return Is string a table alignment?
bool
isTableAlignment( const QString & s );


//
// Parser
//

//! MD parser.
class Parser final
{
public:
	Parser() = default;
	~Parser() = default;

	//! \return Parsed Markdown document.
	QSharedPointer< Document > parse( const QString & fileName, bool recursive = true );

private:
	void parseFile( const QString & fileName, bool recursive, QSharedPointer< Document > doc,
		QStringList * parentLinks = nullptr );
	void clearCache();

	enum class BlockType {
		Unknown,
		Text,
		List,
		ListWithFirstEmptyLine,
		CodeIndentedBySpaces,
		Code,
		Blockquote,
		Heading
	}; // enum BlockType

	BlockType whatIsTheLine( QString & str, bool inList = false, qsizetype * indent = nullptr,
		bool calcIndent = false, bool wasComment = false,
		const std::set< qsizetype > * indents = nullptr ) const;
	void parseFragment( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseText( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseBlockquote( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseList( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseCode( QStringList & fr, QSharedPointer< Block > parent,
		bool collectRefLinks, int indent = 0 );
	void parseCodeIndentedBySpaces( QStringList & fr, QSharedPointer< Block > parent,
		bool collectRefLinks,
		int indent = 4, const QString & syntax = QString() );
	void parseListItem( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseHeading( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName,
		bool collectRefLinks );
	void parseFootnote( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseTable( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseParagraph( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks, bool ignoreLineBreak );

	// Read line from stream.
	template< typename STREAM >
	QPair< QString, bool >
	readLine( STREAM & stream, bool & unfinishedCommentFound )
	{
		static const QString c_startComment = QLatin1String( "<!--" );
		static const QString c_endComment = QLatin1String( "-->" );

		auto line = stream.readLine();

		bool searchEndFromBegining = unfinishedCommentFound;

		bool wasComment = unfinishedCommentFound;

		auto cs = line.indexOf( c_startComment );

		if( !unfinishedCommentFound && cs > -1 )
		{
			unfinishedCommentFound = true;
			wasComment = true;
		}

		if( cs == -1 )
			cs = 0;

		while( unfinishedCommentFound && !stream.atEnd() )
		{
			auto ce = line.indexOf( c_endComment,
				( searchEndFromBegining ? 0 : cs + c_startComment.length() ) );

			searchEndFromBegining = false;

			if( ce > -1 )
			{
				if( cs + c_startComment.length() < ce )
				{
					line.remove( cs, ce + c_endComment.length() - cs );

					unfinishedCommentFound = false;
				}
				else if( unfinishedCommentFound )
				{
					line.remove( 0, ce + c_endComment.length() );

					unfinishedCommentFound = false;
				}
			}
			else if( cs > 0 )
				return { line.left( cs ), false };
			else
				return readLine( stream, unfinishedCommentFound );

			cs = line.indexOf( c_startComment );

			if( !unfinishedCommentFound && cs > -1 )
				unfinishedCommentFound = true;
		}

		if( unfinishedCommentFound )
			return { QString(), wasComment };
		else
			return { line, wasComment };
	};

	template< typename STREAM >
	void parse( STREAM & stream, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc, QStringList & linksToParse,
		const QString & workingPath, const QString & fileName,
		bool collectRefLinks,
		bool top = false )
	{
		QVector< QStringList > splitted;

		QStringList fragment;

		BlockType type = BlockType::Unknown;
		bool emptyLineInList = false;
		qsizetype emptyLinesInList = 0;
		qsizetype emptyLinesInCode = 0;
		bool firstLine = true;
		qsizetype spaces = 0;
		qsizetype lineCounter = 0;
		std::set< qsizetype > indents;
		qsizetype indent = 0;

		// Parse fragment and clear internal cache.
		auto pf = [&]()
			{
				splitted.append( fragment );
				parseFragment( fragment, parent, doc, linksToParse,
					workingPath, fileName, collectRefLinks );
				fragment.clear();
				type = BlockType::Unknown;
				emptyLineInList = false;
				emptyLinesInList = 0;
				lineCounter = 0;
				indents.clear();
				indent = 0;
			};

		bool unfinishedCommentFound = false;
		bool wasComment = false;

		auto rl = [&]() -> QString
		{
			QString line;

			std::tie( line, wasComment ) = readLine( stream, unfinishedCommentFound );

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

		QString startOfCode;
		bool split = false;

		while( !stream.atEnd() )
		{
			auto line = rl();

			if( wasComment )
				split = true;

			const qsizetype prevIndent = indent;

			BlockType lineType = whatIsTheLine( line, emptyLineInList, &indent,
				true, split, &indents );

			if( prevIndent != indent )
				indents.insert( indent );

			const auto ns = skipSpaces( 0, line );

			if( ( lineType == BlockType::ListWithFirstEmptyLine ||
				lineType == BlockType::List || lineType == BlockType::CodeIndentedBySpaces ) &&
				( type == BlockType::ListWithFirstEmptyLine || type == BlockType::List ) )
			{
				if( split )
				{
					if( !fragment.isEmpty() )
						pf();

					emptyLineInList = false;
					emptyLinesInList = 0;
					type = lineType;
					fragment.append( line );
					split = false;

					continue;
				}
			}

			if( ns != line.length() )
				split = false;

			if( type == BlockType::CodeIndentedBySpaces && ns > 3 )
				lineType = BlockType::CodeIndentedBySpaces;

			if( type == BlockType::ListWithFirstEmptyLine && lineCounter == 2 &&
				lineType != BlockType::ListWithFirstEmptyLine && lineType != BlockType::List )
			{
				if( emptyLinesInList )
				{
					pf();

					fragment.append( line );
					type = lineType;

					continue;
				}
				else
				{
					emptyLineInList = false;
					emptyLinesInList = 0;
				}
			}

			if( type == BlockType::ListWithFirstEmptyLine && lineCounter == 2 )
				type = BlockType::List;

			if( lineType == BlockType::ListWithFirstEmptyLine )
			{
				type = lineType;
				lineCounter = 1;
				fragment.append( line );

				continue;
			}

			// First line of the fragment.
			if( ns != line.length() && type == BlockType::Unknown )
			{
				type = lineType;

				++lineCounter;

				if( type == BlockType::Code )
					startOfCode = startSequence( line );

				fragment.append( line );

				if( type == BlockType::Heading )
					pf();

				continue;
			}
			else if( ns == line.length() && type == BlockType::Unknown )
				continue;

			++lineCounter;

			// Got new empty line.
			if( ns == line.length() )
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
						++emptyLinesInCode;

						continue;
					}

					case BlockType::Code :
					{
						fragment.append( line );

						continue;
					}

					case BlockType::List :
					case BlockType::ListWithFirstEmptyLine :
					{
						emptyLineInList = true;
						++emptyLinesInList;

						continue;
					}

					default :
						break;
				}
			}
			//! Empty new line in list.
			else if( emptyLineInList )
			{
				if( indentInList( &indents, ns ) || lineType == BlockType::List ||
					lineType == BlockType::CodeIndentedBySpaces )
				{
					for( qsizetype i = 0; i < emptyLinesInList; ++i )
						fragment.append( QString() );

					fragment.append( line );

					emptyLineInList = false;
					emptyLinesInList = 0;

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
			else if( emptyLinesInCode )
			{
				if( lineType == BlockType::CodeIndentedBySpaces )
				{
					const auto indent = skipSpaces( 0, fragment.first() );

					for( qsizetype i = 0; i < emptyLinesInCode; ++i )
						fragment.append( QString( indent, c_32 ) );

					fragment.append( line );
				}
				else
				{
					pf();

					type = lineType;

					fragment.append( line );
				}

				emptyLinesInCode = 0;

				continue;
			}

			// Something new and this is not a code block or a list, blockquote.
			if( type != lineType && type != BlockType::Code && type != BlockType::List &&
				type != BlockType::Blockquote && type != BlockType::ListWithFirstEmptyLine )
			{
				if( type == BlockType::Text && lineType == BlockType::CodeIndentedBySpaces )
					fragment.append( line );
				else
				{
					if( type == BlockType::Text &&
						( lineType == BlockType::ListWithFirstEmptyLine ||
							lineType == BlockType::List ) )
					{
						int num = 0;

						if( isOrderedList( line, &num ) )
						{
							if( num > 1 )
							{
								fragment.append( line );

								continue;
							}
						}
					}

					pf();

					type = lineType;

					if( !line.isEmpty() )
						fragment.append( line );
				}
			}
			// End of code block.
			else if( type == BlockType::Code && type == lineType &&
				startSequence( line ).contains( startOfCode ) &&
				isCodeFences( line, true ) )
			{
				fragment.append( line );

				pf();
			}
			else
				fragment.append( line );
		}

		if( !fragment.isEmpty() )
		{
			if( type == BlockType::Code )
				fragment.append( startOfCode );

			pf();
		}

		if( top )
		{
			for( qsizetype i = 0; i < splitted.size(); ++i )
			{
				parseFragment( splitted[ i ], parent, doc, linksToParse,
					workingPath, fileName, false );
			}
		}
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
