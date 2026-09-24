import QtQuick
import RhythmGameQml

Lr2SkinScreenWrapper {
    id: root
    required property ResultContext result
    screenKey: "result"
    resultContext: root.result
}
