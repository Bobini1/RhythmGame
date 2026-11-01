# 2. Use bindings and reusable components {#theme_tutorial_qml}

[Previous](01-first-screen.md) | [Tutorial](index.md) | [Next: Profiles and settings](03-profile.md)

**Goal:** Change colors with a binding, reuse a button component, and make the
menu fit different window sizes.

Install `docs/examples/theme-tutorial/02-qml/` and choose it for Main Menu.

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

An `id` gives an object a name within its QML file. `warmColors.checked`
refers to a property of the checkbox, and `actions.openSettings()` calls a
method on the named standard component.

Use bindings for values that depend on other properties. Use signal handlers
for actions, such as opening Settings after a button click.

## Put a repeated control in its own file

`MenuButton.qml` defines a button with a larger font and a consistent height.
A QML file whose name begins with a capital letter can be used as a component
by neighboring QML files. `required` means the caller must supply `label` when
it creates the button.

\include{lineno} 02-qml/MenuButton.qml

The menu uses the component four times. Each instance gets its own label and click
handler, while the shared file supplies its appearance.

\snippet 02-qml/main.qml use-component

Change the font size in `MenuButton.qml` and restart. All four buttons should
change. Keep helpers and assets in your theme folder. If you want to reuse
something from Default, copy it: its private files may change, and other
themes may not be installed.

## Choose how the screen resizes

Two approaches work well:

- **Scale a fixed canvas.** Position controls at a design size, then scale the
  whole screen to fit. This keeps the proportions of your design.
- **Use layouts.** Let `RowLayout` and `ColumnLayout` size their children to
  fit the available space. Use `implicitWidth` and `implicitHeight` for natural
  sizes, and `Layout` properties for size limits and how extra space is shared.

The [first menu](01-first-screen.md) scales a fixed 960 by 540 canvas. It keeps
the same proportions, leaving unused space when the window has a different
aspect ratio.

\snippet 01-main/main.qml canvas

The second menu's outer layout fills the screen with a margin of 16 pixels.
It contains the whole menu between two empty `Item` objects that act as spacers.

\snippet 02-qml/main.qml layout

The spacers use `Layout.fillHeight` and equal `Layout.preferredHeight` values
to share unused height equally. This centers the menu. The outer layout has
no spacing, so collapsed spacers leave no gaps in a short window.

\snippet 02-qml/main.qml top-space

\snippet 02-qml/main.qml bottom-space

The inner column contains the image, heading, buttons and checkbox. Its maximum
height is its natural height, `implicitHeight`, so extra window height goes to
the spacers. Its maximum width is 400, and its children stay 12 pixels apart.

\snippet 02-qml/main.qml menu-layout

The image has a preferred height of 120 and can shrink when the menu has less room.
`Layout.fillHeight` lets the layout reduce its height while the controls keep
their usual sizes.

\snippet 02-qml/main.qml image-layout

A layout controls its immediate children's geometry. Use `Layout` properties
on those children and anchors on the surrounding layout or container.
A label needs a constrained width before it can wrap its text.

## Try the layout

Try a wide window, a tall window and a short window. The image should stay next
to the heading, and the menu should stay centered. In a short window, the
spacers collapse and then the image shrinks.

To try a 1:4 split, change the bottom spacer's `Layout.preferredHeight` to 4. When there
is spare height, about one fifth goes above the menu and four fifths below it.
The whole menu moves upward, with the image still next to the controls. Qt's
[layout sizing guide](https://doc.qt.io/qt-6/qtquicklayouts-overview.html#size-constraints)
explains how the size limits affect the result.

The checkbox resets when the screen is created again. The next lesson adds a
color setting saved in the active profile.

For more detail, read Qt's [property binding guide](https://doc.qt.io/qt-6/qtqml-syntax-propertybinding.html)
and [layout guide](https://doc.qt.io/qt-6/qtquicklayouts-overview.html).
