/*
    Copyright (c) 2008-2022 Jan W. Krieger (<jan@jkrieger.de>)
    with contributions from: Razi Alavizadeh

    

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




#ifndef JKQTMATHTEXTLISTNODE_H
#define JKQTMATHTEXTLISTNODE_H
#include "jkqtmathtext/jkqtmathtext_imexport.h"
#include "jkqtmathtext/jkqtmathtexttools.h"
#include "jkqtmathtext/nodes/jkqtmathtextnode.h"
#include <QPainter>
#include <QFontMetricsF>

class JKQTMathText; // forward
// JKQTMATHTEXT_LIB_EXPORT


/** \brief subclass representing a list of nodes in the syntax tree, layed out horizontally
 *  \ingroup jkqtmathtext_items
 *
 *  \note This type of node also implements typesetting sub-/superscript above/below the previous node (if
 *        JKQTMathTextNode::is() is \c true for that previus node. The drawing is done as defined in the
 *        following image \image html jkqtmathtext_subsuper_with_limits.png
 *
 *  \see JKQTMathTextVerticalListNode
 */
class JKQTMATHTEXT_LIB_EXPORT JKQTMathTextHorizontalListNode: public JKQTMathTextMultiChildNode {
    public:
        explicit JKQTMathTextHorizontalListNode(JKQTMathText* parent);
        virtual ~JKQTMathTextHorizontalListNode() override;
        /** \copydoc JKQTMathTextNode::getTypeName() */
        virtual QString getTypeName() const override;
        /** \copydoc JKQTMathTextNode::draw() */
        virtual double draw(QPainter& painter, double x, double y, JKQTMathTextEnvironment currentEv, const JKQTMathTextNodeSize* prevNodeSize=nullptr) override;
        /** \copydoc JKQTMathTextNode::toHtml() */
        virtual bool toHtml(QString& html, JKQTMathTextEnvironment currentEv, JKQTMathTextEnvironment defaultEv) override;
        /** \brief add a child node */
        void addChild(JKQTMathTextNode* n);
        /** \copydoc JKQTMathTextMultiChildNode::getChildren() */
        virtual QList<JKQTMathTextNode*> getChildren() override;
        /** \copydoc JKQTMathTextMultiChildNode::childCount() */
        virtual int childCount() const override;
        /** \copydoc JKQTMathTextMultiChildNode::clearChildren() */
        virtual void clearChildren(bool deleteChildren=true) override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual JKQTMathTextNode* getChild(int i) override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual const JKQTMathTextNode* getChild(int i) const override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual JKQTMathTextNode* replaceChild(int i, JKQTMathTextNode* newChild) override;
    protected:
        /** \copydoc JKQTMathTextNode::getSizeInternal() */
        virtual void getSizeInternal(QPainter& painter, JKQTMathTextEnvironment currentEv, double& width, double& baselineHeight, double& overallHeight, double& strikeoutPos, const JKQTMathTextNodeSize* prevNodeSize=nullptr) override;
        QList<JKQTMathTextNode*> nodes;
    private:
        /** \brief  internal implementation of clearChildren() that is non-virtual, so can be used in the destructor */
        void clearChildrenImpl(bool deleteChildren);
};



/** \brief subclass representing a list of nodes in the syntax tree, layed out vertically
 *  \ingroup jkqtmathtext_items
 *
 *  Each child can be thought of as a line, so this node represents a list of lines.
 *  The layout of the lines can left-aligned, right-aligned or centered.
 *
 *  \image html jkqtmathtext_verticallist.png
 *
 *  \image html jkqtmathtext_verticalalignment.png
 *
 *  \image html jkqtmathtext_horizontalalignment.png
 *
 *  \see JKQTMathTextHorizontalListNode
 */
class JKQTMATHTEXT_LIB_EXPORT JKQTMathTextVerticalListNode: public JKQTMathTextMultiChildNode {
    public:
        /** \brief defines, how lines are beeing spaced by the node
         *
         *  \image html jkqtmathtext_verticallist.png
         */
        enum SpacingMode {
            SMDefault, /*!< space the lines with equilibrated spacing, i.e. the baselines are at least \c QFontMetricsF::lineSpacing()*JKQTMathTextVerticalListNode::lineSpacingFactor apart, but even more, if the height of the text bloxk is larger than the the font's ascent+descent */
            SMMinimal /*!< space the lines as tight as possible, i.e. each line is separated by \c QFontMetricsF::leading()*JKQTMathTextVerticalListNode::lineSpacingFactor from the next line. This is a s compact as possible */
        };
        /** \brief convert a SpacingMode to a String */
        static QString SpacingMode2String(SpacingMode mode);
        /** \brief convert a String to a SpacingMode */
        static SpacingMode String2SpacingMode(QString mode);

        explicit JKQTMathTextVerticalListNode(JKQTMathText* parent, JKQTMathTextHorizontalAlignment _alignment=MTHACentered, double _linespacingFactor=1.0, SpacingMode spacingMode_=SMDefault, JKQTMathTextVerticalOrientation _verticalOrientation=MTVOFirstLine);
        virtual ~JKQTMathTextVerticalListNode() override;
        /** \copydoc JKQTMathTextNode::getTypeName() */
        virtual QString getTypeName() const override;
        /** \copydoc JKQTMathTextNode::draw() */
        virtual double draw(QPainter& painter, double x, double y, JKQTMathTextEnvironment currentEv, const JKQTMathTextNodeSize* prevNodeSize=nullptr) override;
        /** \copydoc JKQTMathTextNode::toHtml() */
        virtual bool toHtml(QString& html, JKQTMathTextEnvironment currentEv, JKQTMathTextEnvironment defaultEv) override;
        /** \brief add a child node */
        void addChild(JKQTMathTextNode* n);
        /** \copydoc JKQTMathTextMultiChildNode::getChildren() */
        virtual QList<JKQTMathTextNode*> getChildren() override;
        /** \copydoc JKQTMathTextMultiChildNode::childCount() */
        virtual int childCount() const override;
        /** \copydoc JKQTMathTextMultiChildNode::clearChildren() */
        virtual void clearChildren(bool deleteChildren=true) override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual JKQTMathTextNode* getChild(int i) override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual const JKQTMathTextNode* getChild(int i) const override;
        /** \copydoc JKQTMathTextMultiChildNode::getChild() */
        virtual JKQTMathTextNode* replaceChild(int i, JKQTMathTextNode* newChild) override;
        /** \copydoc alignment */
        JKQTMathTextHorizontalAlignment getAlignment() const;
        /** \copydoc verticalOrientation */
        JKQTMathTextVerticalOrientation getVerticalOrientation() const;
        /** \copydoc lineSpacingFactor */
        double getLineSpacingFactor() const;
        /** \copydoc spacingMode */
        SpacingMode getSpacingMode() const;
    protected:
        /** \copydoc JKQTMathTextNode::getSizeInternal() */
        virtual void getSizeInternal(QPainter& painter, JKQTMathTextEnvironment currentEv, double& width, double& baselineHeight, double& overallHeight, double& strikeoutPos, const JKQTMathTextNodeSize* prevNodeSize=nullptr) override;

        /** \brief describes the layout of the whole node */
        struct LayoutInfo: public JKQTMathTextNodeSize {
            LayoutInfo();
            /** \brief drawing position for each line */
            QList<QPointF> X;
        };
        /** \brief calclates the layout of the whole block/node */
        LayoutInfo calcLayout(QPainter& painter, JKQTMathTextEnvironment currentEv) const;

        /** \brief list of child nodes, each representing one line */
        QList<JKQTMathTextNode*> nodes;
        /** \brief alignment scheme used to lay out all lines
          *
          * \image html jkqtmathtext_horizontalalignment.png
          */
        JKQTMathTextHorizontalAlignment alignment;
        /** \brief spacing of the separate lines, as factor of the default line-spacing [Default: 1].
         *
         *  This property can be used to move the lines closer together or farther apart.
         *
         *  \image html jkqtmathtext_verticallist.png
         */
        double lineSpacingFactor;
        /** \brief vertical orientation of the baseline of the whole block (with respect to the single lines)
         *
         *  \image html jkqtmathtext_verticalorientation.png
         */
        JKQTMathTextVerticalOrientation verticalOrientation;
        /** \brief defines how the layout algorithm (see calcLayout() ) lays out the single lines */
        SpacingMode spacingMode;



    private:
        /** \brief  internal implementation of clearChildren() that is non-virtual, so can be used in the destructor */
        void clearChildrenImpl(bool deleteChildren);
};



#endif // JKQTMATHTEXTLISTNODE_H









