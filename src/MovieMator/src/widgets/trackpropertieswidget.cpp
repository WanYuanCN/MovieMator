/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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

#include "trackpropertieswidget.h"
#include "ui_trackpropertieswidget.h"
#include "shotcut_mlt_properties.h"
#include "commands/timelinecommands.h"
#include "mainwindow.h"
#include "util.h"
#include <Mlt.h>
#include <QScopedPointer>

static const char* BLEND_PROPERTY_CAIROBLEND = "1";

TrackPropertiesWidget::TrackPropertiesWidget(Mlt::Producer& track, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrackPropertiesWidget),
    m_track(track)
{
    ui->setupUi(this);
//    Util::setColorsToHighlight(ui->nameLabel);
    ui->nameLabel->setText(tr("Track: %1").arg(track.get(kTrackNameProperty)));
    ui->blendModeLabel->hide();
    ui->blendModeCombo->hide();

    ui->nameLabel->setStyleSheet("background-color:rgb(46,46,46);");
    ui->blendModeCombo->setStyleSheet("QComboBox {\
                                      background-color:rgb(82,82,82); \
                                      color:rgb(225,225,225); \
                                      border: 1px solid black;\
                                      border-radius: 4px;\
                                      padding: 1px 2px 1px 2px;\
                                      min-width: 9em;\
                                      }"
                                      "QComboBox::drop-down { \
                                      subcontrol-origin: padding; \
                                      subcontrol-position: top right;\
                                      width: 20px;\
                                      border-left-width: 1px;\
                                      border-left-color: rgb(82,82,82);\
                                      border-left-style: solid;\
                                      border-top-right-radius: 4px; \
                                      border-bottom-right-radius: 4px;\
                                      }"
                                      "QComboBox::down-arrow {\
                                      image: url(:/icons/light/8x8/down.png);\
                                      }"
                                      "QComboBox::donw-arrow:on {top:3px;left:3px;}"
                                      "QComboBox QAbstractItemView {border: none;}");

    if (track.get(kVideoTrackProperty))
    {
        QScopedPointer<Mlt::Transition> transition(getTransition("frei0r.cairoblend"));
        if (transition && transition->is_valid()) {
            ui->blendModeCombo->blockSignals(true);
            ui->blendModeCombo->addItem(tr("Over"), "normal");
            ui->blendModeCombo->addItem(tr("Add"), "add");
            ui->blendModeCombo->addItem(tr("Saturate"), "saturate");
            ui->blendModeCombo->addItem(tr("Multiply"), "multiply");
            ui->blendModeCombo->addItem(tr("Screen"), "screen");
            ui->blendModeCombo->addItem(tr("Overlay"), "overlay");
            ui->blendModeCombo->addItem(tr("Darken"), "darken");
            ui->blendModeCombo->addItem(tr("Dodge"), "colordodge");
            ui->blendModeCombo->addItem(tr("Burn"), "colorburn");
            ui->blendModeCombo->addItem(tr("Hard Light"), "hardlight");
            ui->blendModeCombo->addItem(tr("Soft Light"), "softlight");
            ui->blendModeCombo->addItem(tr("Difference"), "difference");
            ui->blendModeCombo->addItem(tr("Exclusion"), "exclusion");
            ui->blendModeCombo->addItem(tr("HSL Hue"), "hslhue");
            ui->blendModeCombo->addItem(tr("HSL Saturation"), "hslsaturatation");
            ui->blendModeCombo->addItem(tr("HSL Color"), "hslcolor");
            ui->blendModeCombo->addItem(tr("HSL Luminosity"), "hslluminocity");
            ui->blendModeCombo->blockSignals(false);
            ui->blendModeLabel->show();
            ui->blendModeCombo->show();

            QString blendMode = transition->get(BLEND_PROPERTY_CAIROBLEND);
            onModeChanged(blendMode);
        }
    }
}

TrackPropertiesWidget::~TrackPropertiesWidget()
{
    delete ui;
}

Mlt::Transition* TrackPropertiesWidget::getTransition(const QString& name)
{
    // track.consumer() is the multitrack
    QScopedPointer<Mlt::Service> service(m_track.consumer());
    if (service && service->is_valid()) {
        Mlt::Multitrack multi(*service);
        int trackIndex;

        // Get the track index by iterating until multitrack.track() == track.get_producer().
        for (trackIndex = 0; trackIndex < multi.count(); ++trackIndex) {
            QScopedPointer<Mlt::Producer> producer(multi.track(trackIndex));
            if (producer->get_producer() == m_track.get_producer())
                break;
        }

        // Iterate the consumers until found transition by mlt_service and track_b index.
        while (service && service->is_valid() && tractor_type != service->type()) {
            if (service->type() == transition_type) {
                Mlt::Transition t(reinterpret_cast<mlt_transition>(service->get_service()));
                if (name == t.get("mlt_service") && t.get_b_track() == trackIndex)
                    return new Mlt::Transition(t);
            }
            service.reset(service->consumer());
        };
    }
    return nullptr;
}

void TrackPropertiesWidget::on_blendModeCombo_currentIndexChanged(int index)
{
    if (index >= 0) {
        QScopedPointer<Mlt::Transition> transition(getTransition("frei0r.cairoblend"));
        if (transition && transition->is_valid()) {
            Timeline::ChangeBlendModeCommand* command = new Timeline::ChangeBlendModeCommand(*(MAIN.timelineDock()->model()),
                *transition, BLEND_PROPERTY_CAIROBLEND, ui->blendModeCombo->itemData(index).toString());
            connect(command, SIGNAL(modeChanged(QString&)), SLOT(onModeChanged(QString&)));
            MAIN.pushCommand(command);
        }
    }
}

void TrackPropertiesWidget::onModeChanged(QString& mode)
{
    for (int i = 0; i < ui->blendModeCombo->count(); ++i) {
        if (ui->blendModeCombo->itemData(i).toString() == mode) {
            ui->blendModeCombo->blockSignals(true);
            ui->blendModeCombo->setCurrentIndex(i);
            ui->blendModeCombo->blockSignals(false);
            break;
        }
    }
}
