#include "library/aoide/coverartdelegate.h"

#include <QPainter>

#include "library/aoide/tracktablemodel.h"

namespace mixxx {

namespace aoide {

CoverArtDelegate::CoverArtDelegate(QTableView* parent)
        : TableItemDelegate(parent),
          m_tableModel(parent ? dynamic_cast<TrackTableModel*>(parent->model()) : nullptr) {
    DEBUG_ASSERT(m_tableModel);
}

void CoverArtDelegate::paintItem(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    DEBUG_ASSERT(m_tableModel);
    DEBUG_ASSERT(painter);
    // TODO: Load cover art image in the background
    const auto backgroundColor =
            m_tableModel->item(index).body().mediaSource().artwork().backgroundColor();
    if (!backgroundColor.isValid()) {
        return;
    }
    painter->fillRect(option.rect, backgroundColor);
}

} // namespace aoide

} // namespace mixxx