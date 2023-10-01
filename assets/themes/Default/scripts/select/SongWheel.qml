import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic

Pane {
    id: root

    readonly property bool active: StackView.status === StackView.Active
    readonly property string imagesUrl: rootUrl + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

    RowLayout {
        anchors.fill: parent

        PathView {
            id: pathView

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
            pathItemCount: 15

            // selected item should be in the middle of the arc
            preferredHighlightBegin: 0.5
            preferredHighlightEnd: 0.5
            snapMode: PathView.SnapToItem

            delegate: Loader {
                property bool isCurrentItem: PathView.isCurrentItem

                source: display instanceof ChartData ? "Chart.qml" : "Folder.qml"
            }
            path: Path {
                startX: 250
                startY: -100

                PathLine {
                    x: 0
                    y: pathView.height + 100
                }
            }

            Component.onCompleted: {
                positionViewAtIndex(0, PathView.Center);
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

                onWheel: wheel => {
                    if (wheel.angleDelta.y > 0)
                        pathView.decrementCurrentIndex();
                    else
                        pathView.incrementCurrentIndex();
                }
            }
        }
    }
    Shortcut {
        enabled: active
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
}