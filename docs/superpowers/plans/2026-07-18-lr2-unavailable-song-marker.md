# LR2 Unavailable Song Marker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace every native-LR2 missing or Arena-unavailable song-title prefix with the compact, non-translatable marker × followed by one space.

**Architecture:** Lr2SelectContext owns the QML-facing unavailableSongPrefix and passes it through the select-model boundary. Lr2SelectItemModel mirrors the same value as a C++ default so direct model users and QML produce identical bar text, while Lr2SkinValueResolver consumes the context property for missing table and course titles. Translation catalogs lose the obsolete word-based messages.

**Tech Stack:** Qt 6 QML, Qt 6 C++, QAbstractListModel, Catch2, CMake presets, Qt Linguist

## Global Constraints

- The marker is exactly U+00D7 MULTIPLICATION SIGN followed by one ASCII space: × .
- The marker is intentionally not wrapped in qsTr and must not appear in translation catalogs.
- Native LR2 uses the marker for Arena-unavailable charts, locally missing table entries, and locally missing course stages.
- Beatoraja keeps unavailable body/title types without an Arena text prefix and keeps (no song) for missing course stages.
- Default-theme and other non-LR2 presentation remain unchanged.
- Preserve unrelated working-tree changes and stage only the files listed by each task.

---

### Task 1: Unify the native LR2 select-model marker

**Files:**
- Modify: RhythmGameQml/Lr2/Lr2SelectItemModel.h
- Modify: RhythmGameQml/Lr2/Lr2SelectItemModel.cpp
- Modify: RhythmGameQml/Lr2/Lr2SelectContext.qml
- Test: test/lr2_skin/Lr2SelectBarModel.test.cpp

**Interfaces:**
- Consumes: arena::ArenaAvailabilityIndex through setArenaAvailability(arena::ArenaAvailabilityIndex*)
- Produces: QString unavailableSongPrefix() const, void setUnavailableSongPrefix(const QString&), and unavailableSongPrefixChanged()
- Produces: QML readonly property string unavailableSongPrefix and model property unavailableSongPrefix

- [ ] **Step 1: Establish the focused baseline**

Run:

~~~powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R "LR2 select item model" --output-on-failure
~~~

Expected: the existing LR2 select-item-model tests pass before the regression cases are changed.

- [ ] **Step 2: Add red regression coverage for missing and Arena-unavailable rows**

In test/lr2_skin/Lr2SelectBarModel.test.cpp, add this helper in the anonymous namespace:

~~~cpp
QString
unavailableSongPrefix()
{
    return QString(QChar(0x00D7)) + QLatin1Char(' ');
}
~~~

In the existing test named "LR2 select item model reads raw table entry fields from lean row maps", stop feeding already-decorated displayText and title values. Keep the raw entry and the unrelated derived fields:

~~~cpp
    Lr2SelectItemModel source;
    source.setItems({
      QVariantMap{
        { QStringLiteral("rawItem"), QVariant::fromValue(entry) },
        { QStringLiteral("titleType"), 0 },
        { QStringLiteral("bodyType"), 4 },
        { QStringLiteral("lamp"), 2 },
        { QStringLiteral("scoreRank"), 6 },
        { QStringLiteral("labelMask"), 0 },
      },
    });
~~~

Replace its display and title expectations with:

~~~cpp
    REQUIRE(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() ==
            unavailableSongPrefix() + QStringLiteral("Song Sub"));
    REQUIRE(source.data(row, Lr2SelectItemModel::TitleRole).toString() ==
            unavailableSongPrefix() + QStringLiteral("Song"));

    source.setUseBeatorajaBarTextTypes(true);
    CHECK(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() ==
          QStringLiteral("Song Sub"));
    CHECK(source.data(row, Lr2SelectItemModel::TitleRole).toString() ==
          QStringLiteral("Song"));
~~~

Add a second test after it:

~~~cpp
TEST_CASE(
  "LR2 select item model prefixes Arena-unavailable charts with the shared marker",
  "[lr2][runtime][select]")
{
    arena::ArenaAvailabilityIndex availability;
    REQUIRE(availability.applyReset(1, QByteArray(32, '\x01')));

    Lr2SelectItemModel source;
    source.setArenaAvailability(&availability);
    source.setItems({
      QVariantMap{
        { QStringLiteral("type"), QStringLiteral("chart") },
        { QStringLiteral("title"), QStringLiteral("Song") },
        { QStringLiteral("subtitle"), QStringLiteral("Sub") },
        { QStringLiteral("sha256"),
          QString(64, QLatin1Char('0')) },
      },
    });

    const QModelIndex row = source.index(0, 0);
    REQUIRE(source.data(row, Lr2SelectItemModel::ArenaAvailabilityRole).toInt() ==
            static_cast<int>(
              arena::ArenaAvailabilityIndex::Availability::UnavailableToSome));
    CHECK(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() ==
          unavailableSongPrefix() + QStringLiteral("Song Sub"));
    CHECK(source.data(row, Lr2SelectItemModel::TitleRole).toString() ==
          unavailableSongPrefix() + QStringLiteral("Song"));

    source.setUseBeatorajaBarTextTypes(true);
    CHECK(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() ==
          QStringLiteral("Song Sub"));
    CHECK(source.data(row, Lr2SelectItemModel::TitleRole).toString() ==
          QStringLiteral("Song"));
}
~~~

Add an explicit QByteArray include beside the existing Qt includes:

~~~cpp
#include <QByteArray>
~~~

- [ ] **Step 3: Build and run the red tests**

Run:

~~~powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R "LR2 select item model" --output-on-failure
~~~

Expected: the missing-entry case reports (missing) instead of ×, and the Arena-unavailable case omits the marker because the current C++ default is empty.

- [ ] **Step 4: Rename and generalize the model property**

In RhythmGameQml/Lr2/Lr2SelectItemModel.h, replace the Arena-specific property, methods, signal, and member with:

~~~cpp
    Q_PROPERTY(QString unavailableSongPrefix READ unavailableSongPrefix WRITE setUnavailableSongPrefix NOTIFY unavailableSongPrefixChanged)

    QString unavailableSongPrefix() const;
    void setUnavailableSongPrefix(const QString& prefix);

    void unavailableSongPrefixChanged();

    QString m_unavailableSongPrefix{ QStringLiteral("\u00D7 ") };
~~~

Keep each declaration in the same section occupied by its previous arenaUnavailablePrefix counterpart.

In RhythmGameQml/Lr2/Lr2SelectItemModel.cpp, replace the getter and setter with:

~~~cpp
QString Lr2SelectItemModel::unavailableSongPrefix() const {
    return m_unavailableSongPrefix;
}

void Lr2SelectItemModel::setUnavailableSongPrefix(const QString& prefix) {
    if (m_unavailableSongPrefix == prefix) {
        return;
    }
    m_unavailableSongPrefix = prefix;
    emit unavailableSongPrefixChanged();
    refreshDerivedItems();
}
~~~

Use m_unavailableSongPrefix instead of the literal (missing) in displayTextForItem() and titleForItem(). Use the same member instead of m_arenaUnavailablePrefix in effectiveDisplayText() and effectiveTitle(). Leave the Beatoraja guards unchanged.

- [ ] **Step 5: Expose the same marker through Lr2SelectContext**

In RhythmGameQml/Lr2/Lr2SelectContext.qml, replace the translated Arena property with:

~~~qml
    readonly property string unavailableSongPrefix: "\u00d7 "
~~~

Update the Lr2SelectItemModel binding:

~~~qml
        unavailableSongPrefix: root.unavailableSongPrefix
~~~

Make entryTitlePrefix() return root.unavailableSongPrefix for both native-LR2 cases:

~~~qml
        if (isMissingTableEntry(item)) {
            return root.unavailableSongPrefix;
        }
        if (!root.arenaSeated || !isChart(item)) {
            return "";
        }
        const availability = Rg.arenaSession.availability;
        const revision = availability.revision;
        return revision >= 0
                && availability.availabilityFor(item.sha256 || "")
                    === ArenaAvailabilityIndex.UnavailableToSome
            ? root.unavailableSongPrefix
            : "";
~~~

- [ ] **Step 6: Verify the select boundary is green**

Run:

~~~powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_lr2_qml_qmllint -j 2
ctest --preset dev-rel -R "LR2 select item model" --output-on-failure
rg -n "arenaUnavailablePrefix|m_arenaUnavailablePrefix|setArenaUnavailablePrefix" RhythmGameQml test
~~~

Expected: the build and focused tests pass; qmllint succeeds; rg prints no matches and exits with code 1.

- [ ] **Step 7: Commit the select-model change**

~~~powershell
git add -- RhythmGameQml/Lr2/Lr2SelectItemModel.h RhythmGameQml/Lr2/Lr2SelectItemModel.cpp RhythmGameQml/Lr2/Lr2SelectContext.qml test/lr2_skin/Lr2SelectBarModel.test.cpp
git commit -m "fix: unify LR2 unavailable song marker"
~~~

### Task 2: Apply the marker to LR2 value-resolver titles

**Files:**
- Modify: RhythmGameQml/Lr2/Lr2SkinValueResolver.qml

**Interfaces:**
- Consumes: selectContext.unavailableSongPrefix
- Preserves: screenRoot.lr2SkinUsesBeatorajaSemantics and the Beatoraja (no song) prefix
- Produces: native-LR2 chartTitle() and chartFullTitle() strings beginning with × for missing table entries and course stages

- [ ] **Step 1: Confirm the resolver still contains the failing legacy paths**

Run:

~~~powershell
rg -n -F '"(missing) "' RhythmGameQml/Lr2/Lr2SkinValueResolver.qml
~~~

Expected: two matches, in missingCourseStageTitle() and missingTableEntryPrefix().

- [ ] **Step 2: Route both resolver paths through the shared property**

Replace missingCourseStageTitle()'s return expression with:

~~~qml
        return (root.lr2SkinUsesBeatorajaSemantics
                ? "(no song) "
                : selectContext.unavailableSongPrefix) + title;
~~~

Replace missingTableEntryPrefix() with:

~~~qml
    function missingTableEntryPrefix(chart: var) : var {
        return !root.lr2SkinUsesBeatorajaSemantics
            && selectContext
            && selectContext.isMissingTableEntry(chart)
            ? selectContext.unavailableSongPrefix
            : "";
    }
~~~

- [ ] **Step 3: Verify all native resolver call sites consume the shared marker**

Run:

~~~powershell
$resolver = Get-Content -LiteralPath 'RhythmGameQml\Lr2\Lr2SkinValueResolver.qml' -Raw
if ($resolver.Contains('"(missing) "')) { throw 'Legacy LR2 missing marker remains in the resolver' }
if (($resolver.Split('selectContext.unavailableSongPrefix').Count - 1) -ne 2) { throw 'Expected both missing resolver paths to use unavailableSongPrefix' }
if (-not $resolver.Contains('"(no song) "')) { throw 'Beatoraja missing-course marker was removed' }
~~~

Expected: the command exits successfully.

- [ ] **Step 4: Lint the changed QML module**

Run:

~~~powershell
cmake --build --preset dev-rel --target RhythmGame_lr2_qml_qmllint -j 2
~~~

Expected: the target succeeds without a missing-property or binding error.

- [ ] **Step 5: Commit the resolver change**

~~~powershell
git add -- RhythmGameQml/Lr2/Lr2SkinValueResolver.qml
git commit -m "fix: mark missing LR2 song titles compactly"
~~~

### Task 3: Remove obsolete translations and verify the full change

**Files:**
- Modify: share/RhythmGame/themes/Default/translations/Default_jp.ts
- Modify: share/RhythmGame/themes/Default/translations/Default_pl.ts
- Modify: share/RhythmGame/themes/Default/translations/Default_zh_CN.ts

**Interfaces:**
- Consumes: translation sources after Lr2SelectContext no longer calls qsTr for either marker
- Produces: catalogs with no Lr2SelectContext messages for (arena unavailable) or (missing)

- [ ] **Step 1: Refresh the configured translation project**

Run:

~~~powershell
cmake --preset dev-rel
cmake --build --preset dev-rel --target Default_translations -j 2
~~~

Expected: lupdate removes the two active Lr2SelectContext messages or marks them vanished in the Japanese, Polish, and Simplified Chinese catalogs.

- [ ] **Step 2: Remove any retained vanished marker context**

In each of Default_jp.ts, Default_pl.ts, and Default_zh_CN.ts, delete the complete context whose name is Lr2SelectContext. That context contains only these two obsolete sources:

~~~xml
<source>(arena unavailable) </source>
<source>(missing) </source>
~~~

Do not edit Default_en.ts and do not alter any neighboring translation context.

- [ ] **Step 3: Prove the catalogs and LR2 sources contain no legacy native marker**

Run:

~~~powershell
$matches = rg -n -F '<source>(arena unavailable) </source>' share/RhythmGame/themes/Default/translations --glob '*.ts'
if ($LASTEXITCODE -eq 0) { throw $matches }
$matches = rg -n -F '<source>(missing) </source>' share/RhythmGame/themes/Default/translations --glob '*.ts'
if ($LASTEXITCODE -eq 0) { throw $matches }
$matches = rg -n -F '"(missing) "' RhythmGameQml/Lr2
if ($LASTEXITCODE -eq 0) { throw $matches }
~~~

Expected: all three searches find no matches.

- [ ] **Step 4: Compile translations and all affected targets**

Run:

~~~powershell
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_lr2_qml_qmllint release_translations RhythmGame_exe -j 2
~~~

Expected: C++, QML cache generation/lint, Qt translation compilation, linking, and the executable target all succeed.

- [ ] **Step 5: Run focused and adjacent automated checks**

Run:

~~~powershell
ctest --preset dev-rel -R "LR2 select item model" --output-on-failure
& .\build\dev-rel\test\bin\RhythmGame_test.exe "[lr2][runtime][select]"
git diff --check
~~~

Expected: all focused LR2 select tests pass and git diff --check reports no whitespace errors.

- [ ] **Step 6: Verify the runtime presentation matrix**

Using a native LR2 skin with a table and Arena session, verify:

1. A chart unavailable to at least one Arena member renders as × Song Title.
2. A locally missing table entry renders as × Song Title.
3. A locally missing course stage renders as × Song Title.
4. A Beatoraja skin shows no Arena text prefix and still uses (no song) for a missing course stage.
5. Switching the application language does not change × or introduce a missing Polish glyph.

- [ ] **Step 7: Commit the translation cleanup**

~~~powershell
git add -- share/RhythmGame/themes/Default/translations/Default_jp.ts share/RhythmGame/themes/Default/translations/Default_pl.ts share/RhythmGame/themes/Default/translations/Default_zh_CN.ts
git commit -m "chore: remove obsolete LR2 marker translations"
~~~
