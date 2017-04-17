#ifndef RUNNER_WIDGET_H
#define RUNNER_WIDGET_H

#include <QWidget>

namespace Ui {
class runner_widget;
}

class runner_widget : public QWidget
{
    Q_OBJECT

public:
    explicit runner_widget(QWidget *parent = 0);
    ~runner_widget();

private:
    Ui::runner_widget *ui;
};

#endif // RUNNER_WIDGET_H
