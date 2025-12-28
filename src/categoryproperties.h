#ifndef CATEGORYPROPERTIES_H
#define CATEGORYPROPERTIES_H

#include <QDialog>
#include <QMap>
#include <QList>
#include <QSqlDatabase>
#include <QChartView>

QT_BEGIN_NAMESPACE
class QPieSeries;
QT_END_NAMESPACE

struct CategoryNode {
    int id = 0;
    int parentId = 0;
    QString name;
    int directCount = 0;      // rows directly in this category
    int totalCount = 0;       // not needed for chart, but kept if useful later
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
    void collectAll(CategoryNode* node, QList<CategoryNode*>& out);

    void buildPieChart();

private:
    int selectedId = 0;
    CategoryNode* root = nullptr;

    QMap<int, CategoryNode*> nodes;
    QSqlDatabase db;

    QChartView* chartView = nullptr;
};

#endif // CATEGORYPROPERTIES_H
