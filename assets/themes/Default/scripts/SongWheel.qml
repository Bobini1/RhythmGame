import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic

Pane {
    RowLayout {
        anchors.fill: parent

        PathView {
            id: pathView

            function decrementCurrentIndex() {
                currentIndex = (currentIndex - 1) % count;
            }
            function incrementCurrentIndex() {
                currentIndex = (currentIndex + 1) % count;
            }
            function open(item) {
                if (item instanceof ChartData) {
                    console.info("Opening chart " + item.path);
                    globalRoot.openChart(item.path);
                } else {
                    model = SongFolderFactory.open(item);
                }
            }

            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width / 2
            dragMargin: 200
            focus: true
            highlightMoveDuration: 100
            model: SongFolderFactory.open("/")
            pathItemCount: 20

            // selected item should be in the middle of the arc
            preferredHighlightBegin: 0.5
            preferredHighlightEnd: 0.5
            snapMode: PathView.SnapToItem

            delegate: Text {
                // selected item should be yellow
                color: PathView.isCurrentItem ? "yellow" : "white"
                text: display instanceof ChartData ? display.title : display

                Component.onCompleted: {
                    if (display instanceof ChartData && display.keymode === ChartData.Keymode.K14) {
                        color = "red";
                    }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        pathView.open(display);
                        pathView.currentIndex = index;
                    }
                }
            }
            path: Path {
                startX: 120
                startY: -100

                PathLine {
                    x: 0
                    y: pathView.height + 100
                }
            }

            Keys.onDownPressed: {
                incrementCurrentIndex();
            }
            Keys.onLeftPressed: {
                if (model.parentFolder) {
                    model = SongFolderFactory.open(model.parentFolder);
                } else {
                    sceneStack.pop();
                }
            }
            Keys.onReturnPressed: {
                open(model.at(currentIndex));
            }
            Keys.onRightPressed: {
                open(model.at(currentIndex));
            }
            Keys.onUpPressed: {
                decrementCurrentIndex();
            }
            onModelChanged: {
                model.minimumAmount = pathItemCount;
            }

            MouseArea {
                id: mouse

                anchors.fill: parent

                onWheel: {
                    if (wheel.angleDelta.y > 0)
                        pathView.decrementCurrentIndex();
                    else
                        pathView.incrementCurrentIndex();
                }
            }
        }
    }
}