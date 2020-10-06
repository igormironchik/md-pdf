
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
nextWord( const QString & line, int startPos, int stopPos,
	const QString & special )
{
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

//! \return Position of string.
int
stringPos( const QString & line, int startPos, const QString & what )
{
	return line.indexOf( what, startPos );
}

//! Find comment position.
QPair< int, CommentType >
commentPos( const QString & line, int startPos,
	const QString & startMultiComment,
	//! Empty if not used.
	const QString & startSingleComment )
{
	const auto mi = stringPos( line, startPos, startMultiComment );

	if( mi != -1 )
		return qMakePair( mi, CommentType::Multi );

	if( !startSingleComment.isEmpty() )
	{
		const auto si = stringPos( line, startPos, startSingleComment );

		if( si != -1 )
			return qMakePair( si, CommentType::Single );
	}

	return qMakePair( line.length(), CommentType::NoComment );
}

//! Find multiline comment end.
QPair< int, bool >
multilineCommentEnd( const QString & line, int startPos,
	const QString & endMultiComment )
{
	const auto mi = stringPos( line, startPos, endMultiComment );

	if( mi != -1 )
		return qMakePair( mi + endMultiComment.length(), true );

	return qMakePair( line.length(), false );
}

//! C++ keywords.
static const QStringList c_cppKeyWords = {
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

//! \return Prepared colors.
Syntax::Colors
prepareColors( const QStringList & lines, const QString & startMultiComment,
	const QString & endMultiComment, const QString & startSingleComment,
	const QString & special, const QStringList & keywords )
{
	Syntax::Colors ret;

	int lineIdx = 0;

	bool commentClosed = true;

	for( const auto & l : qAsConst( lines ) )
	{
		int pos = 0;
		int end = 0;

		if( !commentClosed )
		{
			const auto cEnd = multilineCommentEnd( l, 0, endMultiComment );

			if( cEnd.second )
			{
				pos = cEnd.first;
				end = pos;

				ret.append( { lineIdx, 0, cEnd.first - 1, Syntax::ColorRole::Comment } );

				commentClosed = true;
			}
			else
			{
				ret.append( { lineIdx, 0, l.length() - 1, Syntax::ColorRole::Comment } );

				++lineIdx;

				continue;
			}
		}

		auto c = commentPos( l, pos, startMultiComment, startSingleComment );

		while( c.first <= l.length() )
		{
			while( pos < ( c.second != CommentType::NoComment ? c.first : l.length() ) )
			{
				const auto w = nextWord( l, pos, c.first, special );

				if( keywords.contains( w.word ) )
				{
					pos = w.startPos;

					if( pos != end )
						ret.append( { lineIdx, end, pos - 1, Syntax::ColorRole::Regular } );

					end = w.endPos + 1;

					ret.append( { lineIdx, w.startPos, w.endPos, Syntax::ColorRole::Keyword } );
				}

				pos = w.endPos + 1;
			}

			if( pos != end )
				ret.append( { lineIdx, end,
					( c.second != CommentType::NoComment ? c.first - 1 : l.length() - 1 ),
					Syntax::ColorRole::Regular } );

			if( c.second == CommentType::NoComment )
				break;

			if( c.second == CommentType::Single )
			{
				ret.append( { lineIdx, c.first, l.length() - 1, Syntax::ColorRole::Comment } );

				break;
			}

			if( c.second == CommentType::Multi )
			{
				bool doBreak = false;

				while( true )
				{
					auto cEnd = multilineCommentEnd( l, c.first + startMultiComment.length(),
						endMultiComment );

					pos = cEnd.first;
					end = pos;

					if( cEnd.second )
					{
						commentClosed = true;

						if( cEnd.first == l.length() )
						{
							ret.append( { lineIdx, c.first, cEnd.first - 1,
								Syntax::ColorRole::Comment } );

							doBreak = true;

							break;
						}
						else
						{
							ret.append( { lineIdx, c.first, cEnd.first - 1,
								Syntax::ColorRole::Comment } );

							c = commentPos( l, cEnd.first, startMultiComment, startSingleComment );

							if( c.first == cEnd.first )
							{
								if( c.second == CommentType::Single )
								{
									ret.append( { lineIdx, c.first, l.length() - 1,
										Syntax::ColorRole::Comment } );

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

						ret.append( { lineIdx, c.first, l.length() - 1,
							Syntax::ColorRole::Comment } );

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

} /* namespace anonymous */


//
// CppSyntax
//

Syntax::Colors
CppSyntax::prepare( const QStringList & lines ) const
{
	return prepareColors( lines, QStringLiteral( "/*" ), QStringLiteral( "*/" ),
		QStringLiteral( "//" ), QStringLiteral( ":,.-/*-+=<>!&()~%^|?[]{}" ),
		c_cppKeyWords );
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
