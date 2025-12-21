#ifndef RANDOMNOISEDIALOG_H
#define RANDOMNOISEDIALOG_H

#include <QString>
#include <QWidget>

namespace RandomNoiseDialog {

void showRandomNoiseGenerator(QWidget *parent = nullptr,
                              const QString &title = "Random Noise Generator");

}

#endif // RANDOMNOISEDIALOG_H
