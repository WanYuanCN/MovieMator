/*
 * Copyright (c) 2012-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "timespinbox.h"
#include "mltcontroller.h"
#include <QRegExpValidator>
#include <QKeyEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QDebug>

TimeSpinBox::TimeSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    m_lineEdit = new TimeSpinBoxLineEdit;
    setLineEdit(m_lineEdit);
    setRange(0, INT_MAX);

 //   setFixedWidth(this->fontMetrics().width("HHHMMSSMMM"));
//    setFixedSize(130, 35);
//    setBackgroundRole(QPalette::ColorRole::Highlight);

    setAlignment(Qt::AlignLeft);

    m_validator = new QRegExpValidator(QRegExp("^\\s*(\\d*:){0,2}(\\d*[.;:])?\\d*\\s*$"), this);

    setValue(0);
#ifdef Q_OS_MAC
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

    font.setPointSize(QGuiApplication::font().pointSize());


    m_lineEdit->setFont(font);
#endif


//    setStyleSheet("background-color: rgb(127,127,127);color:rgb(255,255,255);border-image:url()");


}

QValidator::State TimeSpinBox::validate(QString &input, int &pos) const
{
    return m_validator->validate(input, pos);
}

int TimeSpinBox::valueFromText(const QString &text) const
{
    if (MLT.producer()) {
        return MLT.producer()->time_to_frames(text.toLatin1().constData());
    }
    return 0;
}

QString TimeSpinBox::textFromValue(int val) const
{
    if (MLT.producer()) {
        return MLT.producer()->frames_to_time(val);
    }
    return QString();
}

void TimeSpinBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        clearFocus();
    else
        QSpinBox::keyPressEvent(event);
}

void TimeSpinBox::setFontSize(int size)
{
#ifdef Q_OS_MAC
    QFont sysFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

    QFont font = QFont("DigifaceWide");

    font.setPointSize(size);
    font.setLetterSpacing(QFont::PercentageSpacing, 140);

    m_lineEdit->setFont(font);
#endif
    Q_UNUSED(size)
}


TimeSpinBoxLineEdit::TimeSpinBoxLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_selectOnMousePress(false)
{

}

void TimeSpinBoxLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    selectAll();
    m_selectOnMousePress = true;
}

void TimeSpinBoxLineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    if (m_selectOnMousePress) {
        selectAll();
        m_selectOnMousePress = false;
    }
}

