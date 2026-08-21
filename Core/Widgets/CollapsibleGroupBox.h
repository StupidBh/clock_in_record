#pragma once

#include <QString>
#include <QWidget>

class QLayout;
class QPushButton;

// 自定义可折叠的分组
class CollapsibleGroupBox final : public QWidget {
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

    void setContentLayout(QLayout* layout);

private slots:
    void toggle(bool checked);

private:
    void updateButtonText() const;

    QPushButton* m_toggleButton = nullptr;
    QWidget* m_contentWidget = nullptr;
    QString m_title;
};
