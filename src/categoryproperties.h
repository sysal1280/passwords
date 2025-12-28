#ifndef CATEGORYPROPERTIES_H
#define CATEGORYPROPERTIES_H

#include <QDialog>
#include <QMap>
#include <QList>
#include <QSqlDatabase>
#include <QChartView>
#include <QTableWidget>
#include <QDialogButtonBox>

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
