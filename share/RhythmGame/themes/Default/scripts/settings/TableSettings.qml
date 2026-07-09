pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts

import "SettingsColors.js" as SettingsColors

Item {
    id: tableSettings

    // ── Browse state ───────────────────────────────────────────────────────

    property var allTables: []
    property string fetchState: "idle"   // "idle" | "loading" | "done" | "error"
    property string fetchError: ""

    property bool   recommendedOnly: true
    property string tag1Filter:      ""
    property string tag2Filter:      ""
    property string browserSearch:   ""
    property bool   showUrlEditor:   false   // toggled by the ⚙ button
    property var    fetchRequest:     null

    readonly property int installedPaneMinimumWidth: 360
    readonly property int installedPaneRowMinimumWidth: 560

    Component.onCompleted: fetchTables()
    Component.onDestruction: cancelFetch()

    readonly property var tagTranslations: ({
        // tag1 — play style
        "SP":                   qsTr("SP"),
        "DP":                   qsTr("DP"),
        "EVENT":                qsTr("EVENT"),
        "PMS":                  qsTr("PMS"),
        "etc":                  qsTr("etc"),
        // tag2 — category
        "General":              qsTr("General"),
        "Personal":             qsTr("Personal"),
        "BMS Event":            qsTr("BMS Event"),
        "Chart Event":          qsTr("Chart Event"),
        "Self-made Chart Only": qsTr("Self-made Chart Only"),
        "Uploader":             qsTr("Uploader")
    })

    function translateTag(tag) {
        return tagTranslations[tag] ?? tag
    }

    function currentTableListUrl() {
        const generalVars = Rg.profileList?.mainProfile?.vars?.generalVars
        return generalVars?.tableListUrl ? String(generalVars.tableListUrl).trim() : ""
    }

    function exceptionMessage(error) {
        return error?.message !== undefined ? String(error.message) : String(error)
    }

    function cancelFetch() {
        const xhr = fetchRequest
        fetchRequest = null
        if (xhr) {
            xhr.onreadystatechange = function () {}
            xhr.abort()
        }
    }

    readonly property var recommendedUrls: {
        const raw = [
            "https://mqppppp.neocities.org/StardustTable.html",
            "https://djkuroakari.github.io/starlighttable.html",
            "https://stellabms.xyz/sl/table.html",
            "https://stellabms.xyz/st/table.html",
            "https://darksabun.club/table/archive/normal1/",
            "https://darksabun.club/table/archive/insane1/",
            "http://rattoto10.jounin.jp/table.html",
            "http://rattoto10.jounin.jp/table_insane.html",
            "https://rattoto10.jounin.jp/table_overjoy.html",
            "https://lets-go-time-hell.github.io/code-stream-table/",
            "https://lets-go-time-hell.github.io/Arm-Shougakkou-table/",
            "https://su565fx.web.fc2.com/Gachimijoy/gachimijoy.html",
            "https://stellabms.xyz/so/table.html",
            "https://stellabms.xyz/sn/table.html",
            "https://air-afother.github.io/osu-table/",
            "https://bms.hexlataia.xyz/tables/ai.html",
            "https://bms.hexlataia.xyz/tables/db.html",
            "https://bms.hexlataia.xyz/tables/olduploader.html",
            "https://stellabms.xyz/upload.html",
            "https://exturbow.github.io/github.io/index.html",
            "http://fezikedifficulty.futene.net/list.html",
            "https://ladymade-star.github.io/luminous/table.html",
            "https://vinylhouse.web.fc2.com/lntougou/difficulty.html",
            "http://flowermaster.web.fc2.com/lrnanido/gla/LN.html",
            "https://skar-wem.github.io/ln/",
            "http://cerqant.web.fc2.com/zindy/table.html",
            "https://notepara.com/glassist/lnoj",
            "https://egret9.github.io/Scramble/",
            "http://minddnim.web.fc2.com/sara/3rd_hard/bms_sara_3rd_hard.html",
            "https://lets-go-time-hell.github.io/Delay-joy-table/",
            "https://kamikaze12345.github.io/github.io/delaytrainingtable/table.html",
            "https://wrench616.github.io/Delay/",
            "https://darksabun.club/table/archive/old-overjoy/",
            "https://monibms.github.io/Dystopia/dystopia.html",
            "https://www.firiex.com/tables/joverjoy",
            "https://plyfrm.github.io/table/timing/",
            "https://plyfrm.github.io/table/bmssearch/index.html",
            "https://yaruki0.net/DPlibrary/",
            "https://stellabms.xyz/dp/table.html",
            "https://stellabms.xyz/dpst/table.html",
            "https://deltabms.yaruki0.net/table/data/dpdelta_head.json",
            "https://deltabms.yaruki0.net/table/data/insane_head.json",
            "http://ereter.net/dpoverjoy/",
            "https://notmichaelchen.github.io/stella-table-extensions/satellite-easy.html",
            "https://notmichaelchen.github.io/stella-table-extensions/satellite-normal.html",
            "https://notmichaelchen.github.io/stella-table-extensions/satellite-hard.html",
            "https://notmichaelchen.github.io/stella-table-extensions/satellite-fullcombo.html",
            "https://notmichaelchen.github.io/stella-table-extensions/stella-easy.html",
            "https://notmichaelchen.github.io/stella-table-extensions/stella-normal.html",
            "https://notmichaelchen.github.io/stella-table-extensions/stella-hard.html",
            "https://notmichaelchen.github.io/stella-table-extensions/stella-fullcombo.html",
            "https://notmichaelchen.github.io/stella-table-extensions/dp-satellite-easy.html",
            "https://notmichaelchen.github.io/stella-table-extensions/dp-satellite-normal.html",
            "https://notmichaelchen.github.io/stella-table-extensions/dp-satellite-hard.html",
            "https://notmichaelchen.github.io/stella-table-extensions/dp-satellite-fullcombo.html",
            "http://walkure.net/hakkyou/for_glassist/bms/?lamp=easy",
            "http://walkure.net/hakkyou/for_glassist/bms/?lamp=normal",
            "http://walkure.net/hakkyou/for_glassist/bms/?lamp=hard",
            "http://walkure.net/hakkyou/for_glassist/bms/?lamp=fc"
        ]
        const s = new Set()
        for (const u of raw) s.add(u.toLowerCase().replace(/\/$/, ""))
        return s
    }

    // ── Derived option lists and filtered view ─────────────────────────────

    readonly property var tag1Options: {
        const s = new Set()
        for (const t of allTables) if (t.tag1) s.add(t.tag1)
        return Array.from(s).sort()
    }
    readonly property var tag2Options: {
        const s = new Set()
        for (const t of allTables) if (t.tag2) s.add(t.tag2)
        return Array.from(s).sort()
    }

    readonly property var filteredTables: {
        const q = browserSearch.toLowerCase()
        const results = allTables.filter(t => {
            if (recommendedOnly) {
                const norm = t.url.toLowerCase().replace(/\/$/, "")
                if (!recommendedUrls.has(norm)) return false
            }
            if (tag1Filter && t.tag1 !== tag1Filter) return false
            if (tag2Filter && t.tag2 !== tag2Filter) return false
            if (q && !t.name.toLowerCase().includes(q)
                  && !(t.comment && t.comment.toLowerCase().includes(q))
                  && !t.url.toLowerCase().includes(q))
                return false
            return true
        })
        results.sort((a, b) =>
            parseInt(a.tag_order || 0) - parseInt(b.tag_order || 0))
        return results
    }

    function fetchTables() {
        if (fetchState === "loading") return
        const url = currentTableListUrl()
        if (!url) {
            allTables = []
            fetchError = qsTr("No table list URL configured")
            fetchState = "error"
            return
        }

        cancelFetch()

        const xhr = new XMLHttpRequest()
        fetchRequest = xhr
        xhr.open("GET", url)
        fetchState = "loading"
        fetchError = ""
        xhr.onreadystatechange = function () {
            if (xhr.readyState !== XMLHttpRequest.DONE) return
            if (fetchRequest !== xhr) return
            fetchRequest = null
            if (xhr.status === 200) {
                try {
                    const parsed = JSON.parse(xhr.responseText)
                    allTables = Array.isArray(parsed) ? parsed : (parsed.data ?? [])
                    fetchState = "done"
                } catch (e) {
                    fetchError = exceptionMessage(e)
                    fetchState = "error"
                }
            } else {
                fetchError = "HTTP " + xhr.status
                fetchState = "error"
            }
        }
        xhr.send()
    }

    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            property bool held: false

            required property int index
            required property var display

            anchors {
                left: parent?.left
                right: parent?.right
            }
            height: content.height

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressed: held = true
            onReleased: held = false


            Rectangle {
                id: content
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width
                height: Math.max(72, row.implicitHeight + 14)

                property bool highlighted: dragArea.held

                radius: 7
                color: SettingsColors.rowFill(palette, highlighted, rowHover.hovered)
                border.width: highlighted ? 1 : 0
                border.color: SettingsColors.panelBorder(palette)

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                states: State {
                    when: dragArea.held

                    ParentChange {
                        target: content
                        parent: tableSettings
                    }
                    AnchorChanges {
                        target: content
                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }

                HoverHandler {
                    id: rowHover
                }

                RowLayout {
                    id: row

                    anchors.fill: parent
                    spacing: 12
                    anchors.margins: 8

                    TagChip {
                        text: qsTr("Drag")
                        Layout.alignment: Qt.AlignVCenter
                        ToolTip.text: qsTr("Drag to reorder")
                        ToolTip.visible: dragHandleHover.hovered
                        ToolTip.delay: 500

                        HoverHandler {
                            id: dragHandleHover
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 2

                        Label {
                            id: tableName

                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                            font.bold: true
                            text: dragArea.display.name
                            color: palette.text
                        }

                        Label {
                            id: tableUrl

                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                            text: dragArea.display.url
                            color: SettingsColors.alpha(palette.text, 0.7)
                            font.pixelSize: 11
                        }
                    }
                    Component {
                        id: defaultItem
                        Item {
                        }
                    }
                    Loader {
                        Layout.preferredHeight: 32
                        Layout.preferredWidth: 48

                        Component {
                            id: errorItem
                            Label {
                                visible: dragArea.display.status === table.Error
                                text: qsTr("Error")
                                color: SettingsColors.dangerText(palette)
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Component {
                            id: loadingItem
                            Item {
                                BusyIndicator {
                                    running: dragArea.display.status === table.Loading
                                    anchors.fill: parent
                                    anchors.margins: -8
                                }
                            }
                        }
                        sourceComponent: {
                            if (dragArea.display.status === table.Error) {
                                return errorItem;
                            } else if (dragArea.display.status === table.Loading) {
                                return loadingItem;
                            }
                            return defaultItem;
                        }
                    }
                    ActionButton {
                        text: qsTr("Reload")
                        tone: ActionButton.Secondary

                        onClicked: {
                            Rg.tables.reload(dragArea.index);
                        }
                    }
                    ActionButton {
                        text: qsTr("Remove")
                        tone: ActionButton.Danger

                        onClicked: {
                            Rg.tables.removeAt(dragArea.index);
                        }
                    }
                }
            }
            DropArea {
                anchors {
                    fill: parent
                    margins: 10
                }

                onEntered: (drag) => {
                    Rg.tables.reorder(
                        drag.source.index,
                        dragArea.index)
                }
            }
        }
    }

    // ── Browse: entry delegate ─────────────────────────────────────────────

    Component {
        id: browseDelegate

        Rectangle {
            required property var modelData
            required property int index

            width: ListView.view.width
            height: browseRow.implicitHeight + 12
            clip: true

            color: index % 2 === 0 ? palette.base : palette.alternateBase

            RowLayout {
                id: browseRow
                anchors {
                    left: parent.left; right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 8
                }
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 2

                    Label {
                        text: modelData.name
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                    }

                    Label {
                        text: modelData.url
                        elide: Text.ElideRight
                        font.pixelSize: 11
                        color: palette.link
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        HoverHandler {
                            id: urlHover
                            cursorShape: Qt.PointingHandCursor
                        }
                        TapHandler {
                            onTapped: Qt.openUrlExternally(modelData.url)
                        }
                        ToolTip.text: modelData.url
                        ToolTip.visible: urlHover.hovered
                        ToolTip.delay: 600
                    }

                    Label {
                        visible: !!modelData.comment
                        text: modelData.comment
                        elide: Text.ElideRight
                        textFormat: Text.RichText
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        onLinkActivated: (link) => Qt.openUrlExternally(link)
                    }
                }

                TagChip {
                    visible: !!modelData.tag1
                    text: tableSettings.translateTag(modelData.tag1)
                    TapHandler {
                        cursorShape: Qt.PointingHandCursor
                        onTapped: tableSettings.tag1Filter = modelData.tag1
                    }
                }

                TagChip {
                    visible: !!modelData.tag2
                    text: tableSettings.translateTag(modelData.tag2)
                    TapHandler {
                        cursorShape: Qt.PointingHandCursor
                        onTapped: tableSettings.tag2Filter = modelData.tag2
                    }
                }

                ActionButton {
                    text: qsTr("Add")
                    tone: ActionButton.Primary
                    onClicked: Rg.tables.add(modelData.url)
                }
                Rectangle {
                    Layout.preferredWidth: 5
                    color: "transparent"
                }
            }
        }
    }

    // ── Main layout: side-by-side split ───────────────────────────────────

    SettingsWorkspaceScaffold {
        id: pageScaffold

        anchors.fill: parent
        contentSpacing: 12

        SettingsPageHeader {
            title: qsTr("Tables")
            subtitle: qsTr("Manage installed tables and browse recommended table sources.")
            Layout.fillWidth: true
        }

        SplitView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            orientation: Qt.Horizontal

            // ── Left pane: My Tables ──────────────────────────────────────

            Item {
                SplitView.minimumWidth: tableSettings.installedPaneMinimumWidth
                SplitView.fillWidth: true
                SplitView.preferredWidth: 760

                Flickable {
                    id: installedPaneFlickable

                    anchors.fill: parent
                    contentWidth: Math.max(
                        tableSettings.installedPaneRowMinimumWidth,
                        myTabFrame.implicitWidth,
                        width)
                    contentHeight: Math.max(myTabFrame.implicitHeight, parent.height)
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.horizontal: ScrollBar {}

                    WorkbenchPanel {
                        id: myTabFrame

                        width: installedPaneFlickable.contentWidth
                        height: installedPaneFlickable.height
                        title: qsTr("Installed tables")
                        subtitle: qsTr("Drag to reorder; reload or remove individual sources.")

                        ColumnLayout {
                            Layout.fillHeight: true
                            Layout.fillWidth: true

                            ListView {
                                id: songList

                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                flickableDirection: Flickable.VerticalFlick
                                ScrollBar.vertical: ScrollBar {}
                                clip: true
                                spacing: 5
                                model: Rg.tables
                                delegate: dragDelegate
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                TextField {
                                    id: textField

                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 3
                                    placeholderText: qsTr("Add table")
                                    onAccepted: { Rg.tables.add(text); text = "" }
                                }

                                ActionButton {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    text: qsTr("Add")
                                    tone: ActionButton.Primary
                                    onClicked: { Rg.tables.add(textField.text); textField.text = "" }
                                }
                            }
                        }
                    }
                }
            }

            // ── Right pane: Browse ────────────────────────────────────────

            WorkbenchPanel {
                title: qsTr("Browse tables")
                subtitle: qsTr("Filter the remote table list, then add sources to your installed tables.")
                SplitView.minimumWidth: 420
                SplitView.fillWidth: false
                SplitView.preferredWidth: 560

                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    spacing: 0

                    // Filter bar — single row when wide enough, two rows when narrow
                    Item {
                        id: filterBar

                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.topMargin: 8
                        Layout.bottomMargin: 4

                        readonly property bool twoRows: width < 800
                        implicitHeight: twoRows
                            ? filterCombos.implicitHeight + 4 + filterSearch.implicitHeight
                            : filterSearch.implicitHeight

                        // ── Row 1 (two-row mode only): Recommended + Type + Category ─
                        RowLayout {
                            id: filterCombos

                            visible: filterBar.twoRows
                            anchors { top: parent.top; left: parent.left; right: parent.right }
                            spacing: 8

                            CheckBox {
                                text: qsTr("Recommended")
                                checked: tableSettings.recommendedOnly
                                onToggled: tableSettings.recommendedOnly = checked
                                Layout.fillWidth: true
                            }

                            Label { text: qsTr("Type:") }

                            ComboBox {
                                model: [qsTr("Any")].concat(
                                    tableSettings.tag1Options.map(t => tableSettings.translateTag(t)))
                                currentIndex: {
                                    if (!tableSettings.tag1Filter) return 0
                                    const i = tableSettings.tag1Options.indexOf(tableSettings.tag1Filter)
                                    return i >= 0 ? i + 1 : 0
                                }
                                onActivated: tableSettings.tag1Filter =
                                    currentIndex > 0 ? tableSettings.tag1Options[currentIndex - 1] : ""
                            }

                            Label { text: qsTr("Category:") }

                            ComboBox {
                                model: [qsTr("Any")].concat(
                                    tableSettings.tag2Options.map(t => tableSettings.translateTag(t)))
                                currentIndex: {
                                    if (!tableSettings.tag2Filter) return 0
                                    const i = tableSettings.tag2Options.indexOf(tableSettings.tag2Filter)
                                    return i >= 0 ? i + 1 : 0
                                }
                                onActivated: tableSettings.tag2Filter =
                                    currentIndex > 0 ? tableSettings.tag2Options[currentIndex - 1] : ""
                            }
                        }

                        // ── Row 2 (always shown): in single-row mode also carries
                        //    Recommended / Type / Category before the search field. ──
                        RowLayout {
                            id: filterSearch

                            anchors {
                                top: filterBar.twoRows ? filterCombos.bottom : parent.top
                                topMargin: filterBar.twoRows ? 4 : 0
                                left: parent.left
                                right: parent.right
                            }
                            spacing: 8

                            CheckBox {
                                visible: !filterBar.twoRows
                                text: qsTr("Recommended")
                                checked: tableSettings.recommendedOnly
                                onToggled: tableSettings.recommendedOnly = checked
                            }

                            Label { visible: !filterBar.twoRows; text: qsTr("Type:") }

                            ComboBox {
                                visible: !filterBar.twoRows
                                model: [qsTr("Any")].concat(
                                    tableSettings.tag1Options.map(t => tableSettings.translateTag(t)))
                                currentIndex: {
                                    if (!tableSettings.tag1Filter) return 0
                                    const i = tableSettings.tag1Options.indexOf(tableSettings.tag1Filter)
                                    return i >= 0 ? i + 1 : 0
                                }
                                onActivated: tableSettings.tag1Filter =
                                    currentIndex > 0 ? tableSettings.tag1Options[currentIndex - 1] : ""
                            }

                            Label { visible: !filterBar.twoRows; text: qsTr("Category:") }

                            ComboBox {
                                visible: !filterBar.twoRows
                                model: [qsTr("Any")].concat(
                                    tableSettings.tag2Options.map(t => tableSettings.translateTag(t)))
                                currentIndex: {
                                    if (!tableSettings.tag2Filter) return 0
                                    const i = tableSettings.tag2Options.indexOf(tableSettings.tag2Filter)
                                    return i >= 0 ? i + 1 : 0
                                }
                                onActivated: tableSettings.tag2Filter =
                                    currentIndex > 0 ? tableSettings.tag2Options[currentIndex - 1] : ""
                            }

                            TextField {
                                Layout.fillWidth: true
                                placeholderText: qsTr("Search…")
                                onTextChanged: tableSettings.browserSearch = text
                            }

                            ActionButton {
                                text: qsTr("Reload")
                                tone: ActionButton.Secondary
                                enabled: tableSettings.fetchState !== "loading"
                                onClicked: tableSettings.fetchTables()
                            }

                            ToolButton {
                                text: "⚙"
                                checkable: true
                                checked: tableSettings.showUrlEditor
                                onToggled: tableSettings.showUrlEditor = checked
                                ToolTip.text: qsTr("Configure source URL")
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                            }
                        }
                    }

                    // Collapsible URL editor — hidden by default
                    RowLayout {
                        visible: tableSettings.showUrlEditor
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.bottomMargin: 4
                        spacing: 8

                        Label { text: qsTr("Source URL:") }

                        TextField {
                            id: urlField

                            Layout.fillWidth: true
                            Component.onCompleted: {
                                text = Rg.profileList.mainProfile.vars.generalVars.tableListUrl
                            }
                            onEditingFinished: {
                                const gv = Rg.profileList.mainProfile.vars.generalVars
                                if (text !== gv.tableListUrl) {
                                    gv.tableListUrl = text
                                    tableSettings.fetchState = "idle"
                                    tableSettings.allTables = []
                                }
                            }
                        }

                        ActionButton {
                            text: qsTr("Reset")
                            tone: ActionButton.Secondary
                            onClicked: {
                                const gv = Rg.profileList.mainProfile.vars.generalVars
                                gv.resetTableListUrl()
                                urlField.text = gv.tableListUrl
                                tableSettings.fetchState = "idle"
                                tableSettings.allTables = []
                            }
                        }
                    }

                    // Status overlay + list
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: tableSettings.fetchState === "loading"
                            visible: running
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: tableSettings.fetchState === "error"
                            title: qsTr("Failed to load table list")
                            subtitle: tableSettings.fetchError
                            width: Math.min(400, parent.width - 32)
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: tableSettings.fetchState === "done"
                                && tableSettings.filteredTables.length === 0
                            title: qsTr("No tables match")
                            subtitle: qsTr("Change the filters or search text.")
                            width: Math.min(400, parent.width - 32)
                        }

                        ListView {
                            anchors.fill: parent
                            visible: tableSettings.fetchState === "done"
                                && tableSettings.filteredTables.length > 0
                            clip: true
                            spacing: 0
                            model: tableSettings.filteredTables
                            delegate: browseDelegate
                            ScrollBar.vertical: ScrollBar {}
                        }
                    }
                }
            }
        }
    }
}
