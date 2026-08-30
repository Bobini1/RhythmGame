import QtQuick
import RhythmGameQml

// Nonvisual selection-session primitives shared by standard and legacy skins:
// raw folder loading, history storage and pending score-query lifetime.
QtObject {
    id: root

    property var folderContents: []
    property var historyStack: []
    property var pendingScoreDbReplies: []
    property var scores: ({})
    property var previewFiles: ({})
    property var tableCoursesAction: null

    function folderForHistoryItem(item) {
        return item instanceof ChartData ? item.chartDirectory : item;
    }

    function resolveFolderContents(item) {
        item = folderForHistoryItem(item);
        let folder;
        if (item instanceof table) {
            let courses = typeof tableCoursesAction === "function"
                ? tableCoursesAction(item) : item.courses;
            folder = [...item.levels, ...(courses || [])];
        } else if (item instanceof level) {
            folder = item.loadCharts();
        } else if (typeof item === "string") {
            folder = [];
            if (item === "") {
                for (let tableItem of Rg.tables.getList()) {
                    if (tableItem.status === table.Loaded) {
                        folder.push(tableItem);
                    }
                }
            }
            folder.push(...Rg.songFolderFactory.open(item));
            if (item !== "" && folder.length === 0) {
                folder.push(...Rg.songFolderFactory.openChartDirectory(item));
            }
        } else {
            return null;
        }
        return folder.slice();
    }

    function resolveSearchResults(query) {
        return Rg.songFolderFactory.search(query || "");
    }

    function resolveChartDirectory(directory) {
        if (!directory) {
            return null;
        }
        return Rg.songFolderFactory.openChartDirectory(directory);
    }

    function commitFolderContents(contents) {
        if (contents === null || contents === undefined) {
            return false;
        }
        folderContents = [...contents];
        return true;
    }

    function trackScoreDbReply(reply) {
        if (!reply || reply.resultAvailable) {
            return reply;
        }
        pendingScoreDbReplies.push(reply);
        pendingScoreDbReplies = pendingScoreDbReplies.slice();
        let forget = function() {
            reply.finished.disconnect(forget);
            let index = pendingScoreDbReplies.indexOf(reply);
            if (index >= 0) {
                pendingScoreDbReplies.splice(index, 1);
                pendingScoreDbReplies = pendingScoreDbReplies.slice();
            }
        };
        reply.finished.connect(forget);
        return reply;
    }

    function cancelScoreDbReplies() {
        let replies = pendingScoreDbReplies;
        pendingScoreDbReplies = [];
        for (let reply of replies) {
            if (reply && !reply.resultAvailable) {
                reply.cancel();
            }
        }
    }

    Component.onDestruction: cancelScoreDbReplies()
}
