/*
 * Copyright (c) 2016-2019 EffectMatrix Inc.
 * Author: WanYuanCN <ebthon@hotmail.com>
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

#ifndef TEXTLISTMODEL_H
#define TEXTLISTMODEL_H

#include <QImage>
#include <QAbstractItemModel>
#include "maininterface.h"

class TextItemInfo : public QObject
{
    Q_OBJECT

public:

    explicit TextItemInfo(QObject *parent = nullptr){Q_UNUSED(parent);}

    QString textFilePath() const {return m_textFilePath;}
    void setTextFilePath(const QString textFilePath) {m_textFilePath = textFilePath;}
    FILE_HANDLE textFileHandel() const {return  m_textFileHandel;}
    void setTextFileHandel(const FILE_HANDLE textFileHandel) {m_textFileHandel = textFileHandel;}

private:
    QString m_textFilePath;
    FILE_HANDLE m_textFileHandel;
};

class TextListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // 缩略图宽度
    static const int THUMBNAIL_WIDTH = 80;      // 100;
    // 缩略图高度
    static const int THUMBNAIL_HEIGHT = 60;     // 70;

    // 构造函数
    explicit TextListModel(MainInterface *main=nullptr, QObject *parent=nullptr);
    // 析构函数
    ~TextListModel();

    // model的行数
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    // model的列数
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    // model的数据模型
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    // model的 mimeData
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    // model的序号
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    // model的父项
    QModelIndex parent(const QModelIndex &child) const;

    // 给 model添加数据 fileHandle
    void append(TextItemInfo *textItemInfo);

    // row行的文件（第 row个数据的内容）
    TextItemInfo *fileAt(int row) const;
    // row行的缩略图（第 row个数据的缩略图）
    QImage thumbnail(int row) const;

private:
    // model的 list
    QList<TextItemInfo *> *m_effectList;
    // 主界面
    MainInterface *m_mainWindow;
};

#endif // TEXTLISTMODEL_H
