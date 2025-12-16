// categorydialog.h
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialog>
#include <QObject>


class CategoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CategoryDialog(QWidget *parent = nullptr, int existingCount = 0);

    QString categoryName() const;
    bool isTopLevel() const;

private:
    QLineEdit *m_nameEdit;
    QCheckBox *m_topLevelCheck;
};
