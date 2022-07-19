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



#include "jkqtmathtext/nodes/jkqtmathtextinstructionnode.h"
#include "jkqtmathtext/jkqtmathtexttools.h"
#include "jkqtmathtext/jkqtmathtext.h"
#include "jkqtcommon/jkqtpcodestructuring.h"
#include "jkqtcommon/jkqtpstringtools.h"
#include <cmath>
#include <QFontMetricsF>
#include <QDebug>
#include <QFontDatabase>
#include <QFontInfo>
#include <QApplication>
#include <QFont>





JKQTMathTextInstruction1Node::JKQTMathTextInstruction1Node(JKQTMathText* _parent, const QString& _instructionName, JKQTMathTextNode* child, const QStringList& _parameters):
    JKQTMathTextSingleChildNode(child, _parent),
    instructionName(_instructionName),
    parameters(_parameters)
{
}

JKQTMathTextInstruction1Node::~JKQTMathTextInstruction1Node() {
}

const QString& JKQTMathTextInstruction1Node::getInstructionName() const {
    return this->instructionName;
}

const QStringList &JKQTMathTextInstruction1Node::getParameters() const {
    return this->parameters;
}





JKQTMathTextModifiedTextPropsInstructionNode::JKQTMathTextModifiedTextPropsInstructionNode(JKQTMathText* _parent, const QString& name, JKQTMathTextNode* child, const QStringList& parameters):
    JKQTMathTextInstruction1Node(_parent, name, child, parameters)
{
}

JKQTMathTextModifiedTextPropsInstructionNode::~JKQTMathTextModifiedTextPropsInstructionNode() {
}


QString JKQTMathTextModifiedTextPropsInstructionNode::getTypeName() const
{
    return QLatin1String("JKQTMathTextModifiedTextPropsInstructionNode(")+instructionName+")";
}

void JKQTMathTextModifiedTextPropsInstructionNode::getSizeInternal(QPainter& painter, JKQTMathTextEnvironment currentEv, double& width, double& baselineHeight, double& overallHeight, double& strikeoutPos, const JKQTMathTextNodeSize* /*prevNodeSize*/) {
    JKQTMathTextEnvironment ev=currentEv;

    executeInstruction(ev);

    getChild()->getSize(painter, ev, width, baselineHeight, overallHeight, strikeoutPos);
}

double JKQTMathTextModifiedTextPropsInstructionNode::draw(QPainter& painter, double x, double y, JKQTMathTextEnvironment currentEv, const JKQTMathTextNodeSize* /*prevNodeSize*/) {
    doDrawBoxes(painter, x, y, currentEv);
    JKQTMathTextEnvironment ev=currentEv;

    executeInstruction(ev);

    return getChild()->draw(painter, x, y, ev);
}

bool JKQTMathTextModifiedTextPropsInstructionNode::toHtml(QString &html, JKQTMathTextEnvironment currentEv, JKQTMathTextEnvironment defaultEv) {
    JKQTMathTextEnvironment ev=currentEv;
    fillInstructions();
    executeInstruction(ev);

    return getChild()->toHtml(html, ev, defaultEv);
}

bool JKQTMathTextModifiedTextPropsInstructionNode::supportsInstructionName(const QString &instructionName)
{
    fillInstructions();
    return instructions.contains(instructionName);
}

size_t JKQTMathTextModifiedTextPropsInstructionNode::countParametersOfInstruction(const QString &instructionName)
{
    fillInstructions();
    if (instructions.contains(instructionName)) return instructions[instructionName].NParams;
    return 0;
}

void JKQTMathTextModifiedTextPropsInstructionNode::modifyInMathEnvironment(const QString &instructionName, bool &insideMath, const QStringList& params)
{
    fillInstructions();
    if (instructions.contains(instructionName)) {
        JKQTMathTextEnvironment ev;
        ev.insideMath=insideMath;
        instructions[instructionName].modifier(ev, params);
        insideMath=ev.insideMath;
    }
}


void JKQTMathTextModifiedTextPropsInstructionNode::executeInstruction(JKQTMathTextEnvironment &ev) const
{
    instructions.value(getInstructionName(), InstructionProperties()).modifier(ev, getParameters());
}

QHash<QString, JKQTMathTextModifiedTextPropsInstructionNode::InstructionProperties> JKQTMathTextModifiedTextPropsInstructionNode::instructions;

void JKQTMathTextModifiedTextPropsInstructionNode::fillInstructions()
{

    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.bold=true; }, 0);
        instructions["bf"] = i;
        instructions["textbf"] = i;
        instructions["mathbf"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.italic=!ev.italic; }, 0);
        instructions["em"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.italic=true; }, 0);
        instructions["it"] = i;
        instructions["textit"] = i;
        instructions["mathit"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.color=jkqtp_String2QColor(parameters.value(0, ev.color.name())); }, 1);
        instructions["textcolor"] = i;
        instructions["mathcolor"] = i;
        instructions["color"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.italic=true; ev.insideMath=true; }, 0);
        instructions["ensuremath"] = i;
        instructions["equation"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.smallCaps=true; }, 0);
        instructions["sc"] = i;
        instructions["textsc"] = i;
        instructions["mathsc"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.underlined=true; }, 0);
        instructions["ul"] = i;
        instructions["underline"] = i;
        instructions["underlined"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.overline=true; }, 0);
        instructions["ol"] = i;
        instructions["overline"] = i;
        instructions["overlined"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) {ev.strike=true; }, 0);
        instructions["strike"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEroman; ev.italic=false; }, 0);
        instructions["rm"] = i;
        instructions["textrm"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEroman; ev.italic=false; }, 0);
        instructions["mathrm"] = i;
        instructions["unit"] = i;
        instructions["operatorname"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.bold=true; ev.italic=true; }, 0);
        instructions["mathbfit"] = i;
        instructions["bfit"] = i;
        instructions["textbfit"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.insideMath=false; ev.font=JKQTMathTextEnvironmentFont::MTEroman; ev.italic=false; }, 0);
        instructions["text"] = i;
        instructions["mbox"] = i;
        instructions["ensuretext"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEroman; ev.italic=false; ev.bold=true; }, 0);
        instructions["mat"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEcaligraphic; ev.italic=false; }, 0);
        instructions["cal"] = i;
        instructions["textcal"] = i;
        instructions["mathcal"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEcaligraphic; ev.bold=true; }, 0);
        instructions["fcal"] = i;
        instructions["textfcal"] = i;
        instructions["mathfcal"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEfraktur; ev.italic=false; }, 0);
        instructions["frak"] = i;
        instructions["textfrak"] = i;
        instructions["mathfrak"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEfraktur; ev.bold=true; }, 0);
        instructions["ffrak"] = i;
        instructions["textffrak"] = i;
        instructions["mathffrak"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEblackboard; ev.italic=false; }, 0);
        instructions["bb"] = i;
        instructions["textbb"] = i;
        instructions["mathbb"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEtypewriter; ev.italic=false; }, 0);
        instructions["tt"] = i;
        instructions["texttt"] = i;
        instructions["mathtt"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEsans; ev.italic=false; }, 0);
        instructions["sf"] = i;
        instructions["textsf"] = i;
        instructions["mathsf"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEsans; ev.italic=true; }, 0);
        instructions["sfit"] = i;
        instructions["textsfit"] = i;
        instructions["mathsfit"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEscript; ev.italic=false; }, 0);
        instructions["script"] = i;
        instructions["scr"] = i;
        instructions["textscript"] = i;
        instructions["textscr"] = i;
        instructions["mathscript"] = i;
        instructions["mathscr"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.font=JKQTMathTextEnvironmentFont::MTEscript; ev.bold=true; ev.italic=false; }, 0);
        instructions["fscript"] = i;
        instructions["fscr"] = i;
        instructions["textfscript"] = i;
        instructions["textfscr"] = i;
        instructions["mathfscript"] = i;
        instructions["mathfscr"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.fontSize=ev.fontSize/0.8; }, 0);
        instructions["displaystyle"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.fontSize=ev.fontSize*0.8; }, 0);
        instructions["scriptstyle"]= i;
    }
    {
        InstructionProperties i([](JKQTMathTextEnvironment& ev, const QStringList& parameters) { ev.fontSize=ev.fontSize*0.8*0.8; }, 0);
        instructions["scriptscriptstyle"]= i;
    }
}

JKQTMathTextModifiedTextPropsInstructionNode::InstructionProperties::InstructionProperties():
    NParams(0),
    modifier([](JKQTMathTextEnvironment&, const QStringList&) {})
{

}

JKQTMathTextModifiedTextPropsInstructionNode::InstructionProperties::InstructionProperties(const ModifyEnvironmentFunctor &_modifier, size_t _NParams):
    modifier(_modifier),
    NParams(_NParams)
{

}














JKQTMathTextBoxInstructionNode::JKQTMathTextBoxInstructionNode(JKQTMathText* _parent, const QString& name, JKQTMathTextNode* child, const QStringList& parameters):
    JKQTMathTextInstruction1Node(_parent, name, child, parameters)
{
}

JKQTMathTextBoxInstructionNode::~JKQTMathTextBoxInstructionNode() {
}


QString JKQTMathTextBoxInstructionNode::getTypeName() const
{
    return QLatin1String("JKQTMathTextBoxInstructionNode(")+instructionName+")";
}

void JKQTMathTextBoxInstructionNode::getSizeInternal(QPainter& painter, JKQTMathTextEnvironment currentEv, double& width, double& baselineHeight, double& overallHeight, double& strikeoutPos, const JKQTMathTextNodeSize* /*prevNodeSize*/) {
    JKQTMathTextEnvironment ev=currentEv;

    const auto& inst=instructions.value(getInstructionName());
    inst.modifier(ev, getParameters());
    const QPen p=inst.pen(ev, getParameters(), parentMathText);
    const QBrush b=inst.brush(ev, getParameters(), parentMathText);
    const QFontMetricsF fmNonItalic(JKQTMathTextGetNonItalic(currentEv.getFont(parentMathText)));
    const double lw=p.widthF();
    const double padding=inst.paddingFactor*fmNonItalic.tightBoundingRect("x").width();

    getChild()->getSize(painter, ev, width, baselineHeight, overallHeight, strikeoutPos);
    width=width+2.0*(padding+lw/2.0);
    baselineHeight=baselineHeight+padding+lw/2.0;
    overallHeight=overallHeight+2.0*(padding+lw/2.0);
}

double JKQTMathTextBoxInstructionNode::draw(QPainter& painter, double x, double y, JKQTMathTextEnvironment currentEv, const JKQTMathTextNodeSize* /*prevNodeSize*/) {
    doDrawBoxes(painter, x, y, currentEv);
    JKQTMathTextEnvironment ev=currentEv;

    const auto& inst=instructions.value(getInstructionName());
    inst.modifier(ev, getParameters());
    const QPen p=inst.pen(ev, getParameters(), parentMathText);
    const QBrush b=inst.brush(ev, getParameters(), parentMathText);
    const QFontMetricsF fmNonItalic(JKQTMathTextGetNonItalic(currentEv.getFont(parentMathText)));
    const double lw=p.widthF();
    const double padding=inst.paddingFactor*fmNonItalic.tightBoundingRect("x").width();
    const double rr=inst.roundingFactor*fmNonItalic.tightBoundingRect("x").width();
    double width=0, baselineHeight=0, overallHeight=0, strikeoutPos=0;
    getChild()->getSize(painter, ev, width, baselineHeight, overallHeight, strikeoutPos);

    {
        painter.save(); auto __finalpaint=JKQTPFinally([&painter]() {painter.restore();});
        painter.setBrush(b);
        painter.setPen(p);
        const QRectF rect(x+lw/2.0, y-baselineHeight-padding-lw/2.0, width+2.0*padding, overallHeight+2.0*padding);
        if (rr>0) painter.drawRoundedRect(rect, rr, rr, Qt::AbsoluteSize);
        else painter.drawRect(rect);
        if (inst.doubleLine) {
            painter.setBrush(Qt::NoBrush);
            QPen p2=p;
            p2.setWidthF(p.widthF()*0.6);
            const QRectF recti(x+lw*2.5, y-baselineHeight-lw/2.0-padding+2.0*lw, width+2.0*padding-4.0*lw, overallHeight+2.0*padding-4.0*lw);
            if (rr>0) painter.drawRoundedRect(recti, rr, rr, Qt::AbsoluteSize);
            else painter.drawRect(recti);
        }
    }

    double xnew = getChild()->draw(painter, x+padding+lw/2.0, y, ev);

    return xnew+padding+lw/2.0;
}

bool JKQTMathTextBoxInstructionNode::toHtml(QString &html, JKQTMathTextEnvironment currentEv, JKQTMathTextEnvironment defaultEv) {
    JKQTMathTextEnvironment ev=currentEv;
    fillInstructions();
    const auto& inst=instructions.value(getInstructionName());
    inst.modifier(ev, getParameters());
    const QPen p=inst.pen(ev, getParameters(), parentMathText);
    const QBrush b=inst.brush(ev, getParameters(), parentMathText);
    const QFontMetricsF fmNonItalic(JKQTMathTextGetNonItalic(currentEv.getFont(parentMathText)));
    const double lw=p.widthF();
    const double padding=inst.paddingFactor*fmNonItalic.tightBoundingRect("x").width();
    const double rr=inst.roundingFactor*fmNonItalic.tightBoundingRect("x").width();
    QString s=QString("padding: %1px").arg(padding);
    if (p!=Qt::NoPen) {
        if (s.size()>0 && s.right(2)!="; ") s=s+"; ";
        if (p.widthF()>0 && p.color()!=Qt::transparent) s=s+"border-color: "+p.color().name();
        if (s.size()>0 && s.right(2)!="; ") s=s+"; ";
        if (p.widthF()>0 && p.color()!=Qt::transparent) s=s+QString("border-width: %1px").arg(p.width());
    }
    if (b!=Qt::NoBrush) {
        if (s.size()>0 && s.right(2)!="; ") s=s+"; ";
        if (b.color()!=Qt::transparent) s=s+"background-color: "+b.color().name();
    }

    if (s.size()>0) html=html+ QString("<span style=\"%1\">").arg(s);
    bool ok=getChild()->toHtml(html, ev, defaultEv);
    if (s.size()>0) html=html+"</span>";
    return ok;
}

bool JKQTMathTextBoxInstructionNode::supportsInstructionName(const QString &instructionName)
{
    fillInstructions();
    return instructions.contains(instructionName);
}

size_t JKQTMathTextBoxInstructionNode::countParametersOfInstruction(const QString &instructionName)
{
    fillInstructions();
    if (instructions.contains(instructionName)) return instructions[instructionName].NParams;
    return 0;
}

void JKQTMathTextBoxInstructionNode::modifyInMathEnvironment(const QString &instructionName, bool &insideMath, const QStringList& params)
{
    fillInstructions();
    if (instructions.contains(instructionName)) {
        JKQTMathTextEnvironment ev;
        ev.insideMath=insideMath;
        instructions[instructionName].modifier(ev, params);
        insideMath=ev.insideMath;
    }
}


QHash<QString, JKQTMathTextBoxInstructionNode::InstructionProperties> JKQTMathTextBoxInstructionNode::instructions;

void JKQTMathTextBoxInstructionNode::fillInstructions()
{

    {
        InstructionProperties i(InstructionProperties::NoModification,
                                InstructionProperties::DefaultPen,
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/0);
        instructions["fbox"] = i;
        instructions["framebox"] = i;
        instructions["boxed"] = i;
        instructions["framed"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                InstructionProperties::DefaultPen,
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/0);
        i.doubleLine=true;
        instructions["doublebox"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                InstructionProperties::DefaultPen,
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/0);
        i.roundingFactor=0.7;
        instructions["ovalbox"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){
                                    QPen p=InstructionProperties::DefaultPen(ev, parameters, parent);
                                    p.setWidthF(p.widthF()*1.5);
                                    return p;
                                },
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/0);
        i.roundingFactor=0.8;
        instructions["Ovalbox"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                InstructionProperties::DefaultPen,
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/0);
        i.roundingFactor=0.7;
        i.doubleLine=true;
        instructions["ovaldoublebox"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){
                                    QPen p=InstructionProperties::DefaultPen(ev, parameters, parent);
                                    p.setColor(jkqtp_String2QColor(parameters.value(0, p.color().name())));
                                    return p;
                                },
                                InstructionProperties::NoBrush,
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/1);
        instructions["colorbox"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                InstructionProperties::NoPen,
                                [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){
                                    return QBrush(jkqtp_String2QColor(parameters.value(0, QColor(Qt::transparent).name())), Qt::SolidPattern);
                                },
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/1);
        instructions["shaded"] = i;
    }
    {
        InstructionProperties i(InstructionProperties::NoModification,
                                [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){
                                    QPen p=InstructionProperties::DefaultPen(ev, parameters, parent);
                                    p.setColor(jkqtp_String2QColor(parameters.value(0, p.color().name())));
                                    return p;
                                },
                                [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){
                                    return QBrush(jkqtp_String2QColor(parameters.value(1, QColor(Qt::transparent).name())), Qt::SolidPattern);
                                },
                                InstructionProperties::DefaultPadding,
                                /*Nparams=*/2);
        instructions["fcolorbox"] = i;
    }
}

JKQTMathTextBoxInstructionNode::InstructionProperties::ModifyEnvironmentFunctor JKQTMathTextBoxInstructionNode::InstructionProperties::NoModification=
        [](JKQTMathTextEnvironment& ev, const QStringList& parameters){};
JKQTMathTextBoxInstructionNode::InstructionProperties::GetBoxPenFunctor JKQTMathTextBoxInstructionNode::InstructionProperties::DefaultPen=
        [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){ return QPen(ev.color, QFontMetricsF(ev.getFont(parent)).lineWidth(), Qt::SolidLine); };
JKQTMathTextBoxInstructionNode::InstructionProperties::GetBoxPenFunctor JKQTMathTextBoxInstructionNode::InstructionProperties::NoPen=
        [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){ return Qt::NoPen; };
JKQTMathTextBoxInstructionNode::InstructionProperties::GetBoxBrushFunctor JKQTMathTextBoxInstructionNode::InstructionProperties::NoBrush=
        [](JKQTMathTextEnvironment& ev, const QStringList& parameters, JKQTMathText* parent){ return Qt::NoBrush; };
double JKQTMathTextBoxInstructionNode::InstructionProperties::DefaultPadding=0.5;

JKQTMathTextBoxInstructionNode::InstructionProperties::InstructionProperties():
    InstructionProperties(NoModification, DefaultPen, NoBrush, DefaultPadding, 0)
{

}

JKQTMathTextBoxInstructionNode::InstructionProperties::InstructionProperties(const GetBoxPenFunctor &_pen, const GetBoxBrushFunctor &_brush, double _paddingFactor, size_t _NParams):
    InstructionProperties(NoModification, _pen, _brush, _paddingFactor, _NParams)

{

}

JKQTMathTextBoxInstructionNode::InstructionProperties::InstructionProperties(const ModifyEnvironmentFunctor &_modifier, const GetBoxPenFunctor &_pen, const GetBoxBrushFunctor &_brush, double _paddingFactor, size_t _NParams):
    modifier(_modifier),
    pen(_pen),
    brush(_brush),
    NParams(_NParams),
    paddingFactor(_paddingFactor),
    doubleLine(false),
    roundingFactor(0.0)
{

}
