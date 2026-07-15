# LR2 Note Graph Timestamp Design

## Goal

Make LR2/Beatoraja judgement and early/late distribution graphs place colored judgement bars in the same one-second bucket as their corresponding gray note bars.

## Change

In `Lr2NoteChartRenderer.qml`, bucket note-removing judgement events by the note's scheduled timestamp:

```text
note timestamp = hit.offsetFromStart - hit.points.deviation
```

Use the event timestamp as a defensive fallback when the deviation is unavailable or the derived timestamp is invalid. Apply this shared calculation to both judgement and early/late graph types. Do not change replay event storage, timing classification, skin files, or graph rendering.

## Verification

Build the affected QML module and run the existing focused LR2 test slice. Do not add new regression cases for this change.
