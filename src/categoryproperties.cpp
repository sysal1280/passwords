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

    // Set the selected category as the root of the subtree
    if (!nodes.contains(selectedId)) {
        qWarning() << "Selected category ID not found:" << selectedId;
        return;
    }

    root = nodes[selectedId];

    loadApplicationCounts();
    computeTotals(root);
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
    QString appKey = qApp->property("appKey").toString();

    QSqlQuery q(db);
    q.prepare("SELECT id, parent_id, text FROM categories ORDER BY id");

    if (!q.exec()) {
        qWarning() << "Category query failed:" << q.lastError();
        return;
    }

    // First pass: create all nodes
    while (q.next()) {
        int id = q.value(0).toInt();
        QString encryptedText = q.value(2).toString();
        QString decryptedText = DataObfuscator::deobfuscate(
            encryptedText,
            qApp->property("appKey").toByteArray()
            );

        nodes[id] = new CategoryNode{id, decryptedText};
    }

    // Second pass: attach children
    q.first();
    q.previous();

    while (q.next()) {
        int id = q.value(0).toInt();
        int parent = q.value(1).isNull() ? -1 : q.value(1).toInt();

        if (parent != -1 && nodes.contains(parent))
            nodes[parent]->children.append(nodes[id]);
    }
}

void CategoryProperties::loadApplicationCounts()
{
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

void CategoryProperties::addToSeries(CategoryNode* node, QPieSeries* series)
{
    if (node->totalCount > 0)
        series->append(node->name, node->totalCount);

    for (auto* child : node->children)
        addToSeries(child, series);
}

void CategoryProperties::buildPieChart()
{
    QPieSeries* series = new QPieSeries();
    addToSeries(root, series);

    // Apply distinct colors
    static const QVector<QColor> COLORS = {
        QColor("#e6194B"), QColor("#3cb44b"), QColor("#ffe119"),
        QColor("#4363d8"), QColor("#f58231"), QColor("#911eb4"),
        QColor("#46f0f0"), QColor("#f032e6"), QColor("#bcf60c"),
        QColor("#fabebe"), QColor("#008080"), QColor("#e6beff"),
        QColor("#9A6324"), QColor("#fffac8"), QColor("#800000"),
        QColor("#aaffc3"), QColor("#808000"), QColor("#ffd8b1"),
        QColor("#000075"), QColor("#808080")
    };

    int colorIndex = 0;
    for (auto slice : series->slices()) {
        slice->setBrush(COLORS[colorIndex % COLORS.size()]);
        slice->setLabelColor(Qt::black);
        colorIndex++;
    }

    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Applications by Category (Recursive)");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    chartView->setChart(chart);
}
