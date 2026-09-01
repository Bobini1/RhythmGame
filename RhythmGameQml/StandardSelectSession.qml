pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectSession
    \inqmlmodule RhythmGameQml
    \brief Owns the nonvisual lifetime of a selection session.

    The component provides raw folder loading, history storage, preview data,
    and pending score-query cancellation shared by standard and legacy skins.

    This is a low-level storage and acquisition component. The \c resolve...
    functions return data without changing \l folderContents or
    \l historyStack. Call \l commitFolderContents explicitly and update history
    as one transaction. Replies passed to \l trackScoreDbReply are cancelled by
    \l cancelScoreDbReplies and when the reply group is destroyed.

    \l tableCoursesProvider, when set, is called as
    \c tableCoursesProvider(tableItem) and must return the table's course list.
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
    /*! Optional \c tableCoursesProvider(tableItem) course-list provider. */
    property var tableCoursesProvider: null

    PendingReplyGroup {
        id: scoreDbReplies
    }

    /*! Resolves a history \a item to the folder it represents. */
    function folderForHistoryItem(item) {
        return item instanceof ChartData ? item.chartDirectory : item;
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
