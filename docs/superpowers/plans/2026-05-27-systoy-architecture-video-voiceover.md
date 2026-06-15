# SysToy Architecture Video Voiceover Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Chinese narration, smoother pacing, and lighter on-screen copy to the existing SysToy HyperFrames explainer video.

**Architecture:** Keep the current eight-scene HyperFrames structure, add one narration audio clip per scene, shorten or simplify on-screen explanatory copy, and retime the root composition so audio, visuals, and transitions feel aligned.

**Tech Stack:** HyperFrames, Kokoro TTS via `npx hyperframes tts`, HTML, CSS, GSAP

---

### Task 1: Voice Script And Audio Assets

**Files:**
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-01.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-02.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-03.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-04.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-05.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-06.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-07.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/sources/narration/scene-08.txt`
- Create: `projects/systoy-architecture_ppt169_20260527/audio/`

- [ ] Write scene-by-scene Chinese narration in a light but professional tone.
- [ ] Generate one `.wav` per scene with `zf_xiaobei` and a slightly brisk speed.
- [ ] Measure or inspect generated audio durations before retiming the project.

### Task 2: Scene Copy And Timing Refresh

**Files:**
- Modify: `projects/systoy-architecture_ppt169_20260527/index.html`
- Modify: `projects/systoy-architecture_ppt169_20260527/compositions/*.html`

- [ ] Simplify on-screen explanatory copy so subtitles become support text instead of full narration.
- [ ] Tighten scene-level entrance timing and reduce dead air between major beats.
- [ ] Add the scene narration audio clips to the root composition timeline with aligned starts and durations.

### Task 3: Verification And Final Draft Render

**Files:**
- Modify: `projects/systoy-architecture_ppt169_20260527/README.md`

- [ ] Re-run `npx hyperframes lint`, `validate`, and `inspect`.
- [ ] Render a new draft MP4 with narration.
- [ ] Summarize the new output path and remaining font/style caveats.
