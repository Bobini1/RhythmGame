import RhythmGameQml
import QtQuick

Row {
    id: row

    property var current

    NoteImage {
        active: row.current instanceof ChartData && row.current.difficulty === 1
        name: "beginner"
        playLevel: row.current instanceof ChartData ? row.current.playLevel : 0
    }
    NoteImage {
        active: row.current instanceof ChartData && row.current.difficulty === 2
        name: "normal"
        playLevel: row.current instanceof ChartData ? row.current.playLevel : 0
    }
    NoteImage {
        active: row.current instanceof ChartData && row.current.difficulty === 3
        name: "hyper"
        playLevel: row.current instanceof ChartData ? row.current.playLevel : 0
    }
    NoteImage {
        active: row.current instanceof ChartData && row.current.difficulty === 4
        name: "another"
        playLevel: row.current instanceof ChartData ? row.current.playLevel : 0
    }
    NoteImage {
        active: row.current instanceof ChartData && row.current.difficulty === 5
        name: "insane"
        playLevel: row.current instanceof ChartData ? row.current.playLevel : 0
    }
}
