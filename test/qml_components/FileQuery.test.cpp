#include <catch2/catch_test_macros.hpp>

#include "qml_components/FileQuery.h"

#include <QFile>
#include <QTemporaryDir>
#include <QUrl>

#include <optional>

namespace {

auto
writeBytes(const QString& path, const QByteArray& data) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(data) == data.size());
}

auto
textWith(QChar ch) -> QString
{
    return QString(ch);
}

class FolderLaunchProbe : public qml_components::FileQuery
{
  public:
    mutable std::optional<QUrl> openedUrl;

  protected:
    bool openUrl(const QUrl& url) const override
    {
        openedUrl = url;
        return true;
    }
};

} // namespace

TEST_CASE("FileQuery reads no-BOM text files as CP932", "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme.txt");
    writeBytes(path, QByteArray("\x87\x40", 2));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x2460)));
}

TEST_CASE("FileQuery honors UTF-8 BOM for text files", "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme-utf8.txt");
    writeBytes(path, QByteArray("\xEF\xBB\xBF\xE3\x81\x82", 6));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x3042)));
}

TEST_CASE("FileQuery honors UTF-16 BOM for text files", "[FileQuery][encoding]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath("readme-utf16le.txt");
    writeBytes(path, QByteArray("\xFF\xFE\x42\x30", 4));

    const auto text = qml_components::FileQuery().readTextFile(path);
    CHECK(text == textWith(QChar(0x3042)));
}

TEST_CASE("FileQuery lists selectable files with Unicode filenames",
          "[FileQuery][paths]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto filename =
      QStringLiteral("mascot-") + QChar(0x3042) + QStringLiteral(".png");
    writeBytes(tempDir.filePath(filename), QByteArray("x", 1));

    const auto files =
      qml_components::FileQuery().getSelectableFilesForDirectory(
        tempDir.path());
    CHECK(files.contains(filename));
}

TEST_CASE("FileQuery preserves hashes in local file URLs", "[FileQuery][paths]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto path = tempDir.filePath(QStringLiteral("song#mix"));
    const auto url = QUrl(qml_components::FileQuery().localFileUrl(path));

    CHECK(url.isLocalFile());
    CHECK(url.fragment().isEmpty());
    CHECK(url.toLocalFile() == path);
}

TEST_CASE("FileQuery opens folders through a typed local URL",
          "[FileQuery][paths]")
{
    const auto path = QStringLiteral("C:/Songs/song#mix");
    auto query = FolderLaunchProbe{};

    REQUIRE(query.openFolder(path));
    REQUIRE(query.openedUrl.has_value());
    CHECK(query.openedUrl->isLocalFile());
    CHECK_FALSE(query.openedUrl->hasFragment());
    CHECK_FALSE(query.openedUrl->hasQuery());
    CHECK(query.openedUrl->toLocalFile() == path);
}
