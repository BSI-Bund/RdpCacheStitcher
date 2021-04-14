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

#include "screenlabel.h"
#include "notesdialog.h"

#include <QPainter>
#include <math.h>
#include <QProgressDialog>
#include <QApplication>
#include <QPlainTextEdit>

const int ScreenLabel::SCREEN_DEFAULT_WIDTH = 20;
const int ScreenLabel::SCREEN_DEFAULT_HEIGHT = 16;
const int ScreenLabel::SCREENSTORE_SIZE = 99;
const int ScreenLabel::CELL_EMPTY = -1;
const int ScreenLabel::CELL_LOCKED = -2;
const double ScreenLabel::MATCH_EMPTY = -1;

const int ScreenLabel::HEURISTIC_MAX_STORE_DISTANCE = 40;
const double ScreenLabel::HEURISTIC_RESIZED_FACTOR = 0.5;
double ScreenLabel::g_heurStoreDistanceWeight = 0.1;
double ScreenLabel::g_heurColorsThreshold = 0.08;
double ScreenLabel::g_heurColorsWeight = 0.4;

const QString ScreenLabel::CASEFILE_MAGIC("RCS_CASE");
const int ScreenLabel::CASEFILE_VERSION = 2;

ScreenLabel::ScreenLabel(TileStore *tileStore, TileStoreWidget *tileStoreWidget)
    : tileStore(tileStore),
      tileStoreWidget(tileStoreWidget),
      numRows(SCREEN_DEFAULT_HEIGHT),
      numCols(SCREEN_DEFAULT_WIDTH),
      curScreenWidth(tileStore->tileSize * SCREEN_DEFAULT_WIDTH + 2*MARGIN),
      curScreenHeight(tileStore->tileSize * SCREEN_DEFAULT_HEIGHT + 2*MARGIN),
      recommendationIndex(-1),
      curScreen(0)
{
    setMouseTracking(true);
    initScreens();
    useScreen(0);

    notes.setSizeGripEnabled(true);
    notesGeometry = notes.geometry();
}

void ScreenLabel::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    update();
}

void ScreenLabel::leaveEvent(QEvent *)
{
    mousePos.setX(-1);
    update();
}

void ScreenLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (tileStoreWidget->selectedIndex() != -1) {
            // Place selected tile
            QPoint gridPos = mouseGridPos();
            if (gridPos.x() != -1) {
                placeTile(gridPos.x(), gridPos.y(), tileStoreWidget->selectedIndex());
                tileStoreWidget->clearSelection();
                update();
            }
        } else {
            // Select screen tile for recommendations
            QPoint gridPos = mouseGridPos();
            if (gridPos.x() != -1 && screenTileRows.at(gridPos.y()).at(gridPos.x()) == CELL_EMPTY) {
                selectedPos.setX(gridPos.x());
                selectedPos.setY(gridPos.y());
                updateRecommendations();
                update();
            }
        }
    } else if (event->button() == Qt::MiddleButton) {
        // Auto-place best recommendation here
        QPoint gridPos = mouseGridPos();
        if (gridPos.x() != -1 && screenTileRows.at(gridPos.y()).at(gridPos.x()) == CELL_EMPTY) {
            selectedPos.setX(gridPos.x());
            selectedPos.setY(gridPos.y());
            updateRecommendations();
            if (recommendations.size() > 0 && recommendations.at(0).first > 0) {
                int tileIndex = recommendations.at(0).second;
                placeRecommendation(tileIndex);
                update();
            }
        }
    } else if (event->button() == Qt::RightButton) {
        QPoint gridPos = mouseGridPos();
        int xPos = gridPos.x();
        int yPos = gridPos.y();
        if (xPos != -1) {
            int erasedIndex = clearCell(xPos, yPos);
            emit availableTilesChanged();
            tileStoreWidget->selectTile(erasedIndex);
            updateMatchValues();
            updateRecommendations();
            update();
        }
    }
}

void ScreenLabel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setBackgroundMode(Qt::OpaqueMode);
    const int tileSize = tileStore->tileSize;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            const int xPos = MARGIN + col*tileSize;
            const int yPos = MARGIN + row*tileSize;
            const int tileIndex = screenTileRows.at(row).at(col);
            switch (tileIndex) {
                case CELL_LOCKED:
                case CELL_EMPTY:
                {
                    if (col == selectedPos.x() && row == selectedPos.y()) {
                        // Tile is selected for recommendations
                        if (recommendationIndex == -1) {
                            painter.fillRect(xPos, yPos, tileSize, tileSize, CELL_SELECTED);
                        } else {
                            painter.drawImage(xPos, yPos, tileStore->getTile(recommendationIndex).getImage());
                        }
                    } else {
                        // Display match value
                        double matchValue = matchRows.at(row).at(col);
                        if (matchValue >= TileStore::QUALITY_THRESHOLD) {
                            QColor matchCol;
                            // If unlikely matches are hidden, expand color range
                            matchCol.setRed(MAX_MATCH.red()*matchValue);
                            matchCol.setGreen(MAX_MATCH.green()*matchValue);
                            matchCol.setBlue(MAX_MATCH.blue()*matchValue);
                            painter.fillRect(xPos, yPos, tileSize, tileSize, matchCol);
                        } else {
                            painter.fillRect(xPos, yPos, tileSize, tileSize, COL_BACK);
                        }
                        painter.setPen(COL_GRID);
                        painter.drawRect(xPos, yPos, tileSize, tileSize);
                    }
                }
                break;
            default:
                painter.drawImage(xPos, yPos, tileStore->getTile(tileIndex).getImage());
                break;
            }
        }
    }

    // Hovering tile
    QPoint gridPos = mouseGridPos();
    if (gridPos.x() != -1 && tileStoreWidget->selectedIndex() != -1) {
        Tile tile = tileStore->getTile(tileStoreWidget->selectedIndex());
        painter.drawImage(MARGIN + gridPos.x()*tileSize, MARGIN + gridPos.y()*tileSize, tile.getImage());
    }
}

double ScreenLabel::calcMatchValue(Tile &tile, const int index, const int col, const int row, Tile::Filter filter) {
    double matchValue = 0.0;
    int numNeighbours = 0;
    matchNeighbour(tile, index, col - 1, row, Tile::Edge::Left, filter, &numNeighbours, &matchValue);
    matchNeighbour(tile, index, col + 1, row, Tile::Edge::Right, filter, &numNeighbours, &matchValue);
    matchNeighbour(tile, index, col, row - 1, Tile::Edge::Top, filter, &numNeighbours, &matchValue);
    matchNeighbour(tile, index, col, row + 1, Tile::Edge::Bottom, filter, &numNeighbours, &matchValue);
    if (numNeighbours > 0) {
        matchValue /= numNeighbours;
    } else {
        matchValue = 0;
    }
    return matchValue;
}

void ScreenLabel::updateMatchValues()
{
    const int selectedIndex = tileStoreWidget->selectedIndex();
    if (selectedIndex != -1) {
        Tile selectedTile = tileStore->getTile(selectedIndex);
        // Compute match values for each screen tile
        for (int row = 0; row < numRows; ++row) {
            auto matchRow = matchRows.at(row);
            for (int col = 0; col < numCols; ++col) {
                double matchValue = calcMatchValue(selectedTile, selectedIndex, col, row, curFilter);
                matchRow.replace(col, matchValue);
            }
            matchRows.replace(row, matchRow);
        }
    } else {
        // No tile selected: Clear match values
        for (int row = 0; row < numRows; ++row) {
            auto matchRow = matchRows.at(row);
            matchRow.fill(0, numCols);
            matchRows.replace(row, matchRow);
        }
    }
    selectedPos.setX(-1);
    updateRecommendations();    // Effectively clears them
    update();
}

void ScreenLabel::recommendationHover(int tileIndex)
{
    recommendationIndex = tileIndex;
    update();
}

void ScreenLabel::placeRecommendation(int tileIndex)
{
    if (tileIndex != -1 && selectedPos.x() != -1) {
        placeTile(selectedPos.x(), selectedPos.y(), tileIndex);
        selectedPos.setX(-1);
        updateRecommendations();    // Effectively clears them
        update();
    }
}

void ScreenLabel::screenNumberChanged(int newNumber)
{
    storeCurrentScreen();
    useScreen(newNumber - 1);
}

void ScreenLabel::autoplace()
{
    double bestMatch = 0.0;
    int bestIndex = -1;
    int bestCol;
    int bestRow;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int row = 1; row < numRows - 1; ++row) {
        for (int col = 1; col < numCols - 1; ++col) {
            if (screenTileRows.at(row).at(col) == CELL_EMPTY) {
                if (
                        (screenTileRows.at(row).at(col - 1) >= 0)
                        || (screenTileRows.at(row).at(col + 1) >= 0)
                        || (screenTileRows.at(row - 1).at(col) >= 0)
                        || (screenTileRows.at(row + 1).at(col) >= 0)
                    ) {
                    for (int i = 0; i < tileStore->size(); ++i) {
                        Tile tile = tileStore->getTile(i);
                        double matchValue = calcMatchValue(tile, i, col, row, curFilter);
                        if (matchValue > bestMatch && matchValue >= TileStore::QUALITY_THRESHOLD && !tileStore->isHidden(i)) {
                            bestIndex = i;
                            bestMatch = matchValue;
                            bestCol = col;
                            bestRow = row;
                        }
                    }
                }
            }
        }
    }
    QApplication::restoreOverrideCursor();
    if (bestIndex != -1) {
        placeTile(bestCol, bestRow, bestIndex);
        update();
    }
}

bool ScreenLabel::mouseIsInsideScreen()
{
    const int mX = mousePos.x();
    const int mY = mousePos.y();
    const int tileSize = tileStore->tileSize;
    return (tileStore->size() > 0 && mX != -1 &&
            mX >= MARGIN && mX < MARGIN + numCols*tileSize &&
            mY >= MARGIN && mY < MARGIN + numRows*tileSize);
}

QPoint ScreenLabel::mouseGridPos()
{
    if (mouseIsInsideScreen()) {
        const int tileSize = tileStore->tileSize;
        const int tileCol = (mousePos.x() - MARGIN)/tileSize;
        const int tileRow = (mousePos.y() - MARGIN)/tileSize;
        return QPoint(tileCol, tileRow);
    } else {
        return QPoint(-1, -1);
    }
}

void ScreenLabel::matchNeighbour(
        Tile &tile,
        const int index,
        const int col,
        const int row,
        Tile::Edge edge,
        Tile::Filter filter,
        int *numNeighbours,
        double *matchValue)
{
    if (col >= 0 && col < numCols && row >= 0 && row < numRows && screenTileRows.at(row).at(col) >= 0) {
        *numNeighbours += 1;
        const int otherIndex = screenTileRows.at(row).at(col);
        Tile other = tileStore->getTile(screenTileRows.at(row).at(col));
        *matchValue += tile.calcEdgeSimilarity(other, filter, edge);
        // Confidence factors
        // Tile store distance
        int storeDistance = std::min(abs(index - otherIndex), HEURISTIC_MAX_STORE_DISTANCE);
        double distFactor = sqrt(storeDistance*storeDistance)/HEURISTIC_MAX_STORE_DISTANCE;
        *matchValue *= (1.0 - g_heurStoreDistanceWeight*distFactor);
        // Number of edge colors
        const double thisNumColors = double(tile.getNumUniqueEdgeColors(edge, filter) - 1);
        const double otherNumColors = double(other.getNumUniqueEdgeColors(other.oppositeEdge(edge), filter) - 1);
        const double numColors = std::min(thisNumColors, otherNumColors);
        const int numColorsThreshold = tile.size*g_heurColorsThreshold;
        if (numColors <= numColorsThreshold) {
            double decreaseFactor = (numColors/numColorsThreshold)*g_heurColorsWeight;
            *matchValue *= (1.0 - g_heurColorsWeight + decreaseFactor);
        }
        // Resized
        if (tile.isResized && edge == Tile::Edge::Bottom) {
            *matchValue *= HEURISTIC_RESIZED_FACTOR;
        }
    }
}

//
// Helper function for sorting
//
bool qpLess(const QPair<double, int> &a, const QPair<double, int> &b) {
    return a.first > b.first;
}

void ScreenLabel::updateRecommendations()
{
    recommendations.clear();
    if (selectedPos.x() != -1) {
        const int col = selectedPos.x();
        const int row = selectedPos.y();
        for (int i = 0; i < tileStore->size(); ++i) {
            Tile tile = tileStore->getTile(i);
            double matchValue = calcMatchValue(tile, i, col, row, Tile::Filter::Gauss15);
            if (matchValue > 0 && !tileStore->isHidden(i)) {
                recommendations.append(QPair<double, int>(matchValue, i));
            }
        }
    }
    std::sort(recommendations.begin(), recommendations.end(), qpLess);
    emit recommendationsChanged();
}

void ScreenLabel::useScreen(int index)
{
    // Screen
    tileStoreWidget->clearSelection();
    curScreen = index;
    screenTileRows = screenStore.at(curScreen);
    numRows = screenTileRows.size();
    numCols = screenTileRows.at(0).size();
    curScreenWidth = tileStore->tileSize * numCols + 2*MARGIN;
    curScreenHeight =tileStore->tileSize * numRows + 2*MARGIN;
    resize(curScreenWidth, curScreenHeight);
    setPixmap(QPixmap::fromImage(QImage(curScreenWidth, curScreenHeight, QImage::Format_ARGB32)));
    // Notes
    QPlainTextEdit *textEdit = notes.findChild<QPlainTextEdit *>("screenNotesEdit");
    Q_ASSERT(textEdit != NULL);
    textEdit->setPlainText(notesStore.at(curScreen));
    // Propagate update
    initMatchRows();
    selectedPos.setX(-1);
    recommendationIndex = -1;
    updateRecommendations();
    update();
}

void ScreenLabel::initMatchRows() {
    matchRows.clear();
    for (int i = 0; i < numRows; ++i) {
        QVector<double> rm(numCols, MATCH_EMPTY);
        matchRows.push_back(rm);
    }
}

void ScreenLabel::initScreens()
{
    // Init screen store
    screenStore.clear();
    for (int s = 0; s < SCREENSTORE_SIZE; ++s) {
        QVector<QVector<int>> screen;
        for (int i = 0; i < SCREEN_DEFAULT_HEIGHT; ++i) {
            QVector<int> row(SCREEN_DEFAULT_WIDTH, CELL_EMPTY);
            screen.push_back(row);
        }
        screenStore.push_back(screen);
    }
    // Init notes store
    for (int i = 0; i < SCREENSTORE_SIZE; ++i) {
        QString s;
        notesStore.push_back(s);
    }

    // Clear and init current screen
    screenTileRows.clear();
    numRows = SCREEN_DEFAULT_HEIGHT;
    numCols = SCREEN_DEFAULT_WIDTH;
    curScreenWidth = tileStore->tileSize * SCREEN_DEFAULT_WIDTH + 2*MARGIN;
    curScreenHeight = tileStore->tileSize * SCREEN_DEFAULT_HEIGHT + 2*MARGIN;
    // Init tile matrix
    for (int i = 0; i < SCREEN_DEFAULT_HEIGHT; ++i) {
        QVector<int> rt(SCREEN_DEFAULT_WIDTH, CELL_EMPTY);
        screenTileRows.push_back(rt);
    }
    initMatchRows();
    modified = false;
}

bool ScreenLabel::isEmpty(QVector<QVector<int>> screen)
{
    for (int row = 0; row < screen.size(); ++row) {
        auto curRow = screen.at(row);
        for (int col = 0; col < curRow.size(); ++col) {
            if (curRow.at(col) >= 0) {
                return false;
            }
        }
    }
    return true;
}

void ScreenLabel::placeTile(int x, int y, int tileIndex)
{
    clearCell(x, y);
    auto row = screenTileRows.at(y);
    row.replace(x, tileIndex);
    tileStore->incUseCount(tileIndex);
    screenTileRows.replace(y, row);
    modified = true;
    // Check if screen has to grow
    if (x <= 1) {
        for (int row = 0; row < screenTileRows.size(); ++row) {
            auto tileRow = screenTileRows.at(row);
            tileRow.prepend(CELL_EMPTY);
            screenTileRows.replace(row, tileRow);
            auto matchRow = matchRows.at(row);
            matchRow.prepend(0);
            matchRows.replace(row, matchRow);
        }
        numCols++;
        storeCurrentScreen();
        useScreen(curScreen);
    } else if (x >= numCols - 2) {
        for (int row = 0; row < screenTileRows.size(); ++row) {
            auto tileRow = screenTileRows.at(row);
            tileRow.append(CELL_EMPTY);
            screenTileRows.replace(row, tileRow);
            auto matchRow = matchRows.at(row);
            matchRow.append(0);
            matchRows.replace(row, matchRow);
        }
        numCols++;
        storeCurrentScreen();
        useScreen(curScreen);
    }
    if (y <= 1) {
        QVector<int> newRow(numCols, CELL_EMPTY);
        screenTileRows.prepend(newRow);
        QVector<double> newMatchRow(numCols, 0);
        matchRows.prepend(newMatchRow);
        numRows++;
        storeCurrentScreen();
        useScreen(curScreen);
    } else if (y >= numRows - 2) {
        QVector<int> newRow(numCols, CELL_EMPTY);
        screenTileRows.append(newRow);
        QVector<double> newMatchRow(numCols, 0);
        matchRows.append(newMatchRow);
        numRows++;
        storeCurrentScreen();
        useScreen(curScreen);
    }
    emit availableTilesChanged();
}

int ScreenLabel::clearCell(int x, int y)
{
    auto row = screenTileRows.at(y);
    int erasedIndex = row.at(x);
    if (erasedIndex >= 0) {
        tileStore->decUseCount(erasedIndex);
    }
    row.replace(x, CELL_EMPTY);
    screenTileRows.replace(y, row);
    modified = true;
    return erasedIndex;
}

NotesDialog *ScreenLabel::getNotesDialog()
{
    return &notes;
}

bool ScreenLabel::isModified() const
{
    return modified;
}

void ScreenLabel::clearModified()
{
    modified = false;
}

void ScreenLabel::notesWindowStateChanges(int state)
{
    if (state == Qt::Checked) {
        notes.show();
        notes.setGeometry(notesGeometry);
    } else {
        notesGeometry = notes.geometry();
        notes.hide();
    }
}

void ScreenLabel::notesModified(bool)
{
    modified = true;
}

QVector<QPair<double, int>> *ScreenLabel::getRecommendations()
{
    return &recommendations;
}

QString ScreenLabel::saveCase(QString filename)
{
    if (!filename.endsWith(".rcs")) {
        filename.append(".rcs");
    }
    QFile caseFile(filename);
    if (!caseFile.open(QIODevice::WriteOnly)) {
        return "Unable to open file for writing!";
    }
    QDataStream out(&caseFile);
    out << CASEFILE_MAGIC;
    out << (quint32)CASEFILE_VERSION;
    out.setVersion(QDataStream::Qt_5_9);
    QString result = tileStore->saveData(out);
    if (!result.isEmpty()) {
        return result;
    }
    result = saveData(out);
    if (!result.isEmpty()) {
        return result;
    }
    modified = false;
    return "";
}

QString ScreenLabel::loadCase(QString filename) {
    QFile caseFile(filename);
    if (!caseFile.open(QIODevice::ReadOnly)){
        return "Unable to open file!";
    }
    QDataStream in(&caseFile);
    QString magic;
    in >> magic;
    if (magic != CASEFILE_MAGIC) {
        return "Not a valid .rcs case file!";
    }
    quint32 version;
    in >> version;
    if (version > CASEFILE_VERSION) {
        return "Case file has been created by a newer version of this program!";
    }
    in.setVersion(QDataStream::Qt_5_9);
    QString result = tileStore->loadData(in);
    if (!result.isEmpty()) {
        return result;
    }
    bool loadNotes = (version >= 2);
    result = loadData(in, loadNotes);
    if (!result.isEmpty()) {
        return result;
    }
    modified = false;
    return "";
}

QString ScreenLabel::saveData(QDataStream &out)
{
    // Construct vector of screens that actually contain data
    QVector<QVector<QVector<int>>> toSave;
    QVector<QString> notesToSave;
    for (int i = 0; i < screenStore.size(); ++i) {
        if (!isEmpty(screenStore.at(i))) {
            toSave.append(screenStore.at(i));
            notesToSave.append(notesStore.at(i));
        }
    }
    out << toSave;
    out << notesToSave;
    modified = false;
    return "";
}

QString ScreenLabel::loadData(QDataStream &in, bool loadNotes)
{
    screenStore.clear();
    in >> screenStore;
    // Fill up rest of screen store with empty screens
    while (screenStore.size() < SCREENSTORE_SIZE) {
        QVector<QVector<int>> screen;
        for (int i = 0; i < SCREEN_DEFAULT_HEIGHT; ++i) {
            QVector<int> row(SCREEN_DEFAULT_WIDTH, CELL_EMPTY);
            screen.push_back(row);
        }
        screenStore.push_back(screen);
    }
    notesStore.clear();
    if (loadNotes) {
        in >> notesStore;
    }
    // Fill up rest of notes store
    while (notesStore.size() < SCREENSTORE_SIZE) {
        QString s;
        notesStore.push_back(s);
    }
    modified = false;
    return "";
}

QString ScreenLabel::exportScreens(QString prefix)
{
    storeCurrentScreen();
    // Iterate over screens and export used ones
    int exportNum = 0;
    QString notesString;
    for (int s = 0; s < screenStore.size(); ++s) {
        if (!isEmpty(screenStore.at(s))) {
            QVector<QVector<int>> screen = screenStore.at(s);
            // Determine image boundaries
            int top = 999999;
            int right = -1;
            int bottom = -1;
            int left = 999999;
            const int width = screen.at(0).size();
            const int height = screen.size();
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    if (screen.at(row).at(col) >= 0) {
                        top = std::min(row, top);
                        right = std::max(col, right);
                        bottom = std::max(row, bottom);
                        left = std::min(col, left);
                    }
                }
            }
            // Create image
            QImage image((right - left + 1)*tileStore->tileSize, (bottom - top + 1)*tileStore->tileSize, QImage::Format_ARGB32);
            image.fill(Qt::GlobalColor::transparent);
            QPainter painter;
            painter.begin(&image);
            for (int row = top; row <= bottom; ++row) {
                for (int col = left; col <= right; ++col) {
                    const int tileIndex = screen.at(row).at(col);
                    if (tileIndex >= 0) {
                        painter.drawImage((col - left)*tileStore->tileSize, (row - top)*tileStore->tileSize, tileStore->getTile(tileIndex).getImage());
                    }
                }
            }
            painter.end();
            QString filename = prefix + "_" + QString("%1").arg(++exportNum, 2, 10, QChar('0')) + ".png";
            if (!image.save(filename)) {
                return "Unable to save image!";
            }
            // Add notes, if present
            if (!notesStore.at(s).isEmpty()) {
                notesString.append(filename + ":\n" + notesStore.at(s).trimmed() + "\n\n");
            }
        }
    }
    // Write notes textfile
    if (!notesString.isEmpty()) {
        QFile notesFile(prefix + ".txt");
        if (notesFile.open(QIODevice::ReadWrite)) {
            if (!notesFile.write(notesString.toUtf8())) {
                return "Unable to write notes!";
            }
        } else {
            return "Unable to open notes file for writing!";
        }
    }
    return "";
}

void ScreenLabel::storeCurrentScreen()
{
    screenStore.replace(curScreen, screenTileRows);
    QPlainTextEdit *textEdit = notes.findChild<QPlainTextEdit *>("screenNotesEdit");
    Q_ASSERT(textEdit != NULL);
    notesStore.replace(curScreen, textEdit->document()->toPlainText());
}

void ScreenLabel::tileStoreChanged()
{
    useScreen(0);
}
