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

#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QImage>
#include <QColor>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <QtGlobal>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QDockWidget>
#include <string.h>
#include <math.h>
#include <QPlainTextEdit>

#include "ui_mainwindow.h"
#include "tilestore.h"
#include "screenlabel.h"
#include "recommendationslabel.h"
#include "tilestorewidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle("RdpCacheStitcher " + MainWindow::PROGRAM_VERSION);

    // Tile store
    QImage startImage(":/images/data/RCS_start_screen.jpg");
    TileStore store(startImage, 64);
    w.registerTileStore(&store);
    TileStoreWidget *tileStoreWidget = new TileStoreWidget(&store);
    QDockWidget storeDock("Tile store", &w);
    storeDock.setStyleSheet("::title { text-align: center }");
    storeDock.setAllowedAreas(Qt::BottomDockWidgetArea);
    storeDock.setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    storeDock.setWidget(tileStoreWidget);
    w.addDockWidget(Qt::BottomDockWidgetArea, &storeDock);

    // Screen
    ScreenLabel *screenLabel = new ScreenLabel(&store, tileStoreWidget);
    w.registerScreenLabel(screenLabel);
    QScrollArea *screenArea = new QScrollArea();
    screenArea->setBackgroundRole(QPalette::Light);
    screenArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    screenArea->setWidget(screenLabel);
    screenArea->verticalScrollBar()->setValue(3*64);
    screenArea->horizontalScrollBar()->setValue(3*64);
    // Init tutorial start screen
    const int numRows = startImage.height()/64;
    const int numCols = startImage.width()/64;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            screenLabel->placeTile(col + 2, row + 2, row*numCols + col);
        }
    }
    tileStoreWidget->tileStoreChanged();
    screenLabel->clearModified();

    // Best match area
    RecommendationsLabel *recommendationsLabel = new RecommendationsLabel(&store, screenLabel->getRecommendations());
    QScrollArea *recommendationsArea = new QScrollArea();
    recommendationsArea->setBackgroundRole(QPalette::Light);
    recommendationsArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    recommendationsArea->setWidget(recommendationsLabel);
    recommendationsArea->setMinimumWidth(recommendationsLabel->getCellSize() + recommendationsArea->verticalScrollBar()->width());
    recommendationsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Combine Screen and Best Match
    QWidget *upperWidget = w.findChild<QWidget *>("workAreaWidget");
    QHBoxLayout *upperLayout = new QHBoxLayout();
    upperLayout->addWidget(screenArea);
    upperLayout->addWidget(recommendationsArea);
    upperLayout->setStretch(0, 1);
    upperLayout->setStretch(1, 0);
    upperWidget->setLayout(upperLayout);

    // Connect signals and slots
    QObject::connect(tileStoreWidget, SIGNAL(itemSelectionChanged()), screenLabel, SLOT(updateMatchValues()));
    QObject::connect(screenLabel, SIGNAL(recommendationsChanged()), recommendationsLabel, SLOT(recommendationsChanged()));
    QObject::connect(recommendationsLabel, SIGNAL(recommendationHover(int)), screenLabel, SLOT(recommendationHover(int)));
    QObject::connect(recommendationsLabel, SIGNAL(placeRecommendation(int)), screenLabel, SLOT(placeRecommendation(int)));
    QObject::connect(&w, SIGNAL(tileStoreChanged()), screenLabel, SLOT(tileStoreChanged()));
    QObject::connect(&w, SIGNAL(tileStoreChanged()), tileStoreWidget, SLOT(tileStoreChanged()));
    QObject::connect(&w, SIGNAL(tileStoreChanged()), recommendationsLabel, SLOT(tileStoreChanged()));
    QObject::connect(&store, SIGNAL(availableTilesChanged()), tileStoreWidget, SLOT(tileStoreChanged()));
    QObject::connect(&store, SIGNAL(availableTilesChanged()), screenLabel, SLOT(updateRecommendations()));
    QObject::connect(screenLabel, SIGNAL(availableTilesChanged()), tileStoreWidget, SLOT(tileStoreChanged()));

    NotesDialog *notesDialog = screenLabel->getNotesDialog();
    Q_ASSERT(notesDialog != NULL);
    QPlainTextEdit *textEdit = notesDialog->findChild<QPlainTextEdit *>("screenNotesEdit");
    Q_ASSERT(textEdit != NULL);
    QObject::connect(textEdit, SIGNAL(modificationChanged(bool)), screenLabel, SLOT(notesModified(bool)));

    QSpinBox *screenNumberSpinBox = w.findChild<QSpinBox *>("screenNumberSpinBox");
    Q_ASSERT(screenNumberSpinBox != NULL);
    QObject::connect(screenNumberSpinBox, SIGNAL(valueChanged(int)), screenLabel, SLOT(screenNumberChanged(int)));

    QCheckBox *screenNotesCB = w.findChild<QCheckBox *>("screenNotesCheckBox");
    Q_ASSERT(screenNotesCB != NULL);
    QObject::connect(screenNotesCB, SIGNAL(stateChanged(int)), screenLabel, SLOT(notesWindowStateChanges(int)));
    QObject::connect(screenLabel->getNotesDialog(), SIGNAL(notesDialogClosed()), &w, SLOT(notesDialogClosed()));

    QCheckBox *hideUsedCB = w.findChild<QCheckBox *>("hideUsedCheckBox");
    Q_ASSERT(hideUsedCB != NULL);
    QObject::connect(hideUsedCB, SIGNAL(stateChanged(int)), &store, SLOT(hideUsedChanged(int)));

    QCheckBox *hideDuplicatesCB = w.findChild<QCheckBox *>("hideDuplicatesCheckBox");
    Q_ASSERT(hideDuplicatesCB != NULL);
    QObject::connect(hideDuplicatesCB, SIGNAL(stateChanged(int)), &store, SLOT(hideDuplicatesChanged(int)));

    QCheckBox *hideNonSquareCB = w.findChild<QCheckBox *>("hideNonSquareCheckBox");
    Q_ASSERT(hideNonSquareCB != NULL);
    QObject::connect(hideNonSquareCB, SIGNAL(stateChanged(int)), &store, SLOT(hideNonSquareChanged(int)));

    QSpinBox *storeWidthSpinBox = w.findChild<QSpinBox *>("storeWidthSpinBox");
    Q_ASSERT(storeWidthSpinBox != NULL);
    QObject::connect(storeWidthSpinBox, SIGNAL(valueChanged(int)), tileStoreWidget, SLOT(tileStoreWidthChanged(int)));

    QPushButton *autoPlaceButton = w.findChild<QPushButton *>("autoPlaceButton");
    Q_ASSERT(autoPlaceButton != NULL);
    QObject::connect(autoPlaceButton, SIGNAL(pressed()), screenLabel, SLOT(autoplace()));

    w.show();

    return a.exec();
}
