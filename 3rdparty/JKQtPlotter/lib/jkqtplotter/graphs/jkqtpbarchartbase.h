/*
    Copyright (c) 2008-2022 Jan W. Krieger (<jan@jkrieger.de>)

    

    This software is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License (LGPL) as published by
    the Free Software Foundation, either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License (LGPL) for more details.

    You should have received a copy of the GNU Lesser General Public License (LGPL)
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef jkqtpgraphsbarchartbase_H
#define jkqtpgraphsbarchartbase_H

#include <QString>
#include <QPainter>
#include <QPair>
#include "jkqtplotter/jkqtptools.h"
#include "jkqtplotter/jkqtplotter_imexport.h"
#include "jkqtplotter/jkqtpimagetools.h"
#include "jkqtplotter/jkqtpgraphsbase.h"
#include "jkqtplotter/jkqtpgraphsbaseerrors.h"
#include "jkqtplotter/jkqtpgraphsbasestylingmixins.h"


/** \brief This is a base-class for all bar graphs with vertical or horizontal orientation (the orientation is implemented in dervied classes!)
 *  \ingroup jkqtplotter_barssticks
 *
 *  This class plots a bargraph. This image explains the parameters:
 *
 *  \image html bargraph_basics.png
 *
 *  By default the sift parameter is, so the bar is centered at the x-value. The width is 0.9,
 *  so adjacent bars are plotted with a small distance between them. It is possible to use these two parameters
 *  to plot multiple bars for every x-value, by having on JKQTPSpecialLineHorizontalGraph object per
 *  set of bars that belong together. For example for three bars per x-value one would set:
 *  \verbatim
 *        width=0.3
 *        shift=-0.3 / 0 / +0.3
 *  \endverbatim
 *  This results in a bargraph, as shown here:
 *
 *  \image html JKQTPBarVerticalGraph.png
 *
 *  You can also set FillMode::TwoColorFilling, which uses different fill styles for bars above and below
 *  the baseline of the graph:
 *
 *  \image html JKQTPBarVerticalGraphTwoColorFilling.png
 *
 *  \see JKQTPBarHorizontalGraph, JKQTPBarVerticalGraph
 */
class JKQTPLOTTER_LIB_EXPORT JKQTPBarGraphBase: public JKQTPXYBaselineGraph, public JKQTPGraphLineStyleMixin, public JKQTPGraphFillStyleMixin {
        Q_OBJECT
    public:
        /** \brief specifies how the area below the graph is filled
         *
         *  \see setFillMode(), getFillMode(), fillStyleBelow(), \ref JKQTPlotterWigglePlots
         */
        enum FillMode {
            SingleFilling=0, /*!< \brief the whole area is filled with the same color/pattern \image html JKQTPBarVerticalGraph.png */
            TwoColorFilling=1 /*!< \brief the area above and below baseline with the two different colors/pattern \image html JKQTPBarVerticalGraphTwoColorFilling.png */
        };
        Q_ENUM(FillMode)
        /** \brief class constructor */
        JKQTPBarGraphBase(JKQTBasePlotter* parent=nullptr);
        /** \brief class constructor */
        JKQTPBarGraphBase(JKQTPlotter* parent);

        /** \brief plots a key marker inside the specified rectangle \a rect */
        virtual void drawKeyMarker(JKQTPEnhancedPainter& painter, QRectF& rect) override;
        /** \brief returns the color to be used for the key label */
        virtual QColor getKeyLabelColor() const override;



        /** \copydoc shift */
        double getShift() const;
        /** \copydoc width */
        double getWidth() const;

        /** \brief sets the fill color and the color together, where fillColor is set to \a fill and the line-color is set to \c fill.darker(colorDarker)
         *  \see setColor()
         */
        void setFillColor_and_darkenedColor(QColor fill, int colorDarker=200);
		
		/** \brief returns xColumn or yColumn, whichever is used for the position of the bars (depending on whether the barchart is vertical or horizontal \see getBarHeightColumn(), xColumn, yColumn */
		virtual int getBarPositionColumn() const =0;
		
		/** \brief returns xColumn or yColumn, whichever is used for the height of the bars (depending on whether the barchart is vertical or horizontal \see getBarPositionColumn(), xColumn, yColumn */
		virtual int getBarHeightColumn() const =0;
        /** \copydoc m_fillStyleBelow */
        JKQTPGraphFillStyleMixin &fillStyleBelow();
        /** \copydoc m_fillStyleBelow */
        const JKQTPGraphFillStyleMixin& fillStyleBelow() const;
        /** \copydoc m_fillMode */
        FillMode getFillMode() const;
        /** \copydoc rectRadiusAtValue */
        double getRectRadiusAtValue() const;
        /** \copydoc rectRadiusAtBaseline */
        double getRectRadiusAtBaseline() const;

    public slots:
        /** \copydoc m_fillMode */
        void setFillMode(JKQTPBarGraphBase::FillMode mode);

        /** \brief finds all bar charts of the same orientation and determines width and shift, so they stand side by side
         *
         *  \param maxWidth the maximum (relative) width, that all bars will span of the (doubled) inter-bar distance
         *  \param shrinkFactor factor, by which the bar are shrinked compared to the available space
         *
         *  \note This function will scale ALL graphs of the parent plot, which were derived from JKQTPBarHorizontalGraph, that match in orientation (as returned by isHorizontal() ).
         */
        virtual void autoscaleBarWidthAndShift(double maxWidth=0.9, double shrinkFactor=0.8);

        /** \brief equivalent to \c autoscaleBarWidthAndShift(groupWidth,1);
         */
        void autoscaleBarWidthAndShiftSeparatedGroups(double groupWidth=0.75);
        /** \copydoc shift */
        void setShift(double __value);
        /** \copydoc width */
        void setWidth(double __value);

        /** \copydoc rectRadiusAtValue */
        void setRectRadiusAtValue(double __value);
        /** \copydoc rectRadiusAtBaseline */
        void setRectRadiusAtBaseline(double __value);
        /** \brief sets the corner radius of the bars for both ends */
        void setRectRadius(double all);
        /** \brief sets the corner radius of the bars for both ends */
        void setRectRadius(double atValue, double atBaseline);


        /** \brief set outline and fill color at the same time
         *  \see setFillColor_and_darkenedColor()
         */
        virtual void setColor(QColor c);


		/** \brief returns xColumn or yColumn, whichever is used for the position of the bars (depending on whether the barchart is vertical or horizontal \see getBarHeightColumn(), xColumn, yColumn */
        virtual void setBarPositionColumn(int column)  =0;
		
		/** \brief returns xColumn or yColumn, whichever is used for the position of the bars (depending on whether the barchart is vertical or horizontal \see getBarHeightColumn(), xColumn, yColumn */
        virtual void setBarPositionColumn(size_t column)  =0;
		
		/** \brief returns xColumn or yColumn, whichever is used for the height of the bars (depending on whether the barchart is vertical or horizontal \see getBarPositionColumn(), xColumn, yColumn */
        virtual void setBarHeightColumn(int column)  =0;
		
		/** \brief returns xColumn or yColumn, whichever is used for the height of the bars (depending on whether the barchart is vertical or horizontal \see getBarPositionColumn(), xColumn, yColumn */
        virtual void setBarHeightColumn(size_t column)  =0;
    protected:
        /** \brief the width of the bargraphs, relative to the distance between the current and the next x-value
         *
         * See the following graphic to understand this concept:
         *     \image html bargraph_basics.png
         */
        double width;
        /** \brief the shift of the bargraphs, relative to the distance between the current and the next x-value
         *
         * See the following graphic to understand this concept:
         *     \image html bargraph_basics.png
         */
        double shift;
        /** \brief corner radius (in pt) for bars at the "value" end */
        double rectRadiusAtValue;
        /** \brief corner radius (in pt) for bars at the "baseline" end */
        double rectRadiusAtBaseline;
        /** \brief specifies how the area of the graph is filles */
        FillMode m_fillMode;
        /** \brief if m_fillMode \c ==FillAboveAndBelowDifferently then this fill style is used below the baseline and
         *         the default fill style is used above */
        JKQTPGraphFillStyleMixin m_fillStyleBelow;


        /** \brief this function is used by autoscaleBarWidthAndShift() to determine whether a given graph shall be taken into account when autoscaling. 
		 *         Typically this returns \c true for all JKQTPBarGraphBase-derved objects with the same orientation (horizontal or vertical) */
        virtual bool considerForAutoscaling( JKQTPBarGraphBase* other) const=0;

        /** \brief used to generate stacked plots: returns the upper boundary of the parent plot in a stack, for the index-th datapoint */
        virtual double getParentStackedMax(int index) const ;

        /** \brief returns \c true, if a stack parent is set (if available) */
        virtual bool hasStackParent() const ;

        /** \brief get the maximum and minimum value in the box-elongation (i.e. value) direction of the graph
         *
         * The result is given in the two parameters which are call-by-reference parameters!
         */
        bool getValuesMinMax(double& mmin, double& mmax, double& smallestGreaterZero) ;
        /** \brief get the maximum and minimum value of the box positions of the graph
         *
         * The result is given in the two parameters which are call-by-reference parameters!
         */
        bool getPositionsMinMax(double& mmin, double& mmax, double& smallestGreaterZero) ;
};







#endif // jkqtpgraphsbarchartbase_H
