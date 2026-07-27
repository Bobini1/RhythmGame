#include "gameplay_logic/BmsNotes.h"

#include "db/SqliteCppDb.h"
#include "support/Compress.h"

auto
gameplay_logic::BmsNotes::load(db::SqliteCppDb& db,
                               const support::Sha256& sha256)
  -> std::unique_ptr<BmsNotes>
{
    using namespace std::string_literals;
    auto statement = db.createStatement(
      "SELECT note_data FROM note_data WHERE sha256 = :sha256");
    statement.bind(":sha256", sha256);
    auto result = statement.executeAndGet<std::string>();
    if (!result.has_value()) {
        throw std::runtime_error{ "Failed to load note data" };
    }
    auto serializedData = QByteArray::fromStdString(*result);
    auto noteData = std::make_unique<BmsNotes>();
    support::decompress(serializedData, *noteData);
    return noteData;
}

auto
gameplay_logic::BmsNotes::save(db::SqliteCppDb& db,
                               const support::Sha256& sha256) const -> void
{
    auto serializedData = serialize();
    auto insertQuery =
      db.createStatement("INSERT OR REPLACE INTO note_data (sha256, note_data) "
                         "VALUES (:sha256, :note_data)");
    insertQuery.bind(":sha256", sha256);
    insertQuery.bind(
      ":note_data", serializedData.data(), serializedData.size());
    insertQuery.execute();
    insertQuery.reset();
}
