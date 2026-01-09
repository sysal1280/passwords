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


#include "categoryproperties.h"
#include "dataobfuscator.h"

#include <QApplication>
#include <QChart>
#include <QDebug>
#include <QHeaderView>
#include <QLocale>
#include <QMessageBox>
#include <QPieSeries>
#include <QSqlError>
#include <QSqlQuery>
#include <QToolTip>
#include <QUuid>
#include <QVBoxLayout>

CategoryProperties::CategoryProperties(int selectedCategoryId, QWidget* parent)
    : QDialog(parent), selectedId(selectedCategoryId)
{
    setWindowTitle("Properties");
    resize(700, 600);

    auto* layout = new QVBoxLayout(this);

    // --- Chart ---
    chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(chartView);

    // --- Table ---
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << "Category" << "Number of passwords");
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setSortingEnabled(true);
    tableWidget->setAlternatingRowColors(false);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->verticalHeader()->setDefaultSectionSize(22);
    layout->addWidget(tableWidget);

    // --- Button box ---
    buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Help,
        Qt::Horizontal,
        this
        );
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::helpRequested,
            this, &CategoryProperties::onHelpRequested);

    // Let chart dominate; table and buttons take minimal space
    layout->setStretch(0, 4); // chart
    layout->setStretch(1, 0); // table (fixed height)
    layout->setStretch(2, 0); // buttons

    // --- Load data ---
    if (!loadDatabase()) {
        qWarning() << "Database failed to open";
        return;
    }

    loadCategories();
    loadApplicationCounts();

    /*
     * Close db
     */
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase("categoryprops_conn");

    if (!nodes.contains(selectedId)) {
        qWarning() << "Selected category ID not found:" << selectedId;
        return;
    }

    root = nodes[selectedId];

    buildPieChart();
    buildTable();

    setMaximumSize(450,550);
}

CategoryProperties::~CategoryProperties()
{
    for (auto* node : nodes.values())
        delete node;
}

bool CategoryProperties::loadDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString(QUuid::WithoutBraces));
    db.setDatabaseName(qApp->property("dbFile").toString());

    if (!db.open()) {
        qWarning() << "DB open error:" << db.lastError();
        return false;
    }
    return true;
}

void CategoryProperties::loadCategories()
{
    QSqlQuery q(db);
    q.prepare("SELECT id, parent_id, text FROM categories ORDER BY id");

    if (!q.exec()) {
        qWarning() << "Category query failed:" << q.lastError();
        return;
    }

    while (q.next()) {
        int id = q.value(0).toInt();
        int parentId = q.value(1).isNull() ? 0 : q.value(1).toInt();

        QString encryptedText = q.value(2).toString();
        QString decryptedText = DataObfuscator::deobfuscate(
            encryptedText,
            qApp->property("appKey").toByteArray()
            );

        auto* node = new CategoryNode;
        node->id = id;
        node->parentId = parentId;
        node->name = decryptedText;

        nodes[id] = node;
    }

    // Build tree
    const auto &map = nodes;   // prevent detach during iteration

    for (auto* node : map) {
        if (node->parentId != 0 && nodes.contains(node->parentId))
            nodes[node->parentId]->children.append(node);
    }

}

void CategoryProperties::loadApplicationCounts()
{
    const auto &map = nodes;   // prevent detach
    for (auto* node : map)
        node->directCount = 0;

    QSqlQuery q(db);
    q.prepare("SELECT category_id, COUNT(*) FROM application GROUP BY category_id");

    if (!q.exec()) {
        qWarning() << "Application count query failed:" << q.lastError();
        return;
    }

    while (q.next()) {
        int catId = q.value(0).toInt();
        int count = q.value(1).toInt();

        if (nodes.contains(catId))
            nodes[catId]->directCount = count;
    }
}

int CategoryProperties::computeTotals(CategoryNode* node)
{
    int sum = node->directCount;

    const auto &children = node->children;   // prevent detach
    for (auto* child : children)
        sum += computeTotals(child);

    node->totalCount = sum;
    return sum;
}

void CategoryProperties::collectAll(CategoryNode* node, QList<CategoryNode*>& out)
{
    out.append(node);

    const auto &children = node->children;   // prevent detach

    for (auto* child : children)
        collectAll(child, out);
}

void CategoryProperties::buildPieChart()
{
    auto* series = new QPieSeries();

    QList<CategoryNode*> allNodes;
    collectAll(root, allNodes);

    const auto &nodes = allNodes;
    for (auto* node : nodes) {
        if (node->directCount > 0)   // skip empty categories
            series->append(node->name, node->directCount);
    }

    connect(series, &QPieSeries::hovered,
            this, [this](QPieSlice* slice, bool state) {
                if (state) {
            QLocale locale;
                    QString text = QString("%1: %2 (%3%)")
                                       .arg(slice->label(),
                                            locale.toString(slice->value()),
                                            QString::number(slice->percentage() * 100.0, 'f', 1));

                    QToolTip::showText(QCursor::pos(), text, this->chartView);
                } else {
                    QToolTip::hideText();
                }
            });

    static const QVector<QColor> COLORS = {
        QColor(33, 150, 243),   // Blue
        QColor(255, 87, 34),    // Deep Orange
        QColor(156, 39, 176),   // Purple
        QColor(76, 175, 80),    // Green
        QColor(244, 67, 54),    // Red
        QColor(0, 188, 212),    // Cyan
        QColor(255, 193, 7),    // Amber
        QColor(63, 81, 181),    // Indigo
        QColor(121, 85, 72),    // Brown
        QColor(96, 125, 139),   // Blue Grey

        QColor(229, 57, 53),    // Red Dark
        QColor(25, 118, 210),   // Blue Dark
        QColor(56, 142, 60),    // Green Dark
        QColor(255, 160, 0),    // Amber Dark
        QColor(123, 31, 162),   // Purple Dark
        QColor(0, 151, 167),    // Cyan Dark
        QColor(230, 74, 25),    // Deep Orange Dark
        QColor(93, 64, 55),     // Brown Dark
        QColor(48, 63, 159),    // Indigo Dark
        QColor(69, 90, 100),    // Blue Grey Dark

        QColor(255, 138, 128),  // Red Light
        QColor(144, 202, 249),  // Blue Light
        QColor(165, 214, 167),  // Green Light
        QColor(255, 224, 130),  // Amber Light
        QColor(206, 147, 216),  // Purple Light
        QColor(128, 222, 234),  // Cyan Light
        QColor(255, 171, 145),  // Deep Orange Light
        QColor(161, 136, 127),  // Brown Light
        QColor(159, 168, 218),  // Indigo Light
        QColor(176, 190, 197)   // Blue Grey Light
    };

    int colorIndex = 0;
    for (auto* slice : series->slices()) {
        slice->setBrush(COLORS[colorIndex % COLORS.size()]);
        slice->setLabelColor(Qt::black);
        colorIndex++;
    }

    auto* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Passwords by Category");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);

    chartView->setChart(chart);
}

void CategoryProperties::buildTable()
{
    QList<CategoryNode*> allNodes;
    collectAll(root, allNodes);

    tableWidget->clearContents();
    tableWidget->setRowCount(allNodes.size() + 1);

    int row = 0;
    int total = 0;

    // Prevent detach — clazy fix
    const auto &nodes = allNodes;

    for (auto* node : nodes) {
        auto* nameItem = new QTableWidgetItem(node->name);
        auto* countItem = new QTableWidgetItem(QLocale().toString(node->directCount));

        countItem->setData(Qt::UserRole, node->directCount);

        tableWidget->setItem(row, 0, nameItem);
        tableWidget->setItem(row, 1, countItem);

        total += node->directCount;
        row++;
    }

    // Total row
    auto* totalLabelItem = new QTableWidgetItem("Total");
    QLocale locale = QLocale::system();
    auto* totalValueItem = new QTableWidgetItem(locale.toString(total));
    totalValueItem->setData(Qt::UserRole, total);

    QFont boldFont = tableWidget->font();
    boldFont.setBold(true);
    totalLabelItem->setFont(boldFont);
    totalValueItem->setFont(boldFont);

    tableWidget->setItem(row, 0, totalLabelItem);
    tableWidget->setItem(row, 1, totalValueItem);

    tableWidget->resizeColumnsToContents();

    // Show about 4 rows visibly
    adjustTableHeightForVisibleRows(4);
}

void CategoryProperties::adjustTableHeightForVisibleRows(int visibleRows)
{
    visibleRows = qMax(1, visibleRows);

    int rowCount = tableWidget->rowCount();
    int rowsToShow = qMin(visibleRows, rowCount);

    int headerHeight = tableWidget->horizontalHeader()->height();
    int rowHeight = tableWidget->verticalHeader()->defaultSectionSize();
    int frame = 2 * tableWidget->frameWidth();

    int totalHeight = headerHeight + rowHeight * rowsToShow + frame;

    tableWidget->setMinimumHeight(totalHeight);
    tableWidget->setMaximumHeight(totalHeight);
}

void CategoryProperties::onHelpRequested()
{
    QMessageBox::information(
        this,
        tr("Category Properties Help"),
        tr("This dialog shows a pie chart and table of passwords by category.")
        );
}
