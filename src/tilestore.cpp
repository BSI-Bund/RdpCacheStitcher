/*
  Copyright 2020 Bundesamt fuer Sicherheit in der Informationstechnik (BSI)

  This file is part of RdpCacheStitcher.

  RdpCacheStitcher is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  RdpCacheStitcher is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with RdpCacheStitcher.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "tilestore.h"
#include "mainwindow.h"

#include <QDir>
#include <QProgressDialog>
#include <QSet>
#include <QHash>
#include <iostream>

const double TileStore::QUALITY_THRESHOLD = 0.45;

TileStore::TileStore() :
    tileSize(0)
{
}

TileStore::TileStore(QImage tileImage, int tileSize) :
    tileSize(tileSize)
{
    const int numRows = tileImage.height()/tileSize;
    const int numCols = tileImage.width()/tileSize;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            store.append(Tile(tileImage.copy(col*tileSize, row*tileSize, tileSize, tileSize), false, false));
            useCounts.append(0);
        }
    }
}

QString TileStore::loadTiles(QString dir)
{
    store.clear();
    useCounts.clear();
    tileSize = 0;
    QDir tileDir(dir);
    QStringList images = tileDir.entryList(QStringList() << "*.bmp", QDir::Files, QDir::Name);
    if (images.isEmpty()) {
        return "No .bmp image files to read!";
    }
    int numSuccess = 0;
    int numFailures = 0;
    int numResized = 0;
    int numDuplicates = 0;
    QProgressDialog pd("Training AI and building blockchain...", "Cancel", 0, images.size());
    pd.setWindowModality(Qt::WindowModal);
    pd.setMinimumDuration(0);
    QSet<uint> imageHashes;
    foreach(QString name, images) {
        pd.setValue(pd.value() + 1);
        QString path = dir + QDir::separator() + name;
        Tile t(path);
        if (t.isNull() || (tileSize != 0 && tileSize != t.size)) {
            numFailures++;
        } else {
            numSuccess++;
            if (t.isResized) {
                numResized++;
            }
            tileSize = t.size;
            useCounts.append(0);
            // Check for duplicate
            uint hash = qHashBits(t.getImage().constBits(), t.getImage().byteCount());
            if (imageHashes.contains(hash)) {
                t.isDuplicate = true;
                numDuplicates++;
            } else {
                imageHashes.insert(hash);
            }
            store.append(t);
        }
    }
    MainWindow::displayMessage(
                QString("Loaded ") + QString::number(numSuccess) + " tiles ("
                + QString::number(numDuplicates) + " duplicates and "
                + QString::number(numResized) + " non-square).\n" +
                QString::number(numFailures) + " tiles failed to load.");
    return "";
}

int TileStore::size() const
{
    return store.size();
}

Tile TileStore::getTile(int index)
{
    return store.at(index);
}

QString TileStore::saveData(QDataStream &out)
{
    out << (qint32)tileSize;
    out << (qint32)store.size();
    QProgressDialog pd("Saving case data...", "Cancel", 0, store.size());
    pd.setWindowModality(Qt::WindowModal);
    pd.setMinimumDuration(0);
    for (int i = 0; i < store.size(); ++i) {
        pd.setValue(i);
        out << store.at(i).getImage();
        out << (bool)(store.at(i).isResized);
        out << (bool)(store.at(i).isDuplicate);
        if (pd.wasCanceled()) {
            return "Cancelled";
        }
    }
    out << useCounts;
    return "";
}

QString TileStore::loadData(QDataStream &in)
{
    qint32 in_tileSize;
    in >> in_tileSize;
    tileSize = (int)in_tileSize;
    qint32 in_size;
    in >> in_size;
    store.clear();
    QProgressDialog pd("Loading case data...", "Cancel", 0, in_size);
    pd.setWindowModality(Qt::WindowModal);
    pd.setMinimumDuration(0);
    for (int i = 0; i < in_size; ++i) {
        pd.setValue(i);
        QImage image;
        in >> image;
        bool isResized;
        in >> isResized;
        bool isDuplicate;
        in >> isDuplicate;
        store.append(Tile(image, isResized, isDuplicate));
        if (pd.wasCanceled()) {
            return "Camcelled";
        }
    }
    useCounts.clear();
    in >> useCounts;
    return "";
}

int TileStore::getUseCount(int index)
{
    return useCounts.at(index);
}

void TileStore::incUseCount(int index)
{
    useCounts.replace(index, useCounts.at(index) + 1);
}

void TileStore::decUseCount(int index)
{
    int count = useCounts.at(index);
    useCounts.replace(index, count - 1);
    Q_ASSERT(count - 1 >= 0);
}

bool TileStore::isHidden(int index)
{
    Tile t = getTile(index);
    return (
                (hideDuplicates && t.isDuplicate)
                || (hideNonSquare && t.isResized)
                || (hideUsed && useCounts.at(index) > 0)
                );
}

void TileStore::hideUsedChanged(int state)
{
    hideUsed = (state == Qt::Checked);
    emit availableTilesChanged();
}

void TileStore::hideDuplicatesChanged(int state)
{
    hideDuplicates = (state == Qt::Checked);
    emit availableTilesChanged();
}

void TileStore::hideNonSquareChanged(int state)
{
    hideNonSquare = (state == Qt::Checked);
    emit availableTilesChanged();
}

bool TileStore::getHideUsed() const
{
    return hideUsed;
}
