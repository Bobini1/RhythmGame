//
// Created by PC on 31/12/2024.
//

#ifndef KEYCONFIGS_H
#define KEYCONFIGS_H

#include "db/SqliteCppDb.h"
#include "input/InputTranslator.h"

#include <QAbstractListModel>

namespace resource_managers {

class InputTranslators final : public QAbstractListModel
{
    Q_OBJECT

    /**
     * @brief All key configurations
     * Even those that are not used by any profile at the moment.
     */
    QList<QHash<input::Key, input::BmsKey>> keyConfigs;
    QList<input::InputTranslator*> translators;
    db::SqliteCppDb* db;
    input::GamepadManager* gamepadManager;

    auto createInputTranslator() -> input::InputTranslator*;
    void saveKeyConfigs() const;

  public:
    explicit InputTranslators(input::GamepadManager* gamepadManager,
                              db::SqliteCppDb* db,
                              QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto getInputTranslators() -> const QList<input::InputTranslator*>&;

    void onPlayerCountChanged(int n);
};

} // namespace resource_managers

#endif // KEYCONFIGS_H
