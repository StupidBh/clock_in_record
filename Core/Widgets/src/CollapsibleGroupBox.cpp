#include "Widgets/CollapsibleGroupBox.h"

#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent) :
    QWidget(parent),
    m_title(title)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* const mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    m_toggleButton = new QToolButton();
    m_toggleButton->setObjectName(u"collapsibleToggleButton"_s);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(false);
    m_toggleButton->setText(m_title);
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_toggleButton->setAccessibleName(m_title);
    mainLayout->addWidget(m_toggleButton);

    m_contentWidget = new QWidget(this);
    m_contentWidget->setVisible(false);
    mainLayout->addWidget(m_contentWidget);

    connect(m_toggleButton, &QToolButton::toggled, this, &CollapsibleGroupBox::toggle);

    updateButtonState();
}

void CollapsibleGroupBox::setContentLayout(QLayout* layout)
{
    delete m_contentWidget->layout();
    m_contentWidget->setLayout(layout);
}

void CollapsibleGroupBox::toggle(bool checked)
{
    setSizePolicy(QSizePolicy::Preferred, checked ? QSizePolicy::Expanding : QSizePolicy::Fixed);
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->setAlignment(this, checked ? Qt::Alignment { } : Qt::AlignTop);
    }
    m_contentWidget->setVisible(checked);
    updateGeometry();
    updateButtonState();
}

void CollapsibleGroupBox::updateButtonState() const
{
    m_toggleButton->setArrowType(m_toggleButton->isChecked() ? Qt::DownArrow : Qt::RightArrow);
}
