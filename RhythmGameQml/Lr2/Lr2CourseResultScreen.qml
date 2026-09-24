import QtQuick
import RhythmGameQml

Lr2SkinScreenWrapper {
    id: root
    required property CourseResultContext result
    screenKey: "courseResult"
    resultContext: root.result
}
