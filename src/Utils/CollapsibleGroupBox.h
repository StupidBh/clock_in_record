#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLayout>

// �Զ�����۵��ķ���
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

    void setContentLayout(QLayout* layout);
    QWidget* contentWidget() const;

private slots:
    void toggle(bool checked);

private:
    void updateButtonText();

    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    bool m_collapsed;
    QString m_title;
};

#endif // COLLAPSIBLEGROUPBOX_H