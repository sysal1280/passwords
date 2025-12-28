#ifndef CATEGORYPROPERTIES_H
#define CATEGORYPROPERTIES_H

#include <QDialog>
#include <QSqlDatabase>
#include <QMap>

class QChartView;
class QPieSeries;

struct CategoryNode {
    int id;
    QString name;
    QList<CategoryNode*> children;
    int directCount = 0;
    int totalCount = 0;
};

class CategoryProperties : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryProperties(int selectedCategoryId, QWidget* parent = nullptr);
    ~CategoryProperties();

private:
    int selectedId = 0;

    QSqlDatabase db;
    QMap<int, CategoryNode*> nodes;
    CategoryNode* root = nullptr;

    QChartView* chartView = nullptr;

    bool loadDatabase();
    void loadCategories();
    void loadApplicationCounts();
    int computeTotals(CategoryNode* node);
    void buildPieChart();
    void addToSeries(CategoryNode* node, QPieSeries* series);
};

#endif // CATEGORYPROPERTIES_H
