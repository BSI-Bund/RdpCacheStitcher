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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>

#include "screenlabel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static const QString PROGRAM_VERSION;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //
    // "Global" routine: Displays a message in an "OK" messagebox
    //
    static void displayMessage(const QString &message);
    void registerTileStore(TileStore *tileStore);
    void registerScreenLabel(ScreenLabel *screenLabel);

    void closeEvent(QCloseEvent *event) override;

signals:
    void tileStoreChanged();

public slots:
    //
    // Unchecks the screen notes checkbox
    //
    void notesDialogClosed();

private:
    Ui::MainWindow *ui;
    TileStore *tileStore = NULL;
    ScreenLabel *screenLabel = NULL;
    //
    // Remember path between dialogs
    //
    QString caseDialogPath;

    bool confirmIfModified(QString title);

private slots:
    void on_actionNew_case_triggered();
    void on_actionSave_case_as_triggered();
    void on_actionSave_case_triggered();
    void on_actionOpen_case_triggered();
    void on_actionExport_screen_images_triggered();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();

};

#endif // MAINWINDOW_H
