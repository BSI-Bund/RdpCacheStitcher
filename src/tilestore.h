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

#ifndef TILESTORE_H
#define TILESTORE_H

#include <QList>
#include <QVector>

#include "tile.h"

class TileStore : public QObject
{
    Q_OBJECT

public:
    static const double QUALITY_THRESHOLD;
    //
    // Size (width and height) of the tiles in the store.
    // All tiles in the store must have the same size. Tiles
    // with a different size than the first one seen are ignored.
    //
    int tileSize;
    QString name;

    TileStore();
    TileStore(QImage tileImage, int tileSize);

    QString loadTiles(QString dir);
    int size() const;
    Tile getTile(int index);
    QString saveData(QDataStream &out);
    QString loadData(QDataStream &in);
    int getUseCount(int index);
    void incUseCount(int index);
    void decUseCount(int index);
    bool isHidden(int index);
    bool isGoodMatch(double score);
    bool getHideUsed() const;

public slots:
    void hideUsedChanged(int state);
    void hideDuplicatesChanged(int state);
    void hideNonSquareChanged(int state);

signals:
    void availableTilesChanged();

private:
    QList<Tile> store;
    QList<int> useCounts;
    bool hideUsed = false;
    bool hideDuplicates = true;
    bool hideNonSquare = false;
};

#endif // TILESTORE_H
