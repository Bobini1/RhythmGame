pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectSession
    \inqmlmodule RhythmGameQml
    \brief Loads folder contents and stores browsing history.

    StandardSelectState already uses this component. Use it directly only for
    custom browsing. It stores folder contents, history and preview paths;
    your code decides when to load them, sort them and choose a focused row.

    The \c resolve... methods return data without changing \l folderContents or
    \l historyStack. Call \l commitFolderContents and update the history together
    when the navigation is ready to take effect.

    To supply custom table courses, set \l tableCoursesProvider to a function that accepts a
    \c tableItem and returns its course list.
*/
Item {
    id: root

    /*! Raw contents of the current folder, table, level, or search. */
    property var folderContents: []
    /*!
        History entries in navigation order. The root folder is an empty string.
        StandardSelectState also uses table and level values, ChartData for a
        song's chart directory, and objects with a searchQuery field for searches.
    */
    property var historyStack: []
    /*! Preview-file data loaded for the current contents. */
    property var previewFiles: ({})
    /*! Called as \c tableCoursesProvider(tableItem) to return a custom list of courses. */
    property var tableCoursesProvider: null

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

    /*!
        Returns raw entries for folder, table or level \a item. An empty string
        opens the root. Returns an empty list for an empty folder, or null for
        an unsupported item. Does not change history or \l folderContents.
    */
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

    /*!
        Replaces \l folderContents with a copy of \a contents and returns true.
        Null or undefined returns false without changing the current contents.
        Update \l historyStack separately when navigation succeeds.
    */
    function commitFolderContents(contents) {
        if (contents === null || contents === undefined) {
            return false;
        }
        folderContents = [...contents];
        return true;
    }

}
