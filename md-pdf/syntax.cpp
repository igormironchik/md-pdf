
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
Word nextWord( const QString & line, int startPos )
{
	static const QString special = QStringLiteral( ":,.-/*-+=<>!&()~%^|?[]" );
	Word w;

	while( startPos < line.length() && ( line[ startPos ].isSpace() ||
		special.contains( line[ startPos ] ) ) )
			++startPos;

	int i = startPos;

	for( ; i < line.length(); ++i )
	{
		if( line[ i ].isSpace() || special.contains( line[ i ] ) )
			break;
	}

	w.startPos = startPos;
	w.endPos = i - 1;
	w.word = line.mid( startPos, i - startPos );

	return w;
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

	for( const auto & l : qAsConst( lines ) )
	{
		int pos = 0;
		int end = 0;

		while( pos < l.length() )
		{
			const auto w = nextWord( l, pos );

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
			ret.append( { lineIdx, end, l.length() - 1, ColorRole::Regular } );

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
