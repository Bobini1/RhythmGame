# 2. Use bindings and reusable components {#theme_tutorial_qml}

[Previous](01-first-screen.md) | [Tutorial](index.md) | [Next: Profiles and settings](03-profile.md)

Install `docs/examples/theme-tutorial/02-qml/` and choose it for Main Menu. You
will see Song select, Settings and Quit buttons, plus a checkbox that changes the
menu's colors.

[Download 02-qml](examples/theme-tutorial/02-qml.zip) or
[open the source folder](https://github.com/Bobini1/RhythmGame/tree/master/docs/examples/theme-tutorial/02-qml).

## Let properties update the screen

A binding is an expression assigned to a QML property with `:`. When a value
used by the expression changes, QML calculates the expression again. Both the
background and the accent color depend on the checkbox's `checked` property
in the example.

\snippet 02-qml/main.qml binding

The checkbox already keeps track of whether it is checked, so the screen can
use that value directly. Clicking the checkbox updates both colors through
their bindings. The `Behavior` animates each change to the background color
over 180 milliseconds.

\snippet 02-qml/main.qml change-state

`readonly` prevents other code from assigning a new value to `accent`. It
doesn't stop the binding from updating. Keep the color expressions as bindings
rather than setting the label's color in the checkbox handler. Assigning a
value directly to a property replaces any binding that property had.

An `id` gives an object a name within its QML component. `warmColors.checked`
refers to a property of the checkbox, and `actions.openSettings()` calls a
method on the named standard component. Use explicit names when referring
to another object so readers can see where a value comes from.

Use bindings for values that depend on other properties. Use signal handlers
for actions, such as opening Settings after a button click. Keeping the color
bindings tied to `checked` avoids copying the checkbox's state into another
property and keeping both values in sync.

## Put a repeated control in its own file

`MenuButton.qml` defines a button with a larger font and a consistent height.
A QML file whose name begins with a capital letter can be used as a component
by neighboring QML files. `required` means the caller must supply `label` when
it creates the button.

\include{lineno} 02-qml/MenuButton.qml

The menu uses the component three times. Each instance gets its own label and click
handler, while the shared file supplies its appearance.

\snippet 02-qml/main.qml use-component

Change the font size in `MenuButton.qml` and restart. All three buttons should
change. Keep helpers in your theme folder rather than importing another theme's
private files, since a user may not have that other theme installed.

## Choose how the screen resizes

The first menu uses a fixed 960 by 540 canvas. Its scale is the smaller of the
window's width and height ratios, so it fits without stretching. Unused space
remains at the sides or above and below the canvas.

\snippet 01-main/main.qml canvas

The second menu uses a layout that grows with the window up to a width of 400.
Its children use `Layout.fillWidth`, while the layout itself has an explicit
width. A label needs a constrained width before it can wrap its text.

\snippet 02-qml/main.qml layout

A layout controls its immediate children's geometry. Use `Layout` properties
on those children and anchors on the surrounding layout or container.
Default uses both approaches, with a scaled gameplay screen and responsive
settings pages.

The checkbox is temporary state, so its value resets when the screen is
created again. In the next lesson, save a setting in the active profile.
For more detail, read Qt's [property binding guide](https://doc.qt.io/qt-6/qtqml-syntax-propertybinding.html)
and [layout guide](https://doc.qt.io/qt-6/qtquicklayouts-overview.html).
