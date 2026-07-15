# LR2 Note Graph Timestamp Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bucket LR2 judgement and early/late graph entries at their corresponding note timestamps using the gray histogram's coordinate system.

**Architecture:** Keep replay data unchanged and adjust only the shared graph renderer. Derive the note timestamp from the event timestamp minus its judgement deviation, then map it through the persisted histogram's chart-length-normalized bucket transform. Retain one-second bucketing only as a defensive fallback when histogram dimensions are unavailable.

**Tech Stack:** Qt 6, QML

## Global Constraints

- Apply the change to both judgement and early/late graph types through their shared event-bucketing path.
- Do not add regression tests or run verification, per user request.
- Do not change replay storage, timing classification, or skin files.
- Do not change long-note density semantics.

---

### Task 1: Align replay events with histogram buckets

**Files:**
- Modify: `RhythmGameQml/Lr2/Lr2NoteChartRenderer.qml:310-330`

**Interfaces:**
- Consumes: `HitEvent.offsetFromStart` and `HitEvent.points.deviation`
- Produces: `replayBucketForNoteOffset(offset)` and histogram-aligned selection in `appendReplayDataHit(...)`

- [x] **Step 1: Add the shared histogram bucket transform**

Add this helper immediately before `appendReplayDataHit(...)`:

```qml
function replayBucketForNoteOffset(offset: var) : var {
    let histogramBucketCount = root.chartSnapshot.normalDensityData.length;
    let chartLength = Number(root.chartSnapshot.length || 0);
    if (histogramBucketCount > 0 && chartLength > 0) {
        let bucket = Math.floor(offset / chartLength * histogramBucketCount);
        return Math.max(0, Math.min(histogramBucketCount - 1, bucket));
    }
    return Math.max(0, Math.floor(offset / 1000000000));
}
```

- [x] **Step 2: Derive the note timestamp and select its histogram bucket**

Keep the existing note-time derivation and replace the one-second bucket calculation with:

```qml
let eventOffset = root.replayEventOffset(hit, 0);
if (eventOffset < 0) {
    return;
}
let offset = eventOffset - root.hitDeviationNanos(hit);
if (!isFinite(offset) || offset < 0) {
    offset = eventOffset;
}
let second = root.replayBucketForNoteOffset(offset);
```

Leave judgement classification and replay caching unchanged so both graph types share the new transform.

- [x] **Step 3: Review the focused diff**

Run:

```powershell
git diff --check -- RhythmGameQml/Lr2/Lr2NoteChartRenderer.qml
git diff -- RhythmGameQml/Lr2/Lr2NoteChartRenderer.qml
```

Expected: only the bucket helper and its call replace the literal one-second calculation; no tests are run.
