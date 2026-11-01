#include "resource_managers/Vars.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QJSEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTranslator>

#include <memory>

namespace {
void
prepareOptionsApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    if (!QCoreApplication::instance()) {
        [[maybe_unused]] static auto* app = new QCoreApplication(argc, argv);
    }
    static const auto registration = qmlRegisterUncreatableMetaObject(
      resource_managers::select_sort_mode::staticMetaObject,
      "RhythmGameQml",
      1,
      0,
      "SelectSortMode",
      "Enum values only");
    (void)registration;
}

class SelectOptionsHarness
{
  public:
    QQmlEngine engine;

  private:
    std::unique_ptr<QObject> object;

  public:
    SelectOptionsHarness()
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import RhythmGameQml
            QtObject {
                id: root
                readonly property var sortOptions: StandardSelectOptions.sortOptions
                readonly property var replayOptions: StandardSelectOptions.replayOptions
                readonly property var customReplayOptions: root.replayOptions.map(option => ({
                    value: option.value,
                    text: option.value === 3 ? "My combo" : option.text
                })).reverse()
                readonly property string title: StandardSelectOptions.sortText(SelectSortMode.Title)
                readonly property string newest: StandardSelectOptions.replayText(0)
                readonly property string customCombo: root.customReplayOptions.find(option => option.value === 3).text
                readonly property string customNewest: root.customReplayOptions.find(option => option.value === 0).text
            }
        )",
                          QUrl("qrc:/SelectOptionsTest.qml"));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        object.reset(component.create());
        INFO(component.errorString().toStdString());
        REQUIRE(object);
        engine.globalObject().setProperty("h", engine.newQObject(object.get()));
    }

    void check(const QString& expression)
    {
        const auto result = engine.evaluate(expression);
        INFO(expression.toStdString());
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        CHECK(result.toBool());
    }
};

class SelectOptionsTranslator final : public QTranslator
{
  public:
    bool isEmpty() const override { return false; }

    QString translate(const char* context,
                      const char* sourceText,
                      const char*,
                      int) const override
    {
        if (qstrcmp(context, "StandardSelectOptions") == 0) {
            return QStringLiteral("Translated: ") +
                   QString::fromUtf8(sourceText);
        }
        return {};
    }
};
}

TEST_CASE("Themes can customize select labels without changing shared choices",
          "[qml][SelectOptions]")
{
    prepareOptionsApplication();
    SelectOptionsHarness h;
    h.check("h.customCombo === 'My combo' && h.customNewest === 'Newest'");
    h.check("h.replayOptions.find(o => o.value === 3).text === 'Best combo'");
    h.check(
      "JSON.stringify(h.customReplayOptions.map(o => o.value).reverse()) === "
      "JSON.stringify(h.replayOptions.map(o => o.value))");
    h.check("h.title === 'Title' && h.newest === 'Newest'");
}

TEST_CASE("Shared select labels and theme models follow language changes",
          "[qml][SelectOptions]")
{
    prepareOptionsApplication();
    SelectOptionsHarness h;
    SelectOptionsTranslator translator;
    REQUIRE(QCoreApplication::installTranslator(&translator));
    h.engine.retranslate();
    h.check(
      "h.title === 'Translated: Title' && h.newest === 'Translated: Newest'");
    h.check("h.sortOptions.every(o => o.text.startsWith('Translated: '))");
    h.check("h.replayOptions.every(o => o.text.startsWith('Translated: '))");
    h.check("h.customCombo === 'My combo' && h.customNewest === 'Translated: "
            "Newest'");
    REQUIRE(QCoreApplication::removeTranslator(&translator));
    h.engine.retranslate();
    h.check("h.title === 'Title' && h.newest === 'Newest'");
    h.check("h.customCombo === 'My combo' && h.customNewest === 'Newest'");
}
