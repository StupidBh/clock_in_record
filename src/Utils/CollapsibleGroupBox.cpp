#include "CollapsibleGroupBox.h"
#include <QVBoxLayout>

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) :
    QWidget(parent),
    m_collapsed(true),
    m_title(title)
{
    m_toggleButton = new QPushButton();
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);
    m_toggleButton->setStyleSheet(
        "QPushButton { text-align: left; border: none; font-weight: bold; padding: 5px; }"
        "QPushButton:checked { background-color: #e0e0e0; }");

    m_contentWidget = new QWidget();
    m_contentWidget->setWindowFlags(Qt::Popup);
    m_contentWidget->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_toggleButton);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(m_toggleButton, &QPushButton::toggled, this, &CollapsibleGroupBox::toggle);

    updateButtonText();
}

void CollapsibleGroupBox::setContentLayout(QLayout* layout)
{
    m_contentWidget->setLayout(layout);
    auto pw = this->parentWidget();
    if (pw) {
        pw->resize(pw->size().width(), pw->sizeHint().height());
    }
}

QWidget* CollapsibleGroupBox::contentWidget() const
{
    return m_contentWidget;
}

void CollapsibleGroupBox::toggle(bool checked)
{
    m_collapsed = !checked;
    m_contentWidget->setVisible(checked);
    QPoint globalPos = m_toggleButton->mapToGlobal(QPoint(0, 0));
    int x = globalPos.x();
    int y = globalPos.y() + m_toggleButton->height(); // 显示在按钮下方
    m_contentWidget->move(x, y);
    updateButtonText();
}

void CollapsibleGroupBox::updateButtonText()
{
    QString arrow = m_collapsed ? ">" : "v";
    m_toggleButton->setText(arrow + " " + m_title);
}
