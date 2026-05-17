pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host

    property string previewScreenKey: ""
    property int customOffset: 0
    property int revision: 0
    property var metadata: ({})
    property var items: []
    property bool suppressNextClockRestart: false

    function localizedName(value: var) : var {
        if (value === undefined || value === null) {
            return "";
        }
        if (typeof value === "string") {
            return value;
        }
        return value.en || value.ja || value[Qt.locale().name] || "";
    }

    function skinTypeScreenKey(type: var) : var {
        switch (type) {
        case 0:
            return "k7";
        case 1:
            return "k5";
        case 2:
            return "k14";
        case 3:
            return "k10";
        case 5:
            return "select";
        case 6:
            return "decide";
        case 7:
            return "result";
        case 10:
            return "soundset";
        case 12:
            return "k7battle";
        case 13:
            return "k5battle";
        case 15:
            return "courseResult";
        default:
            return "";
        }
    }

    function profileRoot() : var {
        return Rg.profileList ? Rg.profileList.mainProfile : null;
    }

    function configuredThemeName(screen: var) : var {
        let profile = root.profileRoot();
        return profile && profile.themeConfig ? (profile.themeConfig[screen] || "") : "";
    }

    function themeFamilyForScreen(screen: var) : var {
        let themeName = root.configuredThemeName(screen);
        return themeName ? Rg.themes.availableThemeFamilies[themeName] : null;
    }

    function screenObject(screen: var) : var {
        let family = root.themeFamilyForScreen(screen);
        return family && family.screens ? family.screens[screen] : null;
    }

    function availableThemeNamesForScreen(screen: var) : var {
        let result = [];
        let families = Rg.themes.availableThemeFamilies || {};
        for (let [name, family] of Object.entries(families)) {
            if (family && family.screens && family.screens[screen]) {
                result.push(name);
            }
        }
        return result;
    }

    function settingDestinationForScreen(screen: var) : var {
        let profile = root.profileRoot();
        let themeName = root.configuredThemeName(screen);
        if (!profile || !profile.vars || !profile.vars.themeVars || !themeName) {
            return null;
        }
        let screenVars = profile.vars.themeVars[screen];
        return screenVars ? screenVars[themeName] : null;
    }

    function currentPreviewScreen() : var {
        return root.previewScreenKey.length > 0
            ? root.previewScreenKey
            : root.defaultPreviewScreen();
    }

    function defaultPreviewScreen() : var {
        if (root.host.effectiveScreenKey === "select" && root.screenObject("k7")) {
            return "k7";
        }
        return root.host.effectiveScreenKey || "select";
    }

    function setPreviewScreen(screen: var) : var {
        if (screen.length === 0) {
            return false;
        }
        if (screen !== "soundset" && !root.screenObject(screen)) {
            return false;
        }
        if (root.previewScreenKey === screen) {
            return true;
        }
        root.previewScreenKey = screen;
        root.customOffset = 0;
        root.refreshItems();
        return true;
    }

    function previewTitle() : var {
        let screen = root.currentPreviewScreen();
        if (screen === "soundset") {
            let vars = root.host.mainGeneralVars();
            return vars ? (vars.soundset || "") : "";
        }
        let themeName = root.configuredThemeName(screen);
        if (root.metadata.title) {
            return root.metadata.title;
        }
        return themeName.length > 0 ? themeName : screen.toUpperCase();
    }

    function previewMaker() : var {
        return root.metadata.maker || "";
    }

    function normalizeSetting(item: var, family: var) : var {
        if (!item || !item.id) {
            return null;
        }

        let result = {
            id: item.id,
            type: item.type || "",
            name: root.localizedName(item.name) || item.id,
            choices: [],
            labels: [],
            defaultValue: item.default || ""
        };

        if (result.type === "choice") {
            let choices = item.choices || [];
            for (let i = 0; i < choices.length; ++i) {
                let choice = choices[i];
                let value = choice.value !== undefined ? choice.value : choice;
                let label = root.localizedName(choice.name) || String(value);
                result.choices.push(String(value));
                result.labels.push(label);
            }
        } else if (result.type === "file") {
            let directory = family && item.path !== undefined
                ? family.path + "/" + item.path
                : "";
            if (directory.length > 0) {
                let files = Rg.fileQuery.getSelectableFilesForDirectory(directory) || [];
                for (let j = 0; j < files.length; ++j) {
                    result.choices.push(String(files[j]));
                    result.labels.push(String(files[j]));
                }
            }
        }

        if (result.defaultValue === "" && result.choices.length > 0) {
            result.defaultValue = result.choices[0];
        }
        if (result.defaultValue !== "" && result.choices.indexOf(result.defaultValue) < 0) {
            result.choices.unshift(result.defaultValue);
            result.labels.unshift(result.defaultValue);
        }
        return result;
    }

    function buildItems() : var {
        let screen = root.currentPreviewScreen();
        if (screen === "soundset") {
            return [];
        }
        let screenObject = root.screenObject(screen);
        if (!screenObject || !screenObject.settingsData || screenObject.settingsData.length === 0) {
            return [];
        }

        let parsed = null;
        try {
            parsed = JSON.parse(screenObject.settingsData);
        } catch (error) {
            console.warn("Failed to parse LR2 settings data for " + screen + ": " + error);
            return [];
        }
        root.metadata = parsed || {};

        let family = root.themeFamilyForScreen(screen);
        let sourceItems = parsed.items || [];
        let result = [];
        for (let i = 0; i < sourceItems.length; ++i) {
            let normalized = root.normalizeSetting(sourceItems[i], family);
            if (normalized) {
                result.push(normalized);
            }
        }
        return result;
    }

    function refreshItems() : void {
        root.metadata = ({});
        root.items = root.buildItems();
        root.customOffset = Math.max(0, Math.min(root.maxOffset(), root.customOffset));
        ++root.revision;
    }

    function maxOffset() : var {
        return Math.max(0, root.items.length - 5);
    }

    function position() : var {
        let max = root.maxOffset();
        return max > 0 ? root.customOffset / max : 0;
    }

    function setPosition(position: var) : var {
        let max = root.maxOffset();
        let next = Math.max(0, Math.min(max, Math.round(position * max)));
        if (next === root.customOffset) {
            return false;
        }
        root.customOffset = next;
        ++root.revision;
        return true;
    }

    function settingAtVisibleRow(row: var) : var {
        let index = root.customOffset + row;
        return index >= 0 && index < root.items.length ? root.items[index] : null;
    }

    function currentValue(item: var) : var {
        let destination = root.settingDestinationForScreen(root.currentPreviewScreen());
        let value = destination && destination[item.id] !== undefined ? destination[item.id] : undefined;
        if ((value === undefined || value === null || value === "") && root.host.skinSettings) {
            value = root.host.skinSettings[item.id];
        }
        if (value === undefined || value === null || value === "") {
            value = item.defaultValue;
        }
        return value === undefined || value === null ? "" : String(value);
    }

    function settingName(row: var) : var {
        let item = root.settingAtVisibleRow(row);
        return item ? item.name : "";
    }

    function settingValueText(row: var) : var {
        let item = root.settingAtVisibleRow(row);
        if (!item) {
            return "";
        }
        let value = root.currentValue(item);
        let index = item.choices.indexOf(value);
        return index >= 0 && index < item.labels.length ? item.labels[index] : value;
    }

    function changeSetting(row: var, delta: var) : var {
        let item = root.settingAtVisibleRow(row);
        if (!item || item.choices.length <= 0) {
            return false;
        }

        let current = root.currentValue(item);
        let index = item.choices.indexOf(current);
        if (index < 0) {
            index = 0;
        }
        let nextValue = item.choices[root.host.wrapValue(index + delta, item.choices.length)];
        if (nextValue === current) {
            return false;
        }

        let screen = root.currentPreviewScreen();
        let destination = root.settingDestinationForScreen(screen);
        if (destination) {
            destination[item.id] = nextValue;
        }
        if (screen === root.host.effectiveScreenKey || screen === root.host.screenKey) {
            let refreshed = root.host.copyObject(destination || root.host.skinSettings);
            refreshed[item.id] = nextValue;
            root.suppressNextClockRestart = true;
            root.host.skinSettings = refreshed;
        }
        ++root.revision;
        return true;
    }

    function changeSoundset(delta: var) : var {
        let vars = root.host.mainGeneralVars();
        if (!vars) {
            return false;
        }
        let choices = vars.getAvailableSoundsets ? vars.getAvailableSoundsets() : [];
        if (!choices || choices.length <= 0) {
            return false;
        }
        let current = vars.soundset || "";
        let index = choices.indexOf(current);
        if (index < 0) {
            index = 0;
        }
        let next = choices[((index + delta) % choices.length + choices.length) % choices.length];
        if (next === current) {
            return false;
        }
        vars.soundset = next;
        root.refreshItems();
        return true;
    }

    function changeSelectedTheme(delta: var) : var {
        let screen = root.currentPreviewScreen();
        if (screen === "soundset") {
            return root.changeSoundset(delta);
        }

        let profile = root.profileRoot();
        if (!profile || !profile.themeConfig) {
            return false;
        }
        let choices = root.availableThemeNamesForScreen(screen);
        if (choices.length <= 0) {
            return false;
        }

        let current = root.configuredThemeName(screen);
        let index = choices.indexOf(current);
        if (index < 0) {
            index = 0;
        }
        let next = choices[((index + delta) % choices.length + choices.length) % choices.length];
        if (next === current) {
            return false;
        }

        profile.themeConfig[screen] = next;
        root.customOffset = 0;
        root.refreshItems();
        return true;
    }

    function queueSkinClockRestartAfterLoad() : var {
        if (root.suppressNextClockRestart) {
            root.suppressNextClockRestart = false;
            return;
        }
        Qt.callLater(root.host.restartSkinClock);
    }
}
