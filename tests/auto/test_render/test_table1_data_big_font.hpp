
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

static const QVector< DrawPrimitive >  c_testTableWithImagesBigFontData = {
{ DrawPrimitive::Type::MultilineText, QStringLiteral( "Table" ), 50.0000000000000000, 757.0781250000000000, 0.0, 0.0, 495.0000000000000000, 34.9218750000000000, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 52.0000000000000000, 717.8281250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 106.5781250000000000, 717.8281250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Sample" ), 420.9062500000000000, 717.8281250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Pictures" ), 481.2343750000000000, 717.8281250000000000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 738.4531250000000000, 545.0000000000000000, 738.4531250000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 712.0546875000000000, 545.0000000000000000, 712.0546875000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 738.4531250000000000, 50.0000000000000000, 712.0546875000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 738.4531250000000000, 297.5000000000000000, 712.0546875000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 738.4531250000000000, 545.0000000000000000, 712.0546875000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 547.7010625000000346, 0.0, 0.0, 0.0, 0.0, 0.0608750000000000, 0.0608750000000000 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 547.7213541666666288, 0.0, 0.0, 0.0, 0.0, 0.0469714506172840, 0.0469714506172840 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 712.0546875000000000, 545.0000000000000000, 712.0546875000000000, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 541.9276250000000346, 545.0000000000000000, 541.9276250000000346, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 712.0546875000000000, 50.0000000000000000, 541.9276250000000346, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 712.0546875000000000, 297.5000000000000000, 541.9276250000000346, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 712.0546875000000000, 545.0000000000000000, 541.9276250000000346, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 378.0001250000000255, 0.0, 0.0, 0.0, 0.0, 0.0676388888888889, 0.0676388888888889 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 387.7401250000000346, 0.0, 0.0, 0.0, 0.0, 0.0494115259740260, 0.0494115259740260 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 541.9276250000000346, 545.0000000000000000, 541.9276250000000346, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 372.2266875000000255, 545.0000000000000000, 372.2266875000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 541.9276250000000346, 50.0000000000000000, 372.2266875000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 541.9276250000000346, 297.5000000000000000, 372.2266875000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 541.9276250000000346, 545.0000000000000000, 372.2266875000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 52.0000000000000000, 351.6016875000000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 106.5781250000000000, 351.6016875000000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Lorem" ), 440.2343750000000000, 351.6016875000000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Text, QStringLiteral( "Ipsum" ), 494.8125000000000000, 351.6016875000000255, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 372.2266875000000255, 545.0000000000000000, 372.2266875000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 345.8282500000000255, 545.0000000000000000, 345.8282500000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 372.2266875000000255, 50.0000000000000000, 345.8282500000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 372.2266875000000255, 297.5000000000000000, 345.8282500000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 372.2266875000000255, 545.0000000000000000, 345.8282500000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 52.0000000000000000, 161.2032500000000255, 0.0, 0.0, 0.0, 0.0, 0.0469714506172840, 0.0469714506172840 },
{ DrawPrimitive::Type::Image, QStringLiteral( "" ), 299.5000000000000000, 181.4949166666666827, 0.0, 0.0, 0.0, 0.0, 0.0444992690058480, 0.0444992690058480 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 345.8282500000000255, 545.0000000000000000, 345.8282500000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 155.4298125000000255, 545.0000000000000000, 155.4298125000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 50.0000000000000000, 345.8282500000000255, 50.0000000000000000, 155.4298125000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 297.5000000000000000, 345.8282500000000255, 297.5000000000000000, 155.4298125000000255, 0.0, 0.0, 0.0, 0.0 },
{ DrawPrimitive::Type::Line, QStringLiteral( "" ), 545.0000000000000000, 345.8282500000000255, 545.0000000000000000, 155.4298125000000255, 0.0, 0.0, 0.0, 0.0 }
}; // c_testTableWithImagesBigFontData
