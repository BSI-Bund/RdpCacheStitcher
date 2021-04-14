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

#ifndef BESTMATCHLABEL_H
#define BESTMATCHLABEL_H

#include <QObject>
#include <QLabel>

#include "tilestore.h"

class RecommendationsLabel : public QLabel
{
    Q_OBJECT

public:
    static const int CELL_MARGIN;
    //
    // Max number of best matches/hints to display
    //
    static const int CELLS_MAX;

    RecommendationsLabel(
            TileStore *tileStore,
            QVector<QPair<double, int>> *recommendations
            );

    int getCellSize() const;

    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

public slots:
    void recommendationsChanged();
    void tileStoreChanged();

signals:
    void recommendationHover(int tileIndex);
    void placeRecommendation(int tileIndex);

private:
    QColor COL_BACK = QColor("#ffffff");
    TileStore *tileStore;
    QVector<QPair<double, int>> *recommendations;
    int cellSize;

    int calcRecommendedHoverIndex(QPoint pos);
};

#endif // BESTMATCHLABEL_H
