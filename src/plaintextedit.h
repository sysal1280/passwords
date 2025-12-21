#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QTextEdit>
#include <QMimeData>

class PlainTextEdit : public QTextEdit
{
public:
    using QTextEdit::QTextEdit;

protected:
    void insertFromMimeData(const QMimeData *source) override {
        this->insertPlainText(source->text());
    }
};

#endif // PLAINTEXTEDIT_H
