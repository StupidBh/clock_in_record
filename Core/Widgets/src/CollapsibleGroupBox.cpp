#include "Widgets/CollapsibleGroupBox.h"

#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) :
    QWidget(parent),
    m_title(title)
{
    auto* const mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_toggleButton = new QPushButton();
    m_toggleButton->setObjectName(u"collapsibleToggleButton"_s);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);
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
    m_contentWidget->setVisible(checked);
    updateButtonText();
}

void CollapsibleGroupBox::updateButtonText() const
{
    const QString prefix = m_toggleButton->isChecked() ? u"v "_s : u"> "_s;
    m_toggleButton->setText(prefix + m_title);
}
