# Theme Development

If you want to edit a theme, you only need a text editor.
Edit the scripts and relaunch the game to see your changes.

All errors are printed to log.txt in the assets folder. On platforms other than Windows,
the log is printed to the console as well.
You can also press F10 in-game to open the log overlay.

# Theme structure

A theme is a folder in `assets/themes/` that contains a `theme.json` file.
That file needs to have a `scripts` field with an object
containing relative paths to QML files for each game screen implemented by the theme.

Here is an example of a minimal `theme.json` file:

```json
{
    "scripts": {
        "select": "select.qml"
    }
}
```

