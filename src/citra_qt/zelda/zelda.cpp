#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <fmt/format.h>

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QDialog>
#include <QEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

#include "citra_qt/zelda/game_actor.h"
#include "citra_qt/zelda/game_allocator.h"
#include "citra_qt/zelda/game_common_data.h"
#include "citra_qt/zelda/game_player.h"
#include "citra_qt/zelda/game_types.h"
#include "citra_qt/zelda/search_bar.h"
#include "citra_qt/zelda/zelda.h"
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/memory.h"

namespace zelda {

namespace {

bool IsActor(const game::AllocatorBlock& block) {
    return !block.IsFree() && block.name.addr == 0x00643A90;
}

Ptr<game::Actor> GetActorFromBlock(Ptr<game::AllocatorBlock> block) {
    if (!IsActor(*block))
        return nullptr;
    return Ptr<game::Actor>{u32(block.addr + sizeof(game::AllocatorBlock))};
}

const char* GetActorName(const game::Actor& actor) {
    static const std::unordered_map<game::Id, const char*> names = {
        {game::Id::Player, "Player"},
        {game::Id::Bomb, "Bomb"},
        {game::Id::Arrow, "Arrow"},
        {game::Id::Torch, "Torch"},
        {game::Id::Pot, "Pot"},
        {game::Id::ClearTag, "Bomb smoke (ClearTag)"},
        {game::Id::ObjRailLift, "Moving platform (Deku)"},
        {game::Id::DekuGuard_G, "Deku guard (Gardens)"},
        {game::Id::ObjOwlStatue, "Owl statue"},
    };
    const auto it = names.find(actor.id);
    if (it == names.end())
        return nullptr;
    return it->second;
}

u32 PredictOwlAddress(game::Allocator& allocator) {
    // Allocations that are performed when loading the central Deku Palace room

    const auto alloc = [&](u32 size) { return allocator.Alloc(size, nullptr); };

    auto x = alloc(156);
    allocator.Free(x);
    alloc(76);
    alloc(516);
    alloc(1436);
    alloc(516);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(2228);
    alloc(1436);
    alloc(2228);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1224);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(536);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(3672);
    alloc(1436);
    alloc(720);
    alloc(1436);
    alloc(720);
    alloc(1436);
    alloc(512);
    alloc(1436);
    alloc(3028);
    alloc(1436);
    alloc(524);
    alloc(1436);
    alloc(768);
    alloc(1436);
    alloc(44);
    alloc(516);
    alloc(44);
    auto owl = alloc(656);
    alloc(1436);
    alloc(608);
    alloc(1436);

    return owl.addr;
}

template <typename... Args>
QString Format(const char* format, const Args&... args) {
    return QString::fromStdString(fmt::format(format, args...));
}

class ZeldaDialog;

struct ZeldaInfo {
    using Blocks = std::vector<std::pair<Ptr<game::AllocatorBlock>, game::AllocatorBlock>>;

    u32 HeapSize() const {
        return heap_end - heap_start;
    }

    int FindContainingHeapIdx(VAddr addr) {
        // Because there are many allocations, it's worth doing a binary search instead of a
        // naïve linear search.
        int a = 0;
        int b = blocks.size() - 1;
        while (a <= b) {
            const auto m = (a + b) / 2;
            const auto block_begin = blocks[m].first.addr;
            const auto block_end = block_begin + std::abs(blocks[m].second.size);

            if (block_begin <= addr && addr < block_end)
                return m;

            if (addr < block_begin)
                b = m - 1;
            else
                a = m + 1;
        }
        return -1;
    }

    constexpr auto Fields() const {
        return std::tie(heap_start, heap_end, blocks);
    }

    bool operator==(const ZeldaInfo& rhs) const {
        return Fields() == rhs.Fields();
    }

    bool operator!=(const ZeldaInfo& rhs) const {
        return !(*this == rhs);
    }

    u32 heap_start = 0;
    u32 heap_end = 1;
    Blocks blocks;
    u32 total_free_size = 0;
    Ptr<game::Player> player_actor = nullptr;
    Ptr<game::Actor> target_actor = nullptr;

    Ptr<game::Actor> player_attached_actor = nullptr;

    u32 predicted_owl_addr = 0;
};

class InfoHolder {
public:
    void SetInfo(std::shared_ptr<ZeldaInfo> info) {
        m_info = std::move(info);
    }

protected:
    std::shared_ptr<ZeldaInfo> m_info = std::make_unique<ZeldaInfo>();
};

constexpr const char* HeapTableColumns[] = {"Block", "Address", "Size",
                                            "State", "Ticks",   "Description"};

enum class HeapTableColumn {
    Block = 0,
    Address,
    Size,
    State,
    Ticks,
    Description,
};
static_assert(int(HeapTableColumn::Description) + 1 == std::size(HeapTableColumns));

class HeapTableModel : public QAbstractTableModel, public InfoHolder {
public:
    explicit HeapTableModel(ZeldaDialog* parent);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void BeginReset() {
        beginResetModel();
    }

    void EndReset() {
        endResetModel();
    }

private:
    ZeldaDialog* m_parent;
};

class HeapTableFilterModel : public QSortFilterProxyModel {
public:
    explicit HeapTableFilterModel(ZeldaDialog* parent);
    void InvalidateFilter() {
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
    ZeldaDialog* m_parent;
};

class HeapViewWidget : public QOpenGLWidget, public InfoHolder {
public:
    explicit HeapViewWidget(ZeldaDialog* parent);
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent*) override;

private:
    ZeldaDialog* m_parent;
};

class ZeldaDialog : public QDialog {
public:
    explicit ZeldaDialog(QWidget* parent = nullptr) : QDialog(parent, Qt::CustomizeWindowHint) {
        resize(1900, 370);
        setWindowTitle(QStringLiteral("MM3D Heap Viewer"));
        setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
        setWindowFlag(Qt::WindowMaximizeButtonHint, true);
        setWindowFlag(Qt::WindowStaysOnTopHint, false);

        m_heap_view = new HeapViewWidget(this);
        m_heap_view->setFixedHeight(30);

        m_heap_label = new QLabel(QStringLiteral("-"), this);
        m_free_heap_label = new QLabel(QStringLiteral("-"), this);

        m_heap_table_model = new HeapTableModel(this);
        m_heap_table_proxy_model = new HeapTableFilterModel(this);
        m_heap_table_proxy_model->setSourceModel(m_heap_table_model);
        m_heap_table_proxy_model->setFilterKeyColumn(-1);
        m_heap_table_proxy_model->setSortRole(Qt::UserRole);

        m_keep_selection_cbox = new QCheckBox(QStringLiteral("Keep selection"));
        m_show_actors_cbox = new QCheckBox(QStringLiteral("Show actors"));
        m_show_layouts_cbox = new QCheckBox(QStringLiteral("Show layouts"));
        m_show_free_cbox = new QCheckBox(QStringLiteral("Show free blocks"));
        m_show_other_cbox = new QCheckBox(QStringLiteral("Show others"));
        m_restrict_to_range_cbox = new QCheckBox(QStringLiteral("Restrict to range:"));
        m_range_start_ledit = new QLineEdit;
        m_range_end_ledit = new QLineEdit;
        m_cursor_label = new QLabel(QStringLiteral("-"), this);
        m_pause_time_cbox = new QCheckBox(QStringLiteral("Pause time"));
        auto* dump_btn = new QPushButton(QStringLiteral("Dump heap"));
        auto* time_6pm_btn = new QPushButton(QStringLiteral("6PM"));

        m_heap_table_view = new QTableView(this);
        m_heap_table_view->setModel(m_heap_table_proxy_model);
        m_heap_table_view->verticalHeader()->hide();
        m_heap_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_heap_table_view->setSelectionMode(QAbstractItemView::SingleSelection);
        m_heap_table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_heap_table_view->horizontalHeader()->setStretchLastSection(true);
        m_heap_table_view->setSortingEnabled(true);
        m_heap_table_view->sortByColumn(int(HeapTableColumn::Address),
                                        Qt::SortOrder::AscendingOrder);

        auto* heap_table_search = new SearchBar(this);
        heap_table_search->AddFindShortcut(this);
        heap_table_search->ConnectToFilterModel(m_heap_table_proxy_model);
        heap_table_search->hide();

        m_connected_actor_info_label = new QLabel(this);
        m_target_actor_info_label = new QLabel(this);

        auto* label_grid = new QGridLayout;
        label_grid->addWidget(m_heap_label, 0, 0);
        label_grid->addWidget(m_free_heap_label, 1, 0);
        label_grid->addWidget(m_connected_actor_info_label, 0, 1);
        label_grid->addWidget(m_target_actor_info_label, 1, 1);

        // Set up layouts

        auto* options = new QHBoxLayout;
        options->addWidget(m_keep_selection_cbox);
        options->addWidget(m_show_actors_cbox);
        options->addWidget(m_show_layouts_cbox);
        options->addWidget(m_show_free_cbox);
        options->addWidget(m_show_other_cbox);
        options->addWidget(m_restrict_to_range_cbox);
        options->addWidget(m_range_start_ledit);
        options->addWidget(new QLabel(QStringLiteral("-")));
        options->addWidget(m_range_end_ledit);
        options->addStretch();
        options->addWidget(m_cursor_label);
        options->addWidget(m_pause_time_cbox);
        options->addWidget(time_6pm_btn);
        options->addWidget(dump_btn);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_heap_view);
        layout->addLayout(label_grid);
        layout->addLayout(options);
        layout->addWidget(m_heap_table_view, 9);
        layout->addWidget(heap_table_search);
        setLayout(layout);

        connect(m_heap_table_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &ZeldaDialog::OnHeapTableSelectionChanged);

        connect(dump_btn, &QPushButton::pressed, this, [this] {
            if (!Core::System::GetInstance().IsPoweredOn())
                return;

            const QString path =
                QFileDialog::getSaveFileName(this, tr("Save File"), {}, tr("Binary files (*.bin)"));
            if (path.isEmpty())
                return;
            FileUtil::IOFile file(path.toStdString(), "wb");
            auto* heap = Core::System::GetInstance().Memory().GetPointer(m_info->heap_start);
            file.WriteBytes(heap, m_info->HeapSize());

            FileUtil::IOFile file2(path.toStdString() + ".allocator", "wb");
            file2.WriteBytes(&game::Allocator::Instance(), sizeof(game::Allocator));
        });

        connect(time_6pm_btn, &QPushButton::pressed, [] {
            if (Core::System::GetInstance().IsPoweredOn())
                game::GetCommonData()->save.time = game::HhmmToTime(18, 0);
        });

        InitFilterControls();
    }

    // Called from the core. Do not touch widgets here.
    void Update() {
        if (!Core::System::GetInstance().IsPoweredOn())
            return;

        auto& allocator = game::Allocator::Instance();

        auto info = std::make_unique<ZeldaInfo>();
        info->heap_start = allocator.root_block.addr;
        info->heap_end = allocator.root_block_end.addr;

        info->blocks.reserve(allocator.block_count);
        const Ptr<game::AllocatorBlock> root_block = allocator.root_block;
        Ptr<game::AllocatorBlock> block = root_block;
        do {
            if (block->size > 0)
                info->total_free_size += block->size;

            Ptr<game::Actor> actor = GetActorFromBlock(block);
            if (actor) {
                if (actor->id == game::Id::Player)
                    info->player_actor = actor.Cast<game::Player>();
                else if (actor->id == game::Id::ObjOwlStatue)
                    info->target_actor = actor;
            }

            info->blocks.emplace_back(block, *block);
            block = block->next;
        } while (block != root_block);

        if (info->player_actor) {
            info->player_attached_actor = info->player_actor->attached_actor;

            // Ensure that we do not modify allocator state while an allocation or deallocation
            // is being performed.
            // Otherwise, the internal structures could be in an inconsistent state.

            if (allocator.crit_section->lock_count == 0) {
                game::Allocator backup = allocator;

                info->predicted_owl_addr = PredictOwlAddress(allocator);

                // Restore block and allocator state.
                for (const auto& [addr, block] : info->blocks) {
                    std::memcpy(addr.get(), &block, sizeof(block));
                }
                allocator = backup;
            }
        }

        // Update widgets on the proper thread.
        QMetaObject::invokeMethod(
            this, [this, info = std::move(info)]() mutable { UpdateUi(std::move(info)); });
    }

    void OnHeapViewReleaseEvent(QMouseEvent* e) {
        const auto pt = e->localPos();
        if (m_info->heap_start == 0)
            return;

        const int idx = m_info->FindContainingHeapIdx(TranslateMousePosToAddr(*m_info, pt.x()));
        if (idx == -1)
            return;

        m_heap_table_view->selectRow(SourceToProxyIdx(idx));
    }

    void OnHeapViewHoverEvent(QHoverEvent* e) {
        const auto pt = e->posF();
        if (pt.x() == -1 || m_info->heap_start == 0) {
            m_cursor_label->setText(QStringLiteral("-"));
            return;
        }
        m_cursor_label->setText(QStringLiteral("0x%1").arg(TranslateMousePosToAddr(*m_info, pt.x()),
                                                           8, 16, QLatin1Char('0')));
    }

    u32 TranslateMousePosToAddr(const ZeldaInfo& info, qreal x) const {
        return u32(info.heap_start + info.HeapSize() * x / m_heap_view->width());
    }

    std::optional<QModelIndex> GetSelectedIdx() const {
        auto smodel = m_heap_table_view->selectionModel();
        if (!smodel->hasSelection())
            return {};
        return m_heap_table_proxy_model->mapToSource(smodel->selectedRows()[0]);
    }

    bool ShouldShowActors() const {
        return m_show_actors_cbox->isChecked();
    }

    bool ShouldShowLayouts() const {
        return m_show_layouts_cbox->isChecked();
    }

    bool ShouldShowFree() const {
        return m_show_free_cbox->isChecked();
    }

    bool ShouldShowOthers() const {
        return m_show_other_cbox->isChecked();
    }

    std::pair<u32, u32> GetFilterRange() const {
        if (!m_restrict_to_range_cbox->isChecked())
            return {0, 0xFFFFFFFF};
        return {m_filter_range_start, m_filter_range_end};
    }

    u32 m_selected_addr = 0;
    u32 m_selected_size = 0;

private:
    void InitFilterControls() {
        for (auto* cbox :
             {m_show_actors_cbox, m_show_layouts_cbox, m_show_free_cbox, m_show_other_cbox}) {
            cbox->setChecked(true);
            connect(cbox, &QCheckBox::stateChanged, this,
                    [this](int state) { m_heap_table_proxy_model->InvalidateFilter(); });
        }

        const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

        for (auto* ledit : {m_range_start_ledit, m_range_end_ledit}) {
            ledit->setClearButtonEnabled(true);
            ledit->setMaximumWidth(ledit->width() / 4);
            ledit->setFont(fixed_font);
        }

        m_range_end_ledit->setText(QStringLiteral("0xFFFFFFFF"));

        connect(m_range_start_ledit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_filter_range_start = text.toULong(nullptr, 16);
            m_heap_table_proxy_model->InvalidateFilter();
        });
        connect(m_range_end_ledit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_filter_range_end = text.toULong(nullptr, 16);
            m_heap_table_proxy_model->InvalidateFilter();
        });

        const auto restrict_toggled = [this](int state) {
            m_heap_table_proxy_model->InvalidateFilter();
            m_range_start_ledit->setEnabled(state == Qt::CheckState::Checked);
            m_range_end_ledit->setEnabled(state == Qt::CheckState::Checked);
        };
        restrict_toggled(m_restrict_to_range_cbox->checkState());
        connect(m_restrict_to_range_cbox, &QCheckBox::stateChanged, this, restrict_toggled);
    }

    void UpdateUi(std::unique_ptr<ZeldaInfo> info) {
        if (!Core::System::GetInstance().IsPoweredOn())
            return;

        static std::optional<u16> s_previous_time;
        if (m_pause_time_cbox->isChecked() && s_previous_time) {
            game::GetCommonData()->save.time = *s_previous_time;
        }
        s_previous_time = game::GetCommonData()->save.time;

        auto& allocator = game::Allocator::Instance();

        const bool heap_info_changed = *m_info != *info;
        if (heap_info_changed) {
            const u32 selected_addr = m_selected_addr;

            m_heap_table_model->BeginReset();

            m_info = std::move(info);
            m_heap_table_model->SetInfo(m_info);
            m_heap_view->SetInfo(m_info);

            m_heap_table_model->EndReset();

            m_heap_label->setText(Format("Root block: 0x{:08x} → 0x{:08x} ({} bytes) - {} blocks",
                                         allocator.root_block.addr, allocator.root_block_end.addr,
                                         allocator.size, allocator.block_count));

            if (selected_addr != 0 && m_keep_selection_cbox->isChecked()) {
                const int new_selected_idx = m_info->FindContainingHeapIdx(selected_addr);
                if (new_selected_idx != -1)
                    m_heap_table_view->selectRow(SourceToProxyIdx(new_selected_idx));
            }
        }

        m_free_heap_label->setText(Format("Free size: {} bytes / {} bytes ({:.3f}% free)",
                                          m_info->total_free_size, allocator.size,
                                          100.0f * m_info->total_free_size / allocator.size));

        m_heap_view->update();

        if (m_info->player_actor) {
            const Ptr<game::Player> player = m_info->player_actor;
            m_connected_actor_info_label->setText(Format(
                "Player: {:.3f} {:.3f} {:.3f} | grabbable_actor={:08x} attached_actor={:08x}",
                player->pos.pos.x, player->pos.pos.y, player->pos.pos.z,
                player->grabbable_actor.addr, player->attached_actor.addr));
        } else {
            m_connected_actor_info_label->clear();
        }

        m_target_actor_info_label->setDisabled(false);
        if (m_info->target_actor) {
            const Ptr<game::Actor> target = m_info->target_actor;
            m_target_actor_info_label->setText(
                Format("Statue: {:08x} | params={:04x}", target.addr, target->params));
        } else {
            if (m_info->predicted_owl_addr != 0) {
                m_target_actor_info_label->setText(
                    Format("Statue: {:08x} (predicted load address)", m_info->predicted_owl_addr));
            } else {
                m_target_actor_info_label->setDisabled(true);
            }
        }
    }

    void OnHeapTableSelectionChanged(const QItemSelection& selected,
                                     const QItemSelection& deselected) {
        if (selected.indexes().size() != std::size(HeapTableColumns)) {
            m_selected_addr = 0;
            m_selected_size = 0;
            return;
        }
        const auto idx = GetSelectedIdx().value();
        const auto& [addr, block] = m_info->blocks[idx.row()];
        m_selected_addr = addr.addr;
        m_selected_size = std::abs(block.size);
        m_heap_view->update();
    }

    int SourceToProxyIdx(int idx) const {
        return m_heap_table_proxy_model->mapFromSource(m_heap_table_model->index(idx, 0)).row();
    }

    // Only for use on the UI thread.
    std::shared_ptr<ZeldaInfo> m_info = std::make_unique<ZeldaInfo>();

    QTableView* m_heap_table_view = nullptr;
    HeapTableModel* m_heap_table_model = nullptr;
    HeapTableFilterModel* m_heap_table_proxy_model = nullptr;

    HeapViewWidget* m_heap_view = nullptr;
    QLabel* m_heap_label = nullptr;
    QLabel* m_free_heap_label = nullptr;

    QCheckBox* m_keep_selection_cbox = nullptr;
    QCheckBox* m_show_actors_cbox = nullptr;
    QCheckBox* m_show_layouts_cbox = nullptr;
    QCheckBox* m_show_free_cbox = nullptr;
    QCheckBox* m_show_other_cbox = nullptr;
    QCheckBox* m_restrict_to_range_cbox = nullptr;
    QLineEdit* m_range_start_ledit = nullptr;
    QLineEdit* m_range_end_ledit = nullptr;
    QLabel* m_cursor_label = nullptr;
    QCheckBox* m_pause_time_cbox = nullptr;

    QLabel* m_connected_actor_info_label = nullptr;
    QLabel* m_target_actor_info_label = nullptr;

    u32 m_filter_range_start = 0;
    u32 m_filter_range_end = 0xFFFFFFFF;
};

HeapTableModel::HeapTableModel(ZeldaDialog* parent)
    : QAbstractTableModel(parent), m_parent(parent) {}

int HeapTableModel::rowCount(const QModelIndex& parent) const {
    return m_info->blocks.size();
}

int HeapTableModel::columnCount(const QModelIndex& parent) const {
    return std::size(HeapTableColumns);
}

QVariant HeapTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Orientation::Horizontal)
        return {};
    if (size_t(section) >= std::size(HeapTableColumns))
        return {};
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return {};
    return QString::fromLatin1(HeapTableColumns[section]);
}

enum HeapTableModelUserRole {
    HeapTableModelUserRole_Data = Qt::UserRole,
    HeapTableModelUserRole_IsActor,
    HeapTableModelUserRole_IsFree,
    HeapTableModelUserRole_IsInFilterRange,
};

QVariant HeapTableModel::data(const QModelIndex& index, int role) const {
    const auto& info = m_info;
    if (size_t(index.row()) >= info->blocks.size())
        return {};

    const auto& [addr, block] = info->blocks[index.row()];

    switch (role) {
    case HeapTableModelUserRole_IsActor:
        return IsActor(block);
    case HeapTableModelUserRole_IsFree:
        return block.IsFree() ||
               (block.flags & game::AllocatorBlock::Flag_IsRefCounted && block.ref_count == 0);
    case HeapTableModelUserRole_IsInFilterRange: {
        const auto [range_start, range_end] = m_parent->GetFilterRange();
        const u32 block_end_addr = addr.addr + std::abs(block.size);
        return (range_start <= addr.addr && addr.addr <= range_end) ||
               (range_start <= block_end_addr && block_end_addr <= range_end);
    }
    default:
        break;
    }

    switch (HeapTableColumn(index.column())) {
    case HeapTableColumn::Block:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(addr.addr, 8, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return addr.addr;
        default:
            return {};
        }
    case HeapTableColumn::Address:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(addr.addr + sizeof(game::AllocatorBlock), 8, 16,
                                              QLatin1Char('0'));
        case Qt::UserRole:
            return addr.addr + u32(sizeof(game::AllocatorBlock));
        default:
            return {};
        }
    case HeapTableColumn::Size:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(std::abs(block.size), 8, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return std::abs(block.size);
        default:
            return {};
        }
    case HeapTableColumn::State:
        switch (role) {
        case Qt::DisplayRole:
            return block.IsFree() ? QStringLiteral("Free") : QStringLiteral("Used");
        case Qt::UserRole:
            return block.IsFree() ? 1 : 0;
        default:
            return {};
        }
    case HeapTableColumn::Ticks:
        if (block.IsFree())
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("%1").arg(block.alloc_ticks, 16, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return quint64(block.alloc_ticks);
        default:
            return {};
        }
    case HeapTableColumn::Description:
        if (block.IsFree())
            return {};
        switch (role) {
        case Qt::DisplayRole:
            if (block.flags & game::AllocatorBlock::Flag_IsRefCounted)
                return QStringLiteral("(ref counted: %1)").arg(block.ref_count);
            [[fallthrough]];
        case Qt::UserRole:
            if (!block.name)
                return {};
            if (const auto actor = GetActorFromBlock(addr)) {
                if (const char* name = GetActorName(*actor)) {
                    return QStringLiteral("Actor: ID 0x%1 (%2)")
                        .arg(u16(actor->id), 4, 16, QLatin1Char('0'))
                        .arg(QString::fromLatin1(name));
                }
                return QStringLiteral("Actor: ID 0x%1")
                    .arg(u16(actor->id), 4, 16, QLatin1Char('0'));
            }
            return QString::fromLatin1(block.name);
        default:
            return {};
        }
    }

    return {};
}

HeapTableFilterModel::HeapTableFilterModel(ZeldaDialog* parent)
    : QSortFilterProxyModel(parent), m_parent(parent) {}

bool HeapTableFilterModel::filterAcceptsRow(int source_row,
                                            const QModelIndex& source_parent) const {
    const auto idx = sourceModel()->index(source_row, 0, source_parent);
    const bool is_actor = idx.data(HeapTableModelUserRole_IsActor).toBool();
    const bool is_layout = idx.siblingAtColumn(int(HeapTableColumn::Description))
                               .data(Qt::UserRole)
                               .toString()
                               .contains(QStringLiteral("Layout\\"));
    const bool is_free = idx.data(HeapTableModelUserRole_IsFree).toBool();
    const bool is_other = !is_actor && !is_layout && !is_free;

    if (!m_parent->ShouldShowActors() && is_actor)
        return false;
    if (!m_parent->ShouldShowLayouts() && is_layout)
        return false;
    if (!m_parent->ShouldShowFree() && is_free)
        return false;
    if (!m_parent->ShouldShowOthers() && is_other)
        return false;

    if (!idx.data(HeapTableModelUserRole_IsInFilterRange).toBool())
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

HeapViewWidget::HeapViewWidget(ZeldaDialog* parent) : QOpenGLWidget(parent), m_parent(parent) {
    setAttribute(Qt::WA_Hover);
}

void HeapViewWidget::paintEvent(QPaintEvent*) {
    const auto& info = m_info;
    QPainter painter(this);
    const int width = info->HeapSize();
    const int height = 1;
    painter.setWindow(QRect(0, 0, width, height));

    // Background
    painter.fillRect(0, 0, width, height, QColor(0x212121));

    for (const auto& [addr, block] : info->blocks) {
        QColor color;
        if (block.size <= 0) {
            color.setRgb(0xe83e33);
            if (block.ref_count == 0)
                color.setRgb(0);
        } else {
            color.setRgb(0x71e858);
        }
        painter.fillRect(addr.addr - info->heap_start, 0, std::abs(block.size), height, color);
    }

    if (m_info->target_actor) {
        painter.fillRect(m_info->target_actor.addr - info->heap_start, 0, 0xC000u, height,
                         QColor(0xff, 0xff, 0xff));
    }

    if (m_info->player_actor) {
        Ptr<game::Actor> attached_actor = m_info->player_attached_actor;
        if (attached_actor) {
            painter.fillRect(attached_actor.addr - info->heap_start, 0, 0xC000u, height,
                             QColor(0xff, 0xf2, 0x36));
        }
        if (attached_actor.addr - 0x10 == m_info->target_actor.addr) {
            painter.fillRect(attached_actor.addr - info->heap_start, 0, 0xC000u, height,
                             QColor(0x0D, 0x9A, 0xff));
        }
    }

    if (m_parent->m_selected_addr != 0) {
        painter.fillRect(m_parent->m_selected_addr - info->heap_start, 0,
                         std::max(m_parent->m_selected_size, 0xC000u), height,
                         QColor(0xff, 0xff, 0xff, 0x70));
    }
}

bool HeapViewWidget::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::MouseButtonRelease:
        m_parent->OnHeapViewReleaseEvent(static_cast<QMouseEvent*>(e));
        return true;
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        m_parent->OnHeapViewHoverEvent(static_cast<QHoverEvent*>(e));
        return true;
    default:
        break;
    }
    return QWidget::event(e);
}

static ZeldaDialog* s_dialog;

void OnFrame() {
    if (!s_dialog)
        return;
    s_dialog->Update();
}

} // namespace

void ShowWindow(QWidget* parent) {
    if (!s_dialog) {
        s_dialog = new ZeldaDialog(parent);
    }
    s_dialog->show();
    Core::System::GetInstance().SetFrameCallback(OnFrame);
}

} // namespace zelda
