
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
static const QChar c_47 = QLatin1Char( '/' );
static const QChar c_63 = QLatin1Char( '?' );

static const QString c_startComment = QLatin1String( "<!--" );
static const QString c_endComment = QLatin1String( "-->" );


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

struct RawHtmlBlock {
	QSharedPointer< RawHtml > html = {};
	int htmlBlockType = -1;
	bool continueHtml = false;
	bool onLine = false;
}; // struct RawHtmlBlock

struct MdBlock {
	QStringList data;
	qsizetype emptyLinesBefore = 0;
	bool emptyLineAfter = true;
}; // struct MdBlock


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
		bool calcIndent = false, const std::set< qsizetype > * indents = nullptr ) const;
	void parseFragment( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseText( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseBlockquote( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseList( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseCode( MdBlock & fr, QSharedPointer< Block > parent,
		bool collectRefLinks, int indent = 0 );
	void parseCodeIndentedBySpaces( MdBlock & fr, QSharedPointer< Block > parent,
		bool collectRefLinks,
		int indent = 4, const QString & syntax = QString() );
	void parseListItem( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseHeading( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName,
		bool collectRefLinks );
	void parseFootnote( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseTable( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks );
	void parseParagraph( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks,
		RawHtmlBlock & html );
	void parseFormattedTextLinksImages( MdBlock & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName, bool collectRefLinks, bool ignoreLineBreak,
		RawHtmlBlock & html );

	template< typename STREAM >
	void parse( STREAM & stream, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc, QStringList & linksToParse,
		const QString & workingPath, const QString & fileName,
		bool collectRefLinks,
		bool top = false )
	{
		QVector< MdBlock > splitted;

		QStringList fragment;

		BlockType type = BlockType::Unknown;
		bool emptyLineInList = false;
		qsizetype emptyLinesCount = 0;
		bool firstLine = true;
		qsizetype spaces = 0;
		qsizetype lineCounter = 0;
		std::set< qsizetype > indents;
		qsizetype indent = 0;
		RawHtmlBlock html;
		qsizetype emptyLinesBefore = 0;

		// Parse fragment and clear internal cache.
		auto pf = [&]()
			{
				if( !fragment.isEmpty() )
				{
					MdBlock block = { fragment, emptyLinesBefore, emptyLinesCount > 0 };

					emptyLinesBefore = emptyLinesCount;

					splitted.append( block );

					parseFragment( block, parent, doc, linksToParse,
						workingPath, fileName, collectRefLinks, html );

					fragment.clear();
				}

				type = BlockType::Unknown;
				emptyLineInList = false;
				emptyLinesCount = 0;
				lineCounter = 0;
				indents.clear();
				indent = 0;
			};

		// Eat footnote.
		auto eatFootnote = [&]()
			{
				while( !stream.atEnd() )
				{
					auto line = stream.readLine();

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

		while( !stream.atEnd() )
		{
			auto line = stream.readLine();

			const qsizetype prevIndent = indent;

			BlockType lineType = whatIsTheLine( line, emptyLineInList, &indent,
				true, &indents );

			if( prevIndent != indent )
				indents.insert( indent );

			const auto ns = skipSpaces( 0, line );

			if( type == BlockType::CodeIndentedBySpaces && ns > 3 )
				lineType = BlockType::CodeIndentedBySpaces;

			if( type == BlockType::ListWithFirstEmptyLine && lineCounter == 2 &&
				lineType != BlockType::ListWithFirstEmptyLine && lineType != BlockType::List )
			{
				if( emptyLinesCount > 0 )
				{
					pf();

					fragment.append( line );
					type = lineType;

					continue;
				}
				else
				{
					emptyLineInList = false;
					emptyLinesCount = 0;
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
				++emptyLinesCount;

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
						{
							pf();

							if( html.htmlBlockType >= 6 )
								html.continueHtml = ( emptyLinesCount <= 0 );
						}

						continue;
					}

					case BlockType::Blockquote :
					{
						pf();

						continue;
					}

					case BlockType::CodeIndentedBySpaces :
						continue;
						break;

					case BlockType::Code :
					{
						fragment.append( line );
						emptyLinesCount = 0;

						continue;
					}

					case BlockType::List :
					case BlockType::ListWithFirstEmptyLine :
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
				if( indentInList( &indents, ns ) || lineType == BlockType::List ||
					lineType == BlockType::CodeIndentedBySpaces )
				{
					for( qsizetype i = 0; i < emptyLinesCount; ++i )
						fragment.append( QString() );

					fragment.append( line );

					emptyLineInList = false;
					emptyLinesCount = 0;

					continue;
				}
				else
				{
					pf();

					type = lineType;
					fragment.append( line );
					emptyLinesCount = 0;

					continue;
				}
			}
			else if( emptyLinesCount > 0 )
			{
				if( lineType == BlockType::CodeIndentedBySpaces )
				{
					const auto indent = skipSpaces( 0, fragment.first() );

					for( qsizetype i = 0; i < emptyLinesCount; ++i )
						fragment.append( QString( indent, c_32 ) );

					fragment.append( line );
				}
				else
				{
					pf();

					type = lineType;
					fragment.append( line );
				}

				emptyLinesCount = 0;

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

					if( html.htmlBlockType >= 6 )
						html.continueHtml = ( emptyLinesCount <= 0 );

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

			emptyLinesCount = 0;
		}

		if( !fragment.isEmpty() )
		{
			if( type == BlockType::Code )
				fragment.append( startOfCode );

			pf();
		}

		auto finishHtml = [&] ()
		{
			if( html.html->isFreeTag() )
				doc->appendItem( html.html );
			else
			{
				if( doc->items().back()->type() == ItemType::Paragraph )
				{
					auto p = static_cast< Paragraph* > ( doc->items().back().data() );

					if( p->isDirty() )
						p->appendItem( html.html );
					else
					{
						QSharedPointer< Paragraph > p( new Paragraph );
						p->appendItem( html.html );
						doc->appendItem( p );
					}
				}
				else
				{
					QSharedPointer< Paragraph > p( new Paragraph );
					p->appendItem( html.html );
					doc->appendItem( p );
				}
			}

			html.html.reset( nullptr );
		};

		if( top )
		{
			html.html.reset( nullptr );
			html.htmlBlockType = -1;
			html.continueHtml = false;

			for( qsizetype i = 0; i < splitted.size(); ++i )
			{
				parseFragment( splitted[ i ], parent, doc, linksToParse,
					workingPath, fileName, false, html );

				if( html.htmlBlockType >= 6 )
					html.continueHtml = ( !splitted[ i ].emptyLineAfter );

				if( !html.html.isNull() && !html.continueHtml )
					finishHtml();
			}

			if( !html.html.isNull() )
				finishHtml();
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
