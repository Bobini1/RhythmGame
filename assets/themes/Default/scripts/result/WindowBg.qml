import QtQuick

Item {
    id: window

    Column {
        Row {
            Image {
                id: upper_left

                source: root.iniImagesUrl + "parts.png/window_upper_left"
            }
            Image {
                id: top

                source: root.iniImagesUrl + "parts.png/window_top"
                width: window.width - upper_left.width - upper_right.width
            }
            Image {
                id: upper_right

                source: root.iniImagesUrl + "parts.png/window_upper_right"
            }
        }
        Row {
            Image {
                id: left

                height: window.height - upper_left.height - lower_left.height
                source: root.iniImagesUrl + "parts.png/window_left"
            }
            Image {
                id: body

                height: window.height - upper_left.height - lower_left.height
                source: root.iniImagesUrl + "parts.png/window_body"
                width: window.width - left.width - right.width
            }
            Image {
                id: right

                height: window.height - upper_left.height - lower_left.height
                source: root.iniImagesUrl + "parts.png/window_right"
            }
        }
        Row {
            Image {
                id: lower_left

                source: root.iniImagesUrl + "parts.png/window_lower_left"
            }
            Image {
                id: bottom

                source: root.iniImagesUrl + "parts.png/window_bottom"
                width: window.width - lower_left.width - lower_right.width
            }
            Image {
                id: lower_right

                source: root.iniImagesUrl + "parts.png/window_lower_right"
            }
        }
    }
}