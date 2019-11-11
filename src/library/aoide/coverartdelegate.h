#pragma once

#include <QPointer>
#include <QTableView>

#include "library/tableitemdelegate.h"

namespace mixxx {

namespace aoide {

class TrackTableModel;

class CoverArtDelegate : public TableItemDelegate {
    Q_OBJECT
  public:
    explicit CoverArtDelegate(QTableView* pTableView);
    ~CoverArtDelegate() override = default;

    void paintItem(
            QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

  private:
    QPointer<TrackTableModel> m_tableModel;
};

} // namespace aoide

} // namespace mixxx
