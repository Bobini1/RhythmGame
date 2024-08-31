//
// Created by bobini on 05.10.23.
//

#include "PreviewFilePathFetcher.h"

namespace qml_components {
PreviewFilePathFetcher::PreviewFilePathFetcher(db::SqliteCppDb* db,
                                               QObject* parent)
  : QObject(parent)
  , db(db)
{
}
auto
PreviewFilePathFetcher::getPreviewFilePath(const QString& directory) -> QString
{
    getPreviewFilePathStatement.reset();
    getPreviewFilePathStatement.bind(":directory", directory.toStdString());
    auto result = getPreviewFilePathStatement.executeAndGet<std::string>();
    if (!result) {
        return "";
    }
    return QString::fromStdString(*result);
}
} // namespace qml_components