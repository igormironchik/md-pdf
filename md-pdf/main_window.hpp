
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

#ifndef MD_PDF_MAIN_WINDOW_HPP_INCLUDED
#define MD_PDF_MAIN_WINDOW_HPP_INCLUDED


// md-pdf include.
#include "ui_main_window.h"

// Qt include.
#include <QWidget>
#include <QScopedPointer>
#include <QThread>
#include <QMainWindow>


//
// MainWidget
//

//! Main window.
class MainWidget final
	:	public QWidget
{
	Q_OBJECT

public:
	explicit MainWidget( QWidget * parent );
	~MainWidget() override;

private slots:
	void changeLinkColor();
	void changeBorderColor();
	void changeCodeBackground();
	void changeKeywordColor();
	void changeCommentColor();
	void changeCodeColor();
	void selectMarkdown();
	void process();
	void codeFontSizeChanged( int i );
	void textFontSizeChanged( int i );
	void mmButtonToggled( bool on );
	void textFontChanged( const QFont & f );
	void codeFontChanged( const QFont & f );

private:
	QScopedPointer< Ui::MainWindow > m_ui;
	QThread * m_thread;

	Q_DISABLE_COPY( MainWidget )
}; // class MainWindow


//
// MainWindow
//

class MainWindow
	:	public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow() override = default;

private slots:
	//! About.
	void about();
	//! About Qt.
	void aboutQt();
	//! Licenses.
	void licenses();
	//! Quit.
	void quit();
}; // class MainWindow

#endif // MD_PDF_MAIN_WINDOW_HPP_INCLUDED
