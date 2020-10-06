
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

// md-pdf include.
#include "syntax.hpp"

// Qt include.
#include <QPair>


//
// Syntax
//

//! \return Vector of colored text auxiliary structs.
Syntax::Colors
Syntax::prepare( const QStringList & lines ) const
{
	Colors ret;

	int i = 0;

	for( const auto & s : qAsConst( lines ) )
	{
		ret.append( { i, 0, s.length() - 1, ColorRole::Regular } );
		++i;
	}

	return ret;
}

QSharedPointer< Syntax >
Syntax::createSyntaxHighlighter( const QString & language )
{
	if( language.toLower() == QStringLiteral( "cpp" ) )
		return QSharedPointer< Syntax > ( new CppSyntax );
	else if( language.toLower() == QStringLiteral( "java" ) )
		return QSharedPointer< Syntax > ( new JavaSyntax );
	else
		return QSharedPointer< Syntax > ( new Syntax );
}

namespace /* anonymous */ {

//! Word in the string.
struct Word {
	QString word;
	int startPos;
	int endPos;
}; // struct Word

//! Find next word in the string.
Word
nextWord( const QString & line, int startPos, int stopPos )
{
	static const QString special = QStringLiteral( ":,.-/*-+=<>!&()~%^|?[]{}" );
	Word w;

	while( startPos < stopPos && ( line[ startPos ].isSpace() ||
		special.contains( line[ startPos ] ) ) )
			++startPos;

	int i = startPos;

	for( ; i < stopPos; ++i )
	{
		if( line[ i ].isSpace() || special.contains( line[ i ] ) )
			break;
	}

	w.startPos = startPos;
	w.endPos = i - 1;
	w.word = line.mid( startPos, i - startPos );

	return w;
}

//! Comment type.
enum class CommentType {
	NoComment,
	Single,
	Multi
}; // enum class CommentType

//! Find comment position.
QPair< int, CommentType >
commentPos( const QString & line, int startPos )
{
	for( int i = startPos; i < line.length(); ++i )
	{
		if( line[ i ] == QLatin1Char( '/' ) && i + 1 < line.length() )
		{
			if( line[ i + 1 ] == QLatin1Char( '/' ) )
				return qMakePair( i, CommentType::Single );

			if( line[ i + 1 ] == QLatin1Char( '*' ) )
				return qMakePair( i, CommentType::Multi );
		}
	}

	return qMakePair( line.length(), CommentType::NoComment );
}

//! Find multiline comment end.
QPair< int, bool >
multilineCommentEnd( const QString & line, int startPos )
{
	for( int i = startPos; i < line.length(); ++i )
	{
		if( line[ i ] == QLatin1Char( '*' ) && i + 1 < line.length() &&
			line[ i + 1 ] == QLatin1Char( '/' ) )
				return qMakePair( i + 2, true );
	}

	return qMakePair( line.length(), false );
}

//! C++ keywords.
static const QVector< QString > c_cppKeyWords = {
	QStringLiteral( "asm" ),
	QStringLiteral( "auto" ),
	QStringLiteral( "bool" ),
	QStringLiteral( "break" ),
	QStringLiteral( "case" ),
	QStringLiteral( "catch" ),
	QStringLiteral( "char" ),
	QStringLiteral( "class" ),
	QStringLiteral( "const" ),
	QStringLiteral( "continue" ),
	QStringLiteral( "default" ),
	QStringLiteral( "delete" ),
	QStringLiteral( "do" ),
	QStringLiteral( "double" ),
	QStringLiteral( "else" ),
	QStringLiteral( "enum" ),
	QStringLiteral( "explicit" ),
	QStringLiteral( "export" ),
	QStringLiteral( "extern" ),
	QStringLiteral( "false" ),
	QStringLiteral( "float" ),
	QStringLiteral( "for" ),
	QStringLiteral( "friend" ),
	QStringLiteral( "goto" ),
	QStringLiteral( "if" ),
	QStringLiteral( "inline" ),
	QStringLiteral( "int" ),
	QStringLiteral( "long" ),
	QStringLiteral( "mutable" ),
	QStringLiteral( "namespace" ),
	QStringLiteral( "new" ),
	QStringLiteral( "operator" ),
	QStringLiteral( "private" ),
	QStringLiteral( "protected" ),
	QStringLiteral( "public" ),
	QStringLiteral( "register" ),
	QStringLiteral( "return" ),
	QStringLiteral( "short" ),
	QStringLiteral( "signed" ),
	QStringLiteral( "sizeof" ),
	QStringLiteral( "static" ),
	QStringLiteral( "struct" ),
	QStringLiteral( "switch" ),
	QStringLiteral( "template" ),
	QStringLiteral( "this" ),
	QStringLiteral( "throw" ),
	QStringLiteral( "true" ),
	QStringLiteral( "try" ),
	QStringLiteral( "typedef" ),
	QStringLiteral( "typeid" ),
	QStringLiteral( "typename" ),
	QStringLiteral( "union" ),
	QStringLiteral( "unsigned" ),
	QStringLiteral( "using" ),
	QStringLiteral( "virtual" ),
	QStringLiteral( "void" ),
	QStringLiteral( "volatile" ),
	QStringLiteral( "wchar_t" ),
	QStringLiteral( "while" )
};

} /* namespace anonymous */


//
// CppSyntax
//

Syntax::Colors
CppSyntax::prepare( const QStringList & lines ) const
{
	Colors ret;

	int lineIdx = 0;

	bool commentClosed = true;

	for( const auto & l : qAsConst( lines ) )
	{
		int pos = 0;
		int end = 0;

		if( !commentClosed )
		{
			const auto cEnd = multilineCommentEnd( l, 0 );

			if( cEnd.second )
			{
				pos = cEnd.first;
				end = pos;

				ret.append( { lineIdx, 0, cEnd.first - 1, ColorRole::Comment } );

				commentClosed = true;
			}
			else
			{
				ret.append( { lineIdx, 0, l.length() - 1, ColorRole::Comment } );

				++lineIdx;

				continue;
			}
		}

		auto c = commentPos( l, pos );

		while( c.first <= l.length() )
		{
			while( pos < ( c.second != CommentType::NoComment ? c.first : l.length() ) )
			{
				const auto w = nextWord( l, pos, c.first );

				if( c_cppKeyWords.contains( w.word ) )
				{
					pos = w.startPos;

					if( pos != end )
						ret.append( { lineIdx, end, pos - 1, ColorRole::Regular } );

					end = w.endPos + 1;

					ret.append( { lineIdx, w.startPos, w.endPos, ColorRole::Keyword } );
				}

				pos = w.endPos + 1;
			}

			if( pos != end )
				ret.append( { lineIdx, end,
					( c.second != CommentType::NoComment ? c.first - 1 : l.length() - 1 ),
					ColorRole::Regular } );

			if( c.second == CommentType::NoComment )
				break;

			if( c.second == CommentType::Single )
			{
				ret.append( { lineIdx, c.first, l.length() - 1, ColorRole::Comment } );

				break;
			}

			if( c.second == CommentType::Multi )
			{
				bool doBreak = false;

				while( true )
				{
					auto cEnd = multilineCommentEnd( l, c.first + 2 );

					pos = cEnd.first;
					end = pos;

					if( cEnd.second )
					{
						commentClosed = true;

						if( cEnd.first == l.length() )
						{
							ret.append( { lineIdx, c.first, cEnd.first - 1, ColorRole::Comment } );

							doBreak = true;

							break;
						}
						else
						{
							ret.append( { lineIdx, c.first, cEnd.first - 1, ColorRole::Comment } );

							c = commentPos( l, cEnd.first );

							if( c.first == cEnd.first )
							{
								if( c.second == CommentType::Single )
								{
									ret.append( { lineIdx, c.first, l.length() - 1, ColorRole::Comment } );

									doBreak = true;

									break;
								}
							}
							else
								break;
						}
					}
					else
					{
						commentClosed = false;

						ret.append( { lineIdx, c.first, l.length() - 1, ColorRole::Comment } );

						doBreak = true;

						break;
					}
				}

				if( doBreak )
					break;
			}
		}

		++lineIdx;
	}

	return ret;
}


//
// JavaSyntax
//

Syntax::Colors
JavaSyntax::prepare( const QStringList & lines ) const
{
	Colors ret;

	return ret;
}
