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

#ifndef SCREENLABEL_H
#define SCREENLABEL_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QVector>

#include "tilestore.h"
#include "tilestorewidget.h"
#include "notesdialog.h"

class ScreenLabel : public QLabel
{
    Q_OBJECT

public:
    //
    // Screen size in tiles
    //
    static const int SCREEN_DEFAULT_WIDTH;
    static const int SCREEN_DEFAULT_HEIGHT;
    //
    // Max number of screens, corresponds to max setting in respective spin box
    //
    static const int SCREENSTORE_SIZE;
    //
    // Cell states
    //
    static const int CELL_EMPTY;
    static const int CELL_LOCKED;   // Unused for now
    //
    // Match/hint cell state
    //
    static const double MATCH_EMPTY;
    //
    // Greater distances are reduced to this value for heuristic
    //
    static const int HEURISTIC_MAX_STORE_DISTANCE;
    //
    // Divide final score by this if tile is resized and bottom edge is considered
    //
    static const double HEURISTIC_RESIZED_FACTOR;
    static double g_heurStoreDistanceWeight;
    //
    // Below this value, the score gets penalized for the edge having not enough different colors
    //
    static double g_heurColorsThreshold;
    static double g_heurColorsWeight;

    ScreenLabel(TileStore *tileStore, TileStoreWidget *tileStoreWidget);

    QString saveCase(QString filename);
    QString loadCase(QString filename);
    //
    // Save and load case data, without magic and version (called by save/laodCase)
    //
    QString saveData(QDataStream &out);
    QString loadData(QDataStream &in, bool loadNotes);
    QString exportScreens(QString prefix);
    //
    // Transfer current screen and note contents to store
    //
    void storeCurrentScreen();
    void placeTile(int x, int y, int tileIndex);
    //
    // Placing/removing a tile and changing a note will set modified flag
    //
    bool isModified() const;
    void clearModified();
    QVector<QPair<double, int> > *getRecommendations();
    NotesDialog *getNotesDialog();

    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;


public slots:
    void notesWindowStateChanges(int state);
    void notesModified(bool);
    void tileStoreChanged();
    void initScreens();
    void updateMatchValues();
    void recommendationHover(int tileIndex);
    void placeRecommendation(int tileIndex);
    void screenNumberChanged(int newNumber);
    void autoplace();
    void updateRecommendations();

signals:
    void recommendationsChanged();
    void availableTilesChanged();

private:
    static const int MARGIN = 64;

    QColor COL_BACK = QColor("#dddddd");
    QColor COL_GRID = QColor("#aaaaaa");
    QColor MAX_MATCH = QColor("#88ff44");
    QColor CELL_SELECTED = QColor("#ffee00");

    static const QString CASEFILE_MAGIC;
    static const int CASEFILE_VERSION;

    const Tile::Filter curFilter = Tile::Filter::Gauss15;
    QPoint selectedPos{-1, -1};
    bool modified;
    TileStore *tileStore;
    TileStoreWidget * tileStoreWidget;
    int numRows;
    int numCols;
    //
    // Complete imagle size in pixels
    //
    int curScreenWidth;
    int curScreenHeight;
    int recommendationIndex;
    //
    // Current (last) mouse position
    //
    QPoint mousePos;
    QVector<QVector<int>> screenTileRows;
    QVector<QVector<double>> matchRows;
    int curScreen;
    QVector<QVector<QVector<int>>> screenStore;
    QVector<QString> notesStore;
    QVector<QPair<double, int>> recommendations;
    NotesDialog notes;
    QRect notesGeometry;
    //
    // Depending on numCols, numRows, tileSize and MARGIN
    //
    bool mouseIsInsideScreen();
    //
    // Return cell coordinates or -1 if mouse is outside screen
    //
    QPoint mouseGridPos();
    //
    // Return overall score for a given tile and cell; look at all four neighbours
    //
    double calcMatchValue(
            Tile &tile,
            const int index,
            const int col,
            const int row,
            Tile::Filter filter
            );
    //
    // Store score into *matchValue and update *numNeighbours
    //
    void matchNeighbour(
            Tile &tile,
            const int index,
            const int col,
            const int row,
            Tile::Edge edge,
            Tile::Filter filter,
            int *numNeighbours,
            double *matchValue
            );
    //
    // Switch to screen, i.e. put screen and notes from store into current
    //
    void useScreen(int index);
    bool isEmpty(QVector<QVector<int>> screen);
    //
    // Returns deleted cell content
    //
    int clearCell(int x, int y);
    void initMatchRows();
};

#endif // SCREENLABEL_H
