
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

#ifndef MD_PDF_SYNTAX_HPP_INCLUDED
#define MD_PDF_SYNTAX_HPP_INCLUDED

// Qt include.
#include <QString>
#include <QVector>
#include <QSharedPointer>


//
// Syntax
//

//! Base class for syntax highlighters.
class Syntax
{
protected:
	Syntax() = default;

public:
	virtual ~Syntax() = default;

	//! Color role.
	enum class ColorRole {
		Regular,
		Keyword,
		Comment
	}; // enum class ColorRole

	//! Color for the text.
	struct Color {
		//! Index of line.
		qsizetype line;
		//! Start position of text.
		qsizetype startPos;
		//! End position of text.
		qsizetype endPos;
		//! Color role.
		ColorRole color;
	}; // struct Color

	//! Vector of colored text auxiliary structs.
	using Colors = QVector< Color >;

	//! \return Vector of colored text auxiliary structs.
	virtual Colors prepare( const QStringList & lines ) const;

	//! \return Concrete syntax highlighter.
	static QSharedPointer< Syntax > createSyntaxHighlighter( const QString & language );
}; // class Syntax


//
// CppSyntax
//

//! Syntax for C++.
class CppSyntax final
	:	public Syntax
{
protected:
	friend class Syntax;

	CppSyntax() = default;

public:
	//! \return Vector of colored text auxiliary structs.
	Colors prepare( const QStringList & lines ) const override;
}; // class CppSyntax


//
// JavaSyntax
//

//! Syntax for C++.
class JavaSyntax final
	:	public Syntax
{
protected:
	friend class Syntax;

	JavaSyntax() = default;

public:
	//! \return Vector of colored text auxiliary structs.
	Colors prepare( const QStringList & lines ) const override;
}; // class JavaSyntax

#endif // MD_PDF_SYNTAX_HPP_INCLUDED
