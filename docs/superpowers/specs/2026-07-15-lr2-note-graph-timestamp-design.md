# LR2 Note Graph Timestamp Design

## Goal

Make LR2/Beatoraja judgement and early/late distribution graphs place colored judgement bars in the same one-second bucket as their corresponding gray note bars.

## Change

In `Lr2NoteChartRenderer.qml`, bucket note-removing judgement events by the note's scheduled timestamp:

```text
note timestamp = hit.offsetFromStart - hit.points.deviation
```

Use the event timestamp as a defensive fallback when the deviation is unavailable or the derived timestamp is invalid. Apply this shared calculation to both judgement and early/late graph types. Do not change replay event storage, timing classification, skin files, or graph rendering.

Map that note timestamp to the same persisted histogram coordinate system used by the gray bars:

```text
bucket = floor(note timestamp / chart length * histogram bucket count)
```

Clamp the result to the histogram's final bucket, matching chart histogram generation for notes at the chart endpoint. Fall back to one-second bucketing only when histogram length data is unavailable. This keeps the change local and avoids requiring existing chart histograms to be regenerated.

Long-note density semantics are outside this change. The gray histogram may still represent an active long note across multiple buckets while replay judgements represent discrete events.

## Verification

Build the affected QML module and run the existing focused LR2 test slice. Do not add new regression cases for this change.
