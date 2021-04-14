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
#include "ui_mainwindow.h"
#include "aboutdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

const QString MainWindow::PROGRAM_VERSION("1.1");

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/data/icon.ico"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayMessage(const QString &message) {

    QMessageBox msgBox(QMessageBox::NoIcon,
                       "ERROR",
                       message,
                       QMessageBox::Ok, QApplication::activeWindow(),
                       Qt::FramelessWindowHint);
    msgBox.exec();
}

void MainWindow::registerTileStore(TileStore *tileStore)
{
    this->tileStore = tileStore;
}

void MainWindow::registerScreenLabel(ScreenLabel *screenLabel)
{
    this->screenLabel = screenLabel;
}

void MainWindow::on_actionNew_case_triggered()
{
    if (confirmIfModified("New case")) {
        QFileDialog dialog(this);
        dialog.setWindowTitle("New case: Please select a directory with .bmp RDP cache images");
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::DirectoryOnly);
        dialog.setOption(QFileDialog::ShowDirsOnly, false);
        dialog.setViewMode(QFileDialog::Detail);
        if (dialog.exec() == QDialog::Accepted) {
            QString dir = dialog.directory().absolutePath();
            Q_ASSERT(tileStore != NULL);
            setUpdatesEnabled(false);
            QString result = tileStore->loadTiles(dir);
            if (!result.isEmpty()) {
                displayMessage("Error while loading tiles:\n" + result);
            }
            emit tileStoreChanged();
            screenLabel->initScreens();
            ui->screenNumberSpinBox->setValue(1);
            setUpdatesEnabled(true);
            setWindowTitle("Unnamed case");
        }
    }
}

void MainWindow::on_actionSave_case_as_triggered()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Please choose a path and filename for saving the case file to");
    dialog.setDirectory(caseDialogPath);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter("*.rcs");
    dialog.setDefaultSuffix("rcs");
    dialog.setViewMode(QFileDialog::Detail);
    dialog.selectFile(tileStore->name);
    if (dialog.exec()) {
        QStringList fileNames = dialog.selectedFiles();
        if (!fileNames.isEmpty()) {
            QString filename = fileNames[0];
            caseDialogPath = dialog.directory().absolutePath();
            tileStore->name = filename;
            Q_ASSERT(screenLabel != NULL);
            screenLabel->storeCurrentScreen();
            QString result = screenLabel->saveCase(filename);
            setWindowTitle(filename);
            if (!result.isEmpty()) {
                displayMessage("Error while saving case:\n" + result);
            }
        }
    }
}

void MainWindow::on_actionSave_case_triggered()
{
    if (tileStore->name.size() == 0) {
        on_actionSave_case_as_triggered();
    } else {
        Q_ASSERT(screenLabel != NULL);
        screenLabel->storeCurrentScreen();
        QString result = screenLabel->saveCase(tileStore->name);
        if (!result.isEmpty()) {
            displayMessage("Error while saving case:\n" + result);
        }
    }
}

void MainWindow::on_actionOpen_case_triggered()
{
    if (confirmIfModified("Open case")) {
        QFileDialog dialog(this);
        dialog.setWindowTitle("Please select a .rcs case file for loading");
        dialog.setDirectory(caseDialogPath);
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setNameFilter("*.rcs");
        dialog.setDefaultSuffix("rcs");
        dialog.setViewMode(QFileDialog::Detail);
        if (dialog.exec()) {
            QStringList fileNames = dialog.selectedFiles();
            if (!fileNames.isEmpty()) {
                QString filename = fileNames[0];
                caseDialogPath = dialog.directory().absolutePath();
                tileStore->name = filename;
                Q_ASSERT(screenLabel != NULL);
                setUpdatesEnabled(false);
                QString result = screenLabel->loadCase(filename);
                if (!result.isEmpty()) {
                    displayMessage("Error while loading case file:\n" + result);
                }
                ui->screenNumberSpinBox->setValue(1);
                emit tileStoreChanged();
                setUpdatesEnabled(true);
                setWindowTitle(filename);
            }
        }
    }
}

void MainWindow::on_actionExport_screen_images_triggered()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Please select a directory and prefix for the screen images export");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setViewMode(QFileDialog::Detail);
    QString suggestion = tileStore->name;
    while(suggestion.endsWith(".rcs")) {
        suggestion.truncate(suggestion.length() - 4);
    }
    dialog.selectFile(suggestion);
    if (dialog.exec()) {
        QStringList fileNames = dialog.selectedFiles();
        if (!fileNames.isEmpty()) {
            QString prefix = fileNames[0];
            while(prefix.endsWith(".png")) {
                prefix.truncate(prefix.length() - 4);
            }
            QString result = screenLabel->exportScreens(prefix);
            if (!result.isEmpty()) {
                displayMessage("Error while exporting screen images:\n" + result);
            }
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    if (confirmIfModified("Exit program")) {
        QApplication::quit();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    on_actionExit_triggered();
    event->ignore();
}

void MainWindow::notesDialogClosed()
{
    ui->screenNotesCheckBox->setCheckState(Qt::Unchecked);
}

bool MainWindow::confirmIfModified(QString title)
{
    if (!screenLabel->isModified()) {
        return true;
    }
    QMessageBox msgBox(QMessageBox::NoIcon,
                       title,
                       "Do you really want to discard all changes since last save?",
                       QMessageBox::Yes | QMessageBox::No, this);
    return (msgBox.exec() == QMessageBox::Yes);
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog about;
    about.setWindowTitle("About RdpCacheStitcher " + PROGRAM_VERSION);
    about.exec();
}
