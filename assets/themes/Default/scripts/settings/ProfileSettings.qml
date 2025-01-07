import QtQuick
import QtQuick.Controls.Basic
import "settingsProperties"
import QtQml.Models

Item {
    ScrollView {
        id: scrollView
        clip: true
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            bottom: parent.bottom
        }
        width: Math.min(parent.width / 2, 600)
        padding: 1

        ListView {
            id: list
            width: 600

            spacing: 10
            bottomMargin: 10
            topMargin: 10
            leftMargin: 10
            rightMargin: 10

            Frame {
                anchors.fill: list
                z: -1
            }

            model: {
                
            }
        }
    }
}