/*
 * Copyright (c) 2014-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "shotcut_mlt_properties.h"
#include "filtercontroller.h"
#include <QQmlEngine>
#include <QDir>
#include <Logger.h>
#include <QQmlComponent>
#include <QTimerEvent>
#include "mltcontroller.h"
#include "settings.h"
#include "qmlmetadata.h"
#include <qmlutilities.h>
#include "qmltypes/qmlfilter.h"

FilterController::FilterController(QObject* parent) : QObject(parent),
 m_metadataModel(this),
 m_attachedModel(this),
 m_currentFilterIndex(-1)
{
    startTimer(0);
    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel, SIGNAL(modelAboutToBeReset()), this, SLOT(handleAttachedModelAboutToReset()));
    connect(&m_attachedModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)), this, SLOT(handleAttachedRowsRemoved(const QModelIndex&,int,int)));
    connect(&m_attachedModel, SIGNAL(rowsInserted(const QModelIndex&,int,int)), this, SLOT(handleAttachedRowsInserted(const QModelIndex&,int,int)));
    connect(&m_attachedModel, SIGNAL(duplicateAddFailed(int)), this, SLOT(handleAttachDuplicateFailed(int)));
}

void FilterController::loadFilterMetadata() {
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    foreach (QString dirName, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml"));
        foreach (QString fileName, subdir.entryList()) {
            LOG_DEBUG() << "reading filter metadata" << dirName << fileName;
            QQmlComponent component(QmlUtilities::sharedEngine(), subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata*>(component.create());
            if (meta) {
                // Check if mlt_service is available.
                if (MLT.repository()->filters()->get_data(meta->mlt_service().toLatin1().constData())) {
                    LOG_DEBUG() << "added filter" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta);
                }
            } else if (!meta) {
                LOG_WARNING() << component.errorString();
            }
        }
    };
}

QmlMetadata *FilterController::metadataForService(Mlt::Service *service)
{
    QmlMetadata* meta = 0;
    int rowCount = m_metadataModel.rowCount();
    QString uniqueId = service->get(kShotcutFilterProperty);

    // Fallback to mlt_service for legacy filters
    if (uniqueId.isEmpty()) {
        uniqueId = service->get("mlt_service");
    }

    for (int i = 0; i < rowCount; i++) {
        QmlMetadata* tmpMeta = m_metadataModel.get(i);
        if (tmpMeta->uniqueId() == uniqueId) {
            meta = tmpMeta;
            break;
        }
    }

    return meta;
}

void FilterController::timerEvent(QTimerEvent* event)
{
    loadFilterMetadata();
    killTimer(event->timerId());
}

MetadataModel* FilterController::metadataModel()
{
    return &m_metadataModel;
}

AttachedFiltersModel* FilterController::attachedModel()
{
    return &m_attachedModel;
}

void FilterController::setProducer(Mlt::Producer *producer)
{
    if (producer && producer->get_int("meta.fx_cut"))
        return;
    m_attachedModel.setProducer(producer);
    if (producer && producer->is_valid()) {
        mlt_service_type service_type = producer->type();
        m_metadataModel.setIsClipProducer(service_type != tractor_type
            && service_type != playlist_type);
    }

}

void FilterController::setCurrentFilter(int attachedIndex)
{
    Q_ASSERT(attachedIndex <= m_attachedModel.rowCount());

    if (attachedIndex == m_currentFilterIndex) {
        return;
    }
    m_currentFilterIndex = attachedIndex;

    QmlMetadata* meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter* filter = 0;
    if (meta) {
        Mlt::Filter* mltFilter = m_attachedModel.getFilter(m_currentFilterIndex);

        filter = new QmlFilter(mltFilter, meta);

    }

    emit currentFilterAboutToChange();
    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
}

void FilterController::refreshCurrentFilter(Mlt::Filter *filter)
{
    if(m_currentFilterIndex == -1) return;

    QmlFilter *qmlFilter = m_currentFilter.data();
    if(!qmlFilter) return;

    Mlt::Filter* mltFilter = qmlFilter->getMltFilter();
    if(mltFilter->get_filter() != filter->get_filter())
    {
        return;
    }

    QmlMetadata* meta = m_attachedModel.getMetadata(m_currentFilterIndex);
 /*   QmlFilter* qfilter = 0;
    if (meta)
    {
        Mlt::Filter* mltFilter = m_attachedModel.getFilter(m_currentFilterIndex);
        qfilter = new QmlFilter(mltFilter, meta);
    }
*/
 //   emit currentFilterAboutToChange();
    emit currentFilterChanged(m_currentFilter.data(), meta, m_currentFilterIndex);

//    m_currentFilter.reset(qfilter);
}

void FilterController::refreshKeyFrame(Mlt::Filter *filter, const QVector<key_frame_item> &listKeyFrame)
{
    if(m_currentFilterIndex == -1) return;

    QmlFilter *qmlFilter = m_currentFilter.data();
    if(!qmlFilter) return;

    Mlt::Filter* mltFilter = qmlFilter->getMltFilter();
    if(mltFilter->get_filter() != filter->get_filter())
    {
        return;
    }

    qmlFilter->refreshKeyFrame(listKeyFrame);
}

void FilterController::handleAttachedModelChange()
{
    MLT.refreshConsumer();
}

void FilterController::handleAttachedModelAboutToReset()
{
    setCurrentFilter(-1);
}

void FilterController::handleAttachedRowsRemoved(const QModelIndex&, int first, int)
{
    int newFilterIndex = first;
    if (newFilterIndex >= m_attachedModel.rowCount()) {
        newFilterIndex = m_attachedModel.rowCount() - 1;
    }
    m_currentFilterIndex = -2; // Force update
    setCurrentFilter(newFilterIndex);
}

void FilterController::handleAttachedRowsInserted(const QModelIndex&, int first, int)
{
    m_currentFilterIndex = first;
    Mlt::Filter* mltFilter = m_attachedModel.getFilter(m_currentFilterIndex);
    QmlMetadata* meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter* filter = new QmlFilter(mltFilter, meta);
    filter->setIsNew(true);
    emit currentFilterAboutToChange();
    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
}

void FilterController::handleAttachDuplicateFailed(int index)
{
    Q_ASSERT(index < m_attachedModel.rowCount());

    const QmlMetadata* meta = m_attachedModel.getMetadata(index);
    Q_ASSERT(meta);
    emit statusChanged(tr("Only one %1 filter is allowed.").arg(meta->name()));
    setCurrentFilter(index);
}

void FilterController::addMetadata(QmlMetadata* meta)
{
    Q_ASSERT(meta);
    m_metadataModel.add(meta);
}


void FilterController::addPositionAndSizeFilter()
{

    QmlMetadata *meta = metadataForUniqueId("affineSizePosition");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);

}

void FilterController::addRotateFilter()
{

    QmlMetadata *meta = metadataForUniqueId("affineRotate");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);
}

void FilterController::addCropFilter()
{

    QmlMetadata *meta = metadataForUniqueId("crop");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);
}

void FilterController::addVolumeFilter()
{

    QmlMetadata *meta = metadataForUniqueId("volume");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);
}


void FilterController::addFadeInAudioFilter()
{

    QmlMetadata *meta = metadataForUniqueId("fadeInVolume");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);

}

void FilterController::addFadeOutAudioFilter()
{
    QmlMetadata *meta = metadataForUniqueId("fadeOutVolume");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);

}


void FilterController::addFadeInVideoFilter()
{
    QmlMetadata *meta = metadataForUniqueId("fadeInBrightness");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);


}

void FilterController::addFadeOutVideoFilter()
{
    QmlMetadata *meta = metadataForUniqueId("fadeOutBrightness");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);


}

void FilterController::addTextFilter()
{
    QmlMetadata *meta = metadataForUniqueId("dynamicText");
    Q_ASSERT(meta);
    m_attachedModel.add(meta);
}


QmlMetadata *FilterController::metadataForUniqueId(const char *uniqueId)
{
    QmlMetadata* meta = 0;
    int rowCount = m_metadataModel.rowCount();
    QString qstrUniqueId(uniqueId);

    for (int i = 0; i < rowCount; i++) {
        QmlMetadata* tmpMeta = m_metadataModel.get(i);
        if (tmpMeta->uniqueId() == qstrUniqueId) {
            meta = tmpMeta;
            break;
        }
    }

    return meta;
}

void FilterController::addFilter(const QString &filterID)
{
    QmlMetadata *meta = metadataForUniqueId(filterID.toUtf8().constData());
    if (meta)
        m_attachedModel.add(meta);
}

void FilterController::removeFilter(int row)
{
    m_attachedModel.remove(row);
}

