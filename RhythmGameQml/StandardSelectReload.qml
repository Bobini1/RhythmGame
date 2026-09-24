pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectReload
    \inqmlmodule RhythmGameQml
    \brief Reloads a table or scans the root folder containing the current song.

    StandardSelectState and StandardSelectController already include this behavior.
    Use the component separately when writing your own selector.

    Call \l reload with the focused item, browsing history and folder path.
    A true return means a reload or scan was requested. A false return means the
    caller should refresh its current folder contents instead. The caller keeps
    responsibility for navigation and focus.
*/
Item {
    id: root

    /*!
        Reloads the table at \a focusedItem or in \a history, or scans the root containing \a
        folderPath. Returns true when a reload or scan was requested.
    */
    function reload(focusedItem, history, folderPath) {
        if (!root.enabled) {
            return false;
        }
        if (implementation.reloadTableForItem(focusedItem)) {
            return true;
        }
        for (let i = history.length - 1; i >= 0; --i) {
            if (implementation.reloadTableForItem(history[i])) {
                return true;
            }
        }
        if (implementation.scanRootSongFolderForPath(folderPath)) {
            return true;
        }
        Rg.songFolderFactory.refresh(true);
        return false;
    }

    QtObject {
        id: implementation

        function normalizeLocalPath(path) {
            let value = String(path || "").trim();
            if (value.length === 0) {
                return "";
            }
            if (/^file:\/\//i.test(value)) {
                let url = value;
                if (/^file:\/\/\//i.test(url)) {
                    value = url.slice(8);
                } else {
                    value = url.slice(7);
                }
                value = decodeURIComponent(value);
            }
            value = value.replace(/\\/g, "/");
            while (value.length > 3 && value.endsWith("/")) {
                value = value.slice(0, -1);
            }
            return value;
        }

        function rootSongFolderForPath(path) {
            let target = implementation.normalizeLocalPath(path);
            if (target.length === 0 || !Rg.rootSongFoldersConfig
                    || !Rg.rootSongFoldersConfig.folders) {
                return null;
            }
            let targetLower = target.toLowerCase();
            let folders = Rg.rootSongFoldersConfig.folders;
            let best = null;
            let bestLength = -1;
            for (let i = 0; i < folders.rowCount(); ++i) {
                let folder = folders.at(i);
                let folderPath = implementation.normalizeLocalPath(
                    folder ? folder.name : "");
                if (folderPath.length === 0) {
                    continue;
                }
                let folderLower = folderPath.toLowerCase();
                let matches = targetLower === folderLower
                    || targetLower.startsWith(folderLower + "/");
                if (matches && folderLower.length > bestLength) {
                    best = folder;
                    bestLength = folderLower.length;
                }
            }
            return best;
        }

        function scanRootSongFolderForPath(path: var): var {
            let folder = implementation.rootSongFolderForPath(path);
            return !!folder && !!Rg.rootSongFoldersConfig && !!Rg.rootSongFoldersConfig.scanningQueue && Rg.rootSongFoldersConfig.scanningQueue.scan(folder);
        }

        function reloadTableForItem(item: var): var {
            return item instanceof table && Rg.tables.reloadTable(item);
        }

    }
}
