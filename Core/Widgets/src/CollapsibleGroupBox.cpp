#include "Widgets/CollapsibleGroupBox.h"
#include <QVBoxLayout>

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) :
    QWidget(parent),
    m_collapsed(true),
    m_title(title)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_toggleButton = new QPushButton();
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);
    m_toggleButton->setStyleSheet(
        "QPushButton { text-align: left; border: none; font-weight: bold; padding: 5px; }"
        "QPushButton:checked { background-color: #e0e0e0; }");
    mainLayout->addWidget(m_toggleButton);

    m_contentWidget = new QWidget(this);
    m_contentWidget->setVisible(false);
    mainLayout->addWidget(m_contentWidget);

    connect(m_toggleButton, &QPushButton::toggled, this, &CollapsibleGroupBox::toggle);

    updateButtonText();
}

void CollapsibleGroupBox::setContentLayout(QLayout* layout)
{
    delete m_contentWidget->layout();
    m_contentWidget->setLayout(layout);
}

void CollapsibleGroupBox::toggle(bool checked)
{
    m_collapsed = !checked;
    m_contentWidget->setVisible(checked);
    updateButtonText();
}

void CollapsibleGroupBox::updateButtonText()
{
    QString arrow = m_collapsed ? ">" : "v";
    m_toggleButton->setText(arrow + " " + m_title);
}
