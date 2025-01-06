//
// Created by PC on 31/12/2024.
//

#include "InputTranslators.h"

#include "support/Compress.h"

#include <spdlog/spdlog.h>

namespace resource_managers {

auto
InputTranslators::createInputTranslator() -> input::InputTranslator*
{
    auto* const inputTranslator = new input::InputTranslator(this);
    connect(gamepadManager,
            &input::GamepadManager::axisMoved,
            inputTranslator,
            &input::InputTranslator::handleAxis);
    connect(gamepadManager,
            &input::GamepadManager::buttonPressed,
            inputTranslator,
            &input::InputTranslator::handlePress);
    connect(gamepadManager,
            &input::GamepadManager::buttonReleased,
            inputTranslator,
            &input::InputTranslator::handleRelease);
    return inputTranslator;
}
void
InputTranslators::saveKeyConfigs()
{
    auto statement =
      db->createStatement("INSERT OR REPLACE INTO properties (key, value) "
                          "VALUES ('key_config', ?)");
    const auto data = support::compress(keyConfigs);
    statement.bind(1, data.data(), data.size());
    statement.execute();
}

InputTranslators::InputTranslators(input::GamepadManager* gamepadManager,
                                   db::SqliteCppDb* db,
                                   QObject* parent)
  : QAbstractListModel(parent)
  , db(db)
  , gamepadManager(gamepadManager)
{
    auto statement = db->createStatement(
      "SELECT value FROM properties WHERE key = 'key_config'");
    if (const auto keyConfig = statement.executeAndGet<std::string>();
        keyConfig.has_value()) {
        const auto array = QByteArray::fromStdString(keyConfig.value());
        support::decompress(array, this->keyConfigs);
    }
}
auto
InputTranslators::rowCount(const QModelIndex& parent) const -> int
{
    return translators.size();
}
auto
InputTranslators::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(translators[index.row()]);
    }
    return {};
}
auto
InputTranslators::getInputTranslators() -> const QList<input::InputTranslator*>&
{
    return translators;
}
void
InputTranslators::onPlayerCountChanged(int n)
{
    if (translators.size() == n) {
        return;
    }
    if (keyConfigs.size() < n) {
        while (keyConfigs.size() < n) {
            keyConfigs.emplace_back();
        }
    }

    while (translators.size() < n) {
        beginInsertRows(QModelIndex(), translators.size(), translators.size());
        auto* translator = createInputTranslator();
        connect(translator,
                &input::InputTranslator::keyConfigModified,
                this,
                [this, translator]() {
                    const auto index = translators.indexOf(translator);
                    if (index == -1) {
                        return;
                    }
                    keyConfigs[index] = translator->getKeyConfigHash();
                    emit dataChanged(this->index(index), this->index(index));
                });
        translators.push_back(translator);
        endInsertRows();
    }
    while (translators.size() > n) {
        beginRemoveRows(
          QModelIndex(), translators.size() - 1, translators.size() - 1);
        auto* translator = translators.takeLast();
        translator->deleteLater();
        endRemoveRows();
    }
}
} // namespace resource_managers