import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    model: ObjectModel {
        Row {
            height: thickness.height

            Text {
                anchors.verticalCenter: thickness.verticalCenter
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                text: "Note Thickness"
                verticalAlignment: Text.AlignVCenter
                width: 100
            }
            Slider {
                id: thickness

                from: 0
                to: 100
                value: root.vars.thickness

                onMoved: {
                    root.vars.thickness = value;
                    value = Qt.binding(() => root.vars.thickness);
                }
            }
        }
        ImageSelection {
            id: notes

            propertyId: "notes"
        }
        ImageSelection {
            id: mine

            propertyId: "mine"
        }
        ImageSelection {
            id: keybeam

            propertyId: "keybeam"
        }
        ImageSelection {
            id: bomb

            propertyId: "bomb"
        }
        ImageSelection {
            id: glow

            propertyId: "glow"
        }
    }
}