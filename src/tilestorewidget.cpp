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

#include "tilestorewidget.h"

#include <QTableWidget>
#include <QHeaderView>
#include <iostream>
#include <QPainter>

#include "tilestore.h"

const double TileStoreWidget::USED_TILE_OPACITY = 0.25;
const int TileStoreWidget::TILE_MARGIN = 6;

TileStoreWidget::TileStoreWidget(TileStore *tileStore)
    : tileStore(tileStore),
      width(DEFAULT_WIDTH)
{
    updateStore();
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

int TileStoreWidget::selectedIndex()
{
    int index = -1;
    auto selected = selectedIndexes();
    if (selected.size() == 1) {
        int row = selected.at(0).row();
        int col = selected.at(0).column();
        index = row*width + col;
    }
    if (index == -1 || index >= tileMapping.size()) {
        return -1;
    }
    return tileMapping.at(index);
}

void TileStoreWidget::selectTile(int index)
{
    int tableIndex = -1;
    for (int i = 0; i < tileMapping.size(); ++i) {
        if (tileMapping.at(i) == index) {
            tableIndex = i;
            break;
        }
    }
    if (tableIndex != -1) {
        int row = tableIndex/width;
        int col = tableIndex%width;
        QModelIndex index = model()->index(row, col);
        selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void TileStoreWidget::tileStoreChanged()
{
    updateStore();
}

void TileStoreWidget::tileStoreWidthChanged(int width)
{
    this->width = width;
    updateStore();
}

void TileStoreWidget::updateStore()
{
    clear();
    height = (tileStore->size() + width - 1)/width;
    setRowCount(height);
    setColumnCount(width);
    tileMapping.clear();
    int curPos = 0;
    for (int i = 0; i < tileStore->size(); ++i) {
        if (!tileStore->isHidden(i)) {
            QTableWidgetItem *item = new QTableWidgetItem();
            if (tileStore->getUseCount(i) == 0) {
                item->setData(Qt::DecorationRole, QPixmap::fromImage(tileStore->getTile(i).getImage()));
            } else {
                QImage image = tileStore->getTile(i).getImage();
                QImage usedImage(image.size(), QImage::Format_ARGB32);
                usedImage.fill(Qt::transparent);
                QPainter painter(&usedImage);
                painter.setOpacity(USED_TILE_OPACITY);
                painter.drawImage(QRect(0, 0, image.width(), image.height()), image);
                item->setData(Qt::DecorationRole, QPixmap::fromImage(usedImage));
            }
            setItem(curPos/width, curPos%width, item);
            curPos++;
            tileMapping.append(i);
        }
    }
    horizontalHeader()->setDefaultSectionSize(tileStore->tileSize + TILE_MARGIN);
    verticalHeader()->setDefaultSectionSize(tileStore->tileSize + TILE_MARGIN);
    update();
}
