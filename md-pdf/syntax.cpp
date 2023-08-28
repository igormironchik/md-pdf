
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

// KF6SyntaxHighlighting include.
#include <state.h>

// Qt include.
#include <QPair>


//
// Syntax
//

Syntax::Syntax()
{
	const auto defs = repository.definitions();

	for( const auto & d : defs )
	{
		if( d.name() == QStringLiteral( "C++" ) )
		{
			definitions.insert( QStringLiteral( "c++" ), d );
			definitions.insert( QStringLiteral( "cpp" ), d );

		}
		else
			definitions.insert( d.name().toLower(), d );
	}

	const auto th = repository.themes();

	for( const auto & t : th )
		themes.insert( t.name(), t );
}

KSyntaxHighlighting::Definition
Syntax::definitionForName( const QString & name ) const
{
	static KSyntaxHighlighting::Definition defaultDefinition;

	if( definitions.contains( name.toLower() ) )
		return definitions[ name.toLower() ];
	else
		return defaultDefinition;
}

KSyntaxHighlighting::Theme
Syntax::themeForName( const QString & name ) const
{
	static KSyntaxHighlighting::Theme defaultTheme;

	if( themes.contains( name ) )
		return themes[ name ];
	else
		return defaultTheme;
}

void
Syntax::applyFormat( int offset, int length, const KSyntaxHighlighting::Format & format )
{
	currentColors.push_back( { currentLineNumber, offset, offset + length - 1, format } );
}

Syntax::Colors
Syntax::prepare( const QStringList & lines )
{
	KSyntaxHighlighting::State st;
	currentLineNumber = 0;
	currentColors.clear();

	for( const auto & s : qAsConst( lines ) )
	{
		st = highlightLine( s, st );
		++currentLineNumber;
	}

	return currentColors;
}
