import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    height: 200

    model: ObjectModel {
        Row {
            height: verticalGauge.height

            Text {
                anchors.verticalCenter: verticalGauge.verticalCenter
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                text: "Vertical Gauge"
                verticalAlignment: Text.AlignVCenter
                width: 100
            }
            CheckBox {
                id: verticalGauge

                checked: root.vars.verticalGauge

                onCheckedChanged: {
                    root.vars.verticalGauge = checked;
                    checked = Qt.binding(() => root.vars.verticalGauge);
                }
            }
        }
        ImageSelection {
            id: gauge

            propertyId: "gauge"
        }
    }
}