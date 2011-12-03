/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include <mt32emu/ROMInfo.h>

#include "Master.h"
#include "ROMSelectionDialog.h"
#include "ui_ROMSelectionDialog.h"

using namespace MT32Emu;

static const int CHECKBOX_COLUMN = 0;
static const int FILENAME_COLUMN = 1;
static const int ROM_TYPE_COLUMN = 4;

ROMSelectionDialog::ROMSelectionDialog(QWidget *parent) :
		QDialog(parent),
		ui(new Ui::ROMSelectionDialog),
		controlROMGroup(this),
		pcmROMGroup(this),
		master(*Master::getInstance())
{
	ui->setupUi(this);

	QStringList fileFilter;

	fileFilter.clear();
	fileFilter.append("*.ROM");
	fileFilter.append("*.BIN");
	ui->fileFilterCombo->addItem(fileFilterToString(fileFilter), QVariant(fileFilter));
	fileFilter.clear();
	fileFilter.append("*.*");
	ui->fileFilterCombo->addItem(fileFilterToString(fileFilter), QVariant(fileFilter));
}

ROMSelectionDialog::~ROMSelectionDialog() {
	delete ui;
}

const QString ROMSelectionDialog::fileFilterToString(const QStringList fileFilter) const {
	QString s;
	for (QStringListIterator it(fileFilter); it.hasNext(); s.append(';')) s.append(it.next());
	if (s.size() > 0) return s.left(s.size() - 1);
	return NULL;
}

void ROMSelectionDialog::loadROMInfos(QString s) {
	if (romDir.absolutePath() != s) {
		romDir.setPath(s);
		refreshROMInfos();
	}
}

void ROMSelectionDialog::clearButtonGroup(QButtonGroup &group) {
	QListIterator<QAbstractButton *> it = QListIterator<QAbstractButton *>(group.buttons());
	for (;it.hasNext();) {
		group.removeButton(it.next());
	}
}

void ROMSelectionDialog::refreshROMInfos() {
	QString controlROMFileName;
	QString pcmROMFileName;
	master.getROMFileNames(controlROMFileName, pcmROMFileName);

	clearButtonGroup(controlROMGroup);
	clearButtonGroup(pcmROMGroup);
	controlROMRow = -1;
	pcmROMRow = -1;

	QStringList fileFilter = ui->fileFilterCombo->itemData(ui->fileFilterCombo->currentIndex()).value<QStringList>();
	QStringList dirEntries = romDir.entryList(fileFilter);
	ui->romInfoTable->clearContents();
	ui->romInfoTable->setRowCount(dirEntries.size());

	int row = 0;
	for (QStringListIterator it(dirEntries); it.hasNext();) {
		QString fileName = it.next();
		FileStream file;
		if (!file.open((romDir.absolutePath() + QDir::separator() + fileName).toUtf8())) continue;
		const ROMInfo &romInfo = *ROMInfo::getROMInfo(&file);
		if (&romInfo == NULL) continue;

		QButtonGroup *romGroup;
		QString romType;
		if (romInfo.type == ROMInfo::PCM) {
			romType = QString("PCM");
			romGroup = &pcmROMGroup;
			if (pcmROMRow == -1) pcmROMRow = row;
		} else if (romInfo.type == ROMInfo::Control) {
			romType = QString("Control");
			romGroup = &controlROMGroup;
			if (controlROMRow == -1) controlROMRow = row;
	  } else if (romInfo.type == ROMInfo::Reverb) romType = QString("Reverb");
		else continue;

		if (master.getROMDir() == romDir) {
			if (fileName == controlROMFileName) controlROMRow = row;
			else if (fileName == pcmROMFileName) pcmROMRow = row;
		}

		int column = 0;
		QCheckBox *checkBox = new QCheckBox();
		if (romInfo.type != ROMInfo::Reverb) {
			romGroup->addButton(checkBox);
			romGroup->setId(checkBox, row);
		} else checkBox->setDisabled(true);
		ui->romInfoTable->setCellWidget(row, column++, checkBox);

		if (controlROMRow == row || pcmROMRow == row) {
			checkBox->setChecked(true);
		}

		QTableWidgetItem *item = new QTableWidgetItem(fileName);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(QString((const char *)romInfo.shortName));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(QString((const char *)romInfo.description));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(romType);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(QString((const char *)romInfo.sha1Digest));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		row++;
	}
	ui->romInfoTable->setRowCount(row);
	ui->romInfoTable->resizeColumnsToContents();
}

void ROMSelectionDialog::on_fileFilterCombo_currentIndexChanged(int index) {
Q_UNUSED(index);

	refreshROMInfos();
}

void ROMSelectionDialog::accept() {
	QDialog::accept();
	controlROMRow = controlROMGroup.checkedId();
	pcmROMRow = pcmROMGroup.checkedId();
	master.setROMDir(romDir);
	QString controlROMFileName = NULL;
	QString pcmROMFileName = NULL;
	if (controlROMRow > -1) controlROMFileName = ui->romInfoTable->item(controlROMRow, FILENAME_COLUMN)->text();
	if (pcmROMRow > -1) pcmROMFileName = ui->romInfoTable->item(pcmROMRow, FILENAME_COLUMN)->text();
	master.setROMFileNames(controlROMFileName, pcmROMFileName);
}

void ROMSelectionDialog::reject() {
	QDialog::reject();
	if (controlROMRow > -1) controlROMGroup.button(controlROMRow)->setChecked(true);
	if (pcmROMRow > -1) pcmROMGroup.button(pcmROMRow)->setChecked(true);
}