
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

#include <md-pdf/renderer.hpp>

#include <QVector>

static const QVector< DrawPrimitive >  c_testTableWithImagesData = {
	{ DrawPrimitive::Type::MultilineText, QStringLiteral( "Table" ), 50.0000000000000000, 766.3906250000000000, 0.0, 0.0, 495.0000000000000000, 25.6093750000000000, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 52.0000000000000000, 745.7656250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 79.2890625000000000, 745.7656250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Sample" ), 481.9531250000000000, 745.7656250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Pictures" ), 512.1171875000000000, 745.7656250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 757.0781250000000000, 545.0000000000000000, 757.0781250000000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 741.8789062500000000, 545.0000000000000000, 741.8789062500000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 757.0781250000000000, 50.0000000000000000, 741.8789062500000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 757.0781250000000000, 297.5000000000000000, 741.8789062500000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 757.0781250000000000, 545.0000000000000000, 741.8789062500000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 577.5252812500000346, 0.0, 0.0, 0.0, 0.0, 0.0608750000000000, 0.0608750000000000 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 577.5455729166666288, 0.0, 0.0, 0.0, 0.0, 0.0469714506172840, 0.0469714506172840 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 741.8789062500000000, 545.0000000000000000, 741.8789062500000000, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 573.6385625000000346, 545.0000000000000000, 573.6385625000000346, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 741.8789062500000000, 50.0000000000000000, 573.6385625000000346, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 741.8789062500000000, 297.5000000000000000, 573.6385625000000346, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 741.8789062500000000, 545.0000000000000000, 573.6385625000000346, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 409.7110625000000255, 0.0, 0.0, 0.0, 0.0, 0.0676388888888889, 0.0676388888888889 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 419.4510625000000346, 0.0, 0.0, 0.0, 0.0, 0.0494115259740260, 0.0494115259740260 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 573.6385625000000346, 545.0000000000000000, 573.6385625000000346, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 405.8243437500000255, 545.0000000000000000, 405.8243437500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 573.6385625000000346, 50.0000000000000000, 405.8243437500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 573.6385625000000346, 297.5000000000000000, 405.8243437500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 573.6385625000000346, 545.0000000000000000, 405.8243437500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 52.0000000000000000, 394.5118437500000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 79.2890625000000000, 394.5118437500000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 491.6171875000000000, 394.5118437500000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 518.9062500000000000, 394.5118437500000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 405.8243437500000255, 545.0000000000000000, 405.8243437500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 390.6251250000000255, 545.0000000000000000, 390.6251250000000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 405.8243437500000255, 50.0000000000000000, 390.6251250000000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 405.8243437500000255, 297.5000000000000000, 390.6251250000000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 405.8243437500000255, 545.0000000000000000, 390.6251250000000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 206.0001250000000255, 0.0, 0.0, 0.0, 0.0, 0.0469714506172840, 0.0469714506172840 },
	{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 226.2917916666666827, 0.0, 0.0, 0.0, 0.0, 0.0444992690058480, 0.0444992690058480 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 390.6251250000000255, 545.0000000000000000, 390.6251250000000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 202.1134062500000255, 545.0000000000000000, 202.1134062500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 390.6251250000000255, 50.0000000000000000, 202.1134062500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 390.6251250000000255, 297.5000000000000000, 202.1134062500000255, 0.0, 0.0, 0.0, 0.0 },
	{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 390.6251250000000255, 545.0000000000000000, 202.1134062500000255, 0.0, 0.0, 0.0, 0.0 }
}; // c_testTableWithImagesData
