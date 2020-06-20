#pragma once

#include <QWidget>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QString;
class QSortFilterProxyModel;

class SearchBar : public QWidget {
    Q_OBJECT
public:
    explicit SearchBar(QWidget* parent = nullptr);

    void HideAndClear();
    void ShowAndFocus();
    void SetValue(const QString& string);

    void AddFindShortcut(QWidget* widget);

    void ConnectToFilterModel(QSortFilterProxyModel* model);

signals:
    void TextChanged(const QString& text);
    void CaseInsensitiveChanged(bool);

private:
    QPushButton* m_close_btn;
    QLineEdit* m_box;
    QCheckBox* m_case_insensitive_cbox;
};
