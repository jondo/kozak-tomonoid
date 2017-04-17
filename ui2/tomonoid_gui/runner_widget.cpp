#include "runner_widget.h"
#include "ui_runner_widget.h"

runner_widget::runner_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::runner_widget)
{
    ui->setupUi(this);
}

runner_widget::~runner_widget()
{
    delete ui;
}
