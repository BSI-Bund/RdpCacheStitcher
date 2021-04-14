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

#ifndef NOTESDIALOG_H
#define NOTESDIALOG_H

#include <QDialog>

namespace Ui {
class NotesDialog;
}

class NotesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotesDialog(QWidget *parent = 0);
    ~NotesDialog();

    void reject() override;

signals:
    void notesDialogClosed();

private:
    Ui::NotesDialog *ui;
};

#endif // NOTESDIALOG_H
