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
        if (Rg.rootSongFoldersConfig.rescan(folderPath)) {
            return true;
        }
        Rg.songFolderFactory.refresh(true);
        return false;
    }

    QtObject {
        id: implementation

        function reloadTableForItem(item: var): var {
            return item instanceof table && Rg.tables.reloadTable(item);
        }

    }
}
