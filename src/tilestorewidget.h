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

#ifndef TILESTOREWIDGET_H
#define TILESTOREWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QVector>

#include "tilestore.h"

class TileStoreWidget : public QTableWidget
{
    Q_OBJECT

public:
    static const int DEFAULT_WIDTH = 20;
    static const int TILE_MARGIN;

    TileStoreWidget(TileStore *tileStore);

    //
    // Return the index of the currently selected tile, or -1 if none
    //
    int selectedIndex();
    void selectTile(int index);

public slots:
    void tileStoreChanged();
    void tileStoreWidthChanged(int width);

private:
    static const double USED_TILE_OPACITY;

    TileStore *tileStore;
    QVector<int> tileMapping;
    int width;
    int height;

    void updateStore();
};

#endif // TILESTOREWIDGET_H
