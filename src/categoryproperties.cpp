#include "categoryproperties.h"
#include "dataobfuscator.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QChart>
#include <QChartView>
#include <QPieSeries>

CategoryProperties::CategoryProperties(int selectedCategoryId, QWidget* parent)
    : QDialog(parent), selectedId(selectedCategoryId)
{
    setWindowTitle("Category Breakdown");
    resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);

    chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);

    if (!loadDatabase()) {
        qWarning() << "Database failed to open";
        return;
    }

    loadCategories();
    loadApplicationCounts();

    if (!nodes.contains(selectedId)) {
        qWarning() << "Selected category ID not found:" << selectedId;
        return;
    }

    root = nodes[selectedId];

    // Optional: computeTotals(root); // not used for chart now
    buildPieChart();
}

CategoryProperties::~CategoryProperties()
{
    for (auto* node : nodes.values())
        delete node;
}

bool CategoryProperties::loadDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE", "categoryprops_conn");
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

    // Build tree relationships
    for (auto* node : nodes) {
        if (node->parentId != 0 && nodes.contains(node->parentId)) {
            nodes[node->parentId]->children.append(node);
        }
    }
}

void CategoryProperties::loadApplicationCounts()
{
    // Reset all direct counts
    for (auto* node : nodes)
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

    for (auto* child : node->children)
        sum += computeTotals(child);

    node->totalCount = sum;
    return sum;
}

void CategoryProperties::collectAll(CategoryNode* node, QList<CategoryNode*>& out)
{
    out.append(node);

    for (auto* child : node->children)
        collectAll(child, out);
}

void CategoryProperties::buildPieChart()
{
    auto* series = new QPieSeries();

    // Collect selected node + ALL descendants
    QList<CategoryNode*> allNodes;
    collectAll(root, allNodes);

    qDebug() << "Building chart for selectedId =" << selectedId;
    for (auto* node : allNodes) {
        qDebug() << " - id:" << node->id
                 << "name:" << node->name
                 << "directCount:" << node->directCount;

        // If you want to hide zero-count categories, uncomment:
        // if (node->directCount == 0)
        //     continue;

        series->append(node->name, node->directCount);
    }

    static const QVector<QColor> COLORS = {
        QColor(255, 179, 186), QColor(255, 223, 186), QColor(255, 255, 186),
        QColor(186, 255, 201), QColor(186, 225, 255), QColor(255, 204, 204),
        QColor(204, 255, 229), QColor(204, 229, 255), QColor(229, 204, 255),
        QColor(255, 204, 229), QColor(255, 240, 245), QColor(240, 255, 240),
        QColor(255, 250, 240), QColor(240, 248, 255), QColor(245, 245, 220),
        QColor(255, 228, 225), QColor(224, 255, 255), QColor(255, 239, 213),
        QColor(255, 228, 196), QColor(230, 230, 250)
    };

    int colorIndex = 0;
    for (auto* slice : series->slices()) {
        slice->setBrush(COLORS[colorIndex % COLORS.size()]);
        slice->setLabelColor(Qt::black);
        colorIndex++;
    }

    auto* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Applications by Category");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    chart->setBackgroundVisible(false);
    chart->setBackgroundBrush(Qt::NoBrush);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setPlotAreaBackgroundBrush(Qt::NoBrush);

    chartView->setStyleSheet("background: transparent");
    chartView->setAttribute(Qt::WA_TranslucentBackground);
    chartView->setBackgroundBrush(Qt::NoBrush);

    chartView->setChart(chart);
}

