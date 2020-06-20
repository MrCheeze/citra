#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStyle>
#include "citra_qt/zelda/search_bar.h"

SearchBar::SearchBar(QWidget* parent) : QWidget(parent) {
    m_close_btn = new QPushButton;
    m_close_btn->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    m_box = new QLineEdit;
    m_box->setPlaceholderText(QStringLiteral("Filter..."));

    m_case_insensitive_cbox = new QCheckBox(QStringLiteral("Case insensitive"));
    m_case_insensitive_cbox->setChecked(true);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(m_close_btn);
    layout->addWidget(m_box);
    layout->addWidget(m_case_insensitive_cbox);
    layout->setStretch(1, 1);
    layout->setContentsMargins(5, 5, 5, 5);

    // Close
    auto* close_action = new QAction(this);
    close_action->setShortcut(QKeySequence::Cancel);
    addAction(close_action);
    connect(close_action, &QAction::triggered, this, &SearchBar::HideAndClear);
    connect(m_close_btn, &QPushButton::clicked, this, &SearchBar::HideAndClear);

    connect(m_box, &QLineEdit::textChanged, this, &SearchBar::TextChanged);
    connect(m_case_insensitive_cbox, &QCheckBox::stateChanged, this,
            [this](int state) { emit CaseInsensitiveChanged(state == Qt::Checked); });
}

void SearchBar::HideAndClear() {
    close();
    m_box->clear();
}

void SearchBar::ShowAndFocus() {
    show();
    m_box->setFocus();
}

void SearchBar::SetValue(const QString& string) {
    m_box->setText(string);
}

void SearchBar::AddFindShortcut(QWidget* widget) {
    auto* action = new QAction(widget);
    action->setShortcut(QKeySequence::Find);
    widget->addAction(action);
    connect(action, &QAction::triggered, this, &SearchBar::ShowAndFocus);
}

void SearchBar::ConnectToFilterModel(QSortFilterProxyModel* model) {
    connect(this, &SearchBar::TextChanged, model, &QSortFilterProxyModel::setFilterFixedString);
    connect(this, &SearchBar::CaseInsensitiveChanged, model, [model](bool insensitive) {
        model->setFilterCaseSensitivity(insensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);
    });
    emit CaseInsensitiveChanged(m_case_insensitive_cbox->isChecked());
}
