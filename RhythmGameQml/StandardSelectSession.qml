pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectSession
    \inqmlmodule RhythmGameQml
    \brief Loads folder contents and stores browsing history and pending replies.

    Use this component when implementing browsing behavior that StandardSelectState
    doesn't provide. It stores folder contents, history, scores and preview paths
    without sorting them or choosing the focused row.

    The \c resolve... methods return data without changing \l folderContents or
    \l historyStack. Call \l commitFolderContents and update the history together
    when the navigation is ready to take effect.

    Pass score replies to \l trackScoreDbReply so the session can cancel them
    through \l cancelScoreDbReplies or when it is destroyed. To supply custom
    table courses, set \l tableCoursesProvider to a function that accepts a
    \c tableItem and returns its course list.
*/
Item {
    id: root

    /*! Raw contents of the current folder, table, level, or search. */
    property var folderContents: []
    /*! Navigation history for this selection session. */
    property var historyStack: []
    /*! Score data loaded for the current contents. */
    property var scores: ({})
    /*! Preview-file data loaded for the current contents. */
    property var previewFiles: ({})
    /*! Called as \c tableCoursesProvider(tableItem) to return a custom list of courses. */
    property var tableCoursesProvider: null

    PendingReplyGroup {
        id: scoreDbReplies
    }

    /*! Resolves a history \a item to the folder it represents. */
    function folderForHistoryItem(item) {
        if (item instanceof ChartData) {
            return item.chartDirectory;
        }
        if (item instanceof table) {
            return Rg.tables.resolveTable(item) || null;
        }
        if (item instanceof level) {
            const parentTable = root.historyStack.slice().reverse().find(parent => parent instanceof table);
            if (parentTable) {
                return Rg.tables.resolveLevel(parentTable, item) || null;
            }
        }
        return item;
    }

    /*! Returns raw contents for folder-like \a item. */
    function resolveFolderContents(item) {
        item = folderForHistoryItem(item);
        let folder;
        if (item instanceof table) {
            let courses = typeof tableCoursesProvider === "function"
                ? tableCoursesProvider(item) : item.courses;
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

    /*! Returns search results for \a query. */
    function resolveSearchResults(query) {
        return Rg.songFolderFactory.search(query || "");
    }

    /*! Returns the chart entries in \a directory. */
    function resolveChartDirectory(directory) {
        if (!directory) {
            return null;
        }
        return Rg.songFolderFactory.openChartDirectory(directory);
    }

    /*! Replaces \l folderContents with a copy of \a contents. */
    function commitFolderContents(contents) {
        if (contents === null || contents === undefined) {
            return false;
        }
        folderContents = [...contents];
        return true;
    }

    /*! Retains \a reply until it finishes or the session cancels it. */
    function trackScoreDbReply(reply) {
        return scoreDbReplies.track(reply);
    }

    /*! Cancels and releases all pending score-database replies. */
    function cancelScoreDbReplies() {
        scoreDbReplies.cancelAll();
    }
}
