#pragma once
#include <QLayout>
#include <QPushButton>
#include <QWidget>

// 自定义可折叠的分组
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

    void setContentLayout(QLayout* layout);

private slots:
    void toggle(bool checked);

private:
    void updateButtonText();

    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    bool m_collapsed;
    QString m_title;
};
