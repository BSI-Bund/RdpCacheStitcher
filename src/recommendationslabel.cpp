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

#include "recommendationslabel.h"

#include <QPainter>
#include <QMap>

const int RecommendationsLabel::CELL_MARGIN = 8;
const int RecommendationsLabel::CELLS_MAX = 64;

RecommendationsLabel::RecommendationsLabel(
        TileStore *tileStore,
        QVector<QPair<double, int>> *recommendations
        )
    : tileStore(tileStore),
      recommendations(recommendations)
{
    tileStoreChanged();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMouseTracking(true);
}

void RecommendationsLabel::mouseMoveEvent(QMouseEvent *event)
{
    emit recommendationHover(calcRecommendedHoverIndex(event->pos()));
}

void RecommendationsLabel::leaveEvent(QEvent *)
{
    emit recommendationHover(-1);
}

void RecommendationsLabel::mousePressEvent(QMouseEvent *event)
{
    int index = calcRecommendedHoverIndex(event->pos());
    if (index != -1) {
        emit placeRecommendation(index);
    }
}

void RecommendationsLabel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setBackgroundMode(Qt::OpaqueMode);
    for (int i = 0; i < std::min(recommendations->size(), CELLS_MAX); ++i) {
        const int index = recommendations->at(i).second;
        painter.drawImage(CELL_MARGIN, CELL_MARGIN + i*cellSize, tileStore->getTile(index).getImage());
    }
}

int RecommendationsLabel::getCellSize() const
{
    return cellSize;
}

void RecommendationsLabel::recommendationsChanged()
{
    update();
}

void RecommendationsLabel::tileStoreChanged()
{
    cellSize = tileStore->tileSize + 2*CELL_MARGIN;
    resize(cellSize, CELLS_MAX*cellSize);
    setPixmap(QPixmap::fromImage(QImage(cellSize, CELLS_MAX*cellSize, QImage::Format_ARGB32)));
    setFixedWidth(cellSize);
    update();
}

int RecommendationsLabel::calcRecommendedHoverIndex(QPoint pos)
{
    int result = -1;
    const int x = pos.x();
    const int y = pos.y();
    if (x >= CELL_MARGIN && x < CELL_MARGIN + tileStore->tileSize
            && y >= CELL_MARGIN && y < CELL_MARGIN + CELLS_MAX*cellSize) {
        const int cell = (y - CELL_MARGIN)/cellSize;
        const int inCell = (y - CELL_MARGIN)%cellSize;
        if (inCell >= CELL_MARGIN && inCell < CELL_MARGIN + tileStore->tileSize
                && cell < recommendations->size()) {
            result = recommendations->at(cell).second;
        }
    }
    return result;
}

