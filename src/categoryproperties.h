/*
 * passwords - A GnuPG based password manager
 *
 * Copyright (C) 2025  Adam.Lanzafame <sysal@tuta.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef CATEGORYPROPERTIES_H
#define CATEGORYPROPERTIES_H

#include <QChartView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QMap>
#include <QSqlDatabase>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
class QPieSeries;
QT_END_NAMESPACE

struct CategoryNode {
    int id = 0;
    int parentId = 0;
    QString name;
    int directCount = 0;
    int totalCount = 0;
    QList<CategoryNode*> children;
};

class CategoryProperties : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryProperties(int selectedCategoryId, QWidget* parent = nullptr);
    ~CategoryProperties();

private:
    bool loadDatabase();
    void loadCategories();
    void loadApplicationCounts();
    int computeTotals(CategoryNode* node);
    void collectAll(CategoryNode* node, QList<CategoryNode*>& out);

    void buildPieChart();
    void buildTable();
    void adjustTableHeightForVisibleRows(int visibleRows);

private slots:
    void onHelpRequested();

private:
    int selectedId = 0;
    CategoryNode* root = nullptr;

    QMap<int, CategoryNode*> nodes;
    QSqlDatabase db;

    QChartView* chartView = nullptr;
    QTableWidget* tableWidget = nullptr;
    QDialogButtonBox* buttonBox = nullptr;
};

#endif // CATEGORYPROPERTIES_H
