
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
#include <QTextCodec>


namespace MD {

//
// Parser
//

//! MD parser.
class Parser final
{
public:
	Parser() = default;
	~Parser() = default;

	QSharedPointer< Document > parse( const QString & fileName, bool recursive = true,
		QTextCodec * codec = QTextCodec::codecForName( "UTF-8" ) );

private:
	void parseFile( const QString & fileName, bool recursive, QSharedPointer< Document > doc,
		QTextCodec * codec, QStringList * parentLinks = nullptr );
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

	BlockType whatIsTheLine( const QString & str, bool inList = false ) const;
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
		int indent = 4 );
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
	void parseFormattedTextLinksImages( QStringList & fr, QSharedPointer< Block > parent,
		QSharedPointer< Document > doc,
		QStringList & linksToParse, const QString & workingPath,
		const QString & fileName );
	bool fileExists( const QString & fileName, const QString & workingPath ) const;

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
		int spaces = 0;

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
				line.replace( QLatin1Char( '\t' ), QLatin1String( "    " ) );

				if( firstLine )
				{
					static const QRegExp s( QLatin1String( "[^\\s]" ) );

					spaces = s.indexIn( line );

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
						line.startsWith( QLatin1Char( '\t' ) ) )
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

		static const QRegExp footnoteRegExp( QLatin1String( "\\s*\\[\\^[^\\s]*\\]:.*" ) );


		while( !stream.atEnd() )
		{
			auto line = rl();

			auto simplified = line.simplified();

			BlockType lineType = whatIsTheLine( line, emptyLineInList );

			// First line of the fragment.
			if( !simplified.isEmpty() && type == BlockType::Unknown )
			{
				type = lineType;

				fragment.append( line );

				if( type == BlockType::Heading )
					pf();

				continue;
			}
			else if( simplified.isEmpty() && type == BlockType::Unknown )
				continue;

			// Got new empty line.
			if( simplified.isEmpty() )
			{
				switch( type )
				{
					case BlockType::Text :
					{
						if( footnoteRegExp.exactMatch( fragment.first() ) )
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
							line.startsWith( QLatin1Char( '\t' ) ) )
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
				if( line.startsWith( QLatin1String( "    " ) ) ||
					line.startsWith( QLatin1Char( '\t' ) )  ||
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
			if( type != lineType && type != BlockType::Code && type != BlockType::List )
			{
				pf();

				type = lineType;

				if( !line.isEmpty() )
					fragment.append( line );
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
		{
		}

		bool atEnd() const { return ( m_stream.atEnd() ); }
		QString readLine()
		{
			QString line;
			bool rFound = false;

			QChar c = m_buf;
			m_buf = QChar();

			if( !c.isNull() && c != QLatin1Char( '\r' ) )
				line.append( c );

			if( c == QLatin1Char( '\r' ) )
				rFound = true;

			while( !m_stream.atEnd() )
			{
				m_stream >> c;

				if( rFound && c != QLatin1Char( '\n' ) )
				{
					m_buf = c;

					return line;
				}

				if( c == QLatin1Char( '\r' ) )
				{
					rFound = true;

					continue;
				}
				else if( c == QLatin1Char( '\n' ) )
					return line;

				if( !c.isNull() )
					line.append( c );
			}

			return line;
		}

	private:
		QTextStream & m_stream;
		QChar m_buf;
	}; // class QTextStream

private:
	QStringList m_parsedFiles;

	Q_DISABLE_COPY( Parser )
}; // class Parser

} /* namespace MD */

#endif // MD_PDF_MD_PARSER_HPP_INCLUDED
