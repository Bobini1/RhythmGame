# 2. Use bindings and reusable components {#theme_tutorial_qml}

[Previous](01-first-screen.md) | [Tutorial](index.md) | [Next: Profiles and settings](03-profile.md)

Install `docs/examples/theme-tutorial/02-qml/` and choose it for Main Menu. You
will see Song select, Multiplayer, Settings and Quit buttons, plus a checkbox
that changes the menu's colors.

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
for actions, such as opening Settings after a button click. Keeping the color
bindings tied to `checked` avoids copying the checkbox's state into another
property and keeping both values in sync.

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
change. Keep helpers in your theme folder rather than importing another theme's
private files, since a user may not have that other theme installed.
Don't depend on the Default skin either, it can change.
If you want something from it, simply copy the code or even image assets, don't try to import its files.

## Choose how the screen resizes

You control how your theme looks at different resolutions.
If you just naively set the coordinates based on your setup,
ui might extend beyond the window for a user with a smaller screen or one that prefers windowed mode.
Or appear in the upper left corner for a user with a huge 4k monitor.

There are two main approaches to handle positioning right:
I call them "bottom-up" and "top-down".

In "bottom-up", you define the exact sizes for each leaf component -
for example, you set a button to 100px width and 50px height.
When you need a column of buttons,
you use the [`Column`](https://doc.qt.io/qt-6/qml-qtquick-column.html) QML type for laying them down.
Or set their `x` and `y` coordinates directly or
with [anchors](https://doc.qt.io/qt-6/qtquick-positioning-anchors.html).

You then put everything in a root component which you use
to scale all of its children depending on the size of the window.
So the 100x50 button will actually get stretched to 200x100 on a bigger screen, for example.
Simple and good. I recommend this approach in most cases.

In "top-down", you don't set the sizes of buttons and other leaf components *anywhere*.
Instead, [Layouts](https://doc.qt.io/qt-6/qtquicklayouts-index.html) do it for you.
The only thing you set are implicit sizes: `implicitWidth`, `implicitHeight`.
So you would make a button with `implicitWidth: 100` and `implicitHeight: 50` and then set `Layout.fillHeight: true`.
If you then put buttons like that in a [`ColumnLayout`](https://doc.qt.io/qt-6/qml-qtquick-layouts-columnlayout.html),
it would leave their width at the implicit `100` but
override their heights to fill the full height of the column evenly.

In the example below, I showcase this technique.
The big trick is that you usually don't set the width and height of the layout either,
you have a hierarchy of layouts with one root layout at the top that you set to fill the whole screen.
A window resize then propagates downward through intermediate layouts down to our leaf components, like the buttons.

```qmllang
ColumnLayout {
    height: 200
    width: 200
    Button {
        implicitWidth: 100
        implicitHeight: 50
        Layout.fillHeight: true
    }
    Button {
        implicitWidth: 100
        implicitHeight: 50
        Layout.fillHeight: true
    }
}
```

In the `Default` theme, the settings screen uses the "top-down" approach
while all other screens generally use the "bottom-up" way.
If you want truly dynamic layouts that adapt to the screen size in some clever way, go with "top-down".
But keep in mind that the learning curve is quite steep.

The menu from the [first example](01-first-screen.md) uses a fixed 960 by 540 canvas.
It is set to fit without stretching, leaving unused space on the sides or top and bottom
if the window has a different side ratio.

\snippet 01-main/main.qml canvas

The second menu's outer layout fills the screen with a margin of 16 pixels.
It contains the whole menu between two empty `Item` objects that act as spacers.

\snippet 02-qml/main.qml layout

The spacers use `Layout.fillHeight` and equal `Layout.preferredHeight` values.
Their preferred heights act as relative weights when the layout has room to
grow. They share the unused height equally, keeping the image and controls
centered together. The outer layout has no spacing, so empty spacers don't
leave gaps in a short window.

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

Try a wide window, a tall window and a short window. The image should stay next
to the heading, and the menu should stay centered. In a short window, the
spacers collapse and then the image shrinks.

To try a 1:4 split, change the bottom spacer's `Layout.preferredHeight` to 4. When there
is spare height, about one fifth goes above the menu and four fifths below it.
The whole menu moves upward, with the image still next to the controls. Qt's
[layout sizing guide](https://doc.qt.io/qt-6/qtquicklayouts-overview.html#size-constraints)
explains how the size limits affect the result.

The checkbox is temporary state, so its value resets when the screen is
created again. In the next lesson, we will save its state in the active profile.

For more detail, read Qt's [property binding guide](https://doc.qt.io/qt-6/qtqml-syntax-propertybinding.html)
and [layout guide](https://doc.qt.io/qt-6/qtquicklayouts-overview.html).
