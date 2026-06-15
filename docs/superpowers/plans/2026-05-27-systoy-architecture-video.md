# SysToy Architecture Video Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a HyperFrames project that renders a pure-visual video explaining the SysToy architecture and core mechanisms.

**Architecture:** Reuse the existing `projects/systoy-architecture_ppt169_20260527/` workspace as the dedicated HyperFrames project. Implement one root composition plus a small set of scene compositions covering overview, startup, syscall/trap, scheduling, FAT32/ELF loading, build pipeline, and phase timeline, all styled by a project-specific `DESIGN.md`.

**Tech Stack:** HyperFrames, HTML, CSS, GSAP, Node.js, FFmpeg/Chrome via HyperFrames CLI

---

### Task 1: Project Scaffold And Visual Identity

**Files:**
- Modify: `projects/systoy-architecture_ppt169_20260527/README.md`
- Create: `projects/systoy-architecture_ppt169_20260527/DESIGN.md`
- Create: `projects/systoy-architecture_ppt169_20260527/index.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/`

- [ ] Capture the current workspace state and inspect the existing project directory layout.
- [ ] Create `DESIGN.md` with the locked dark-tech palette, typography, and motion constraints from the approved video design spec.
- [ ] Initialize a root HyperFrames composition in `index.html` that owns the master timeline and references scene sub-compositions.
- [ ] Update the local project `README.md` with preview and render commands for the finished video project.

### Task 2: Core Scene Compositions

**Files:**
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-01-intro.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-02-architecture.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-03-startup.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-04-syscall.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-05-scheduling.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-06-loader.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-07-pipeline.html`
- Create: `projects/systoy-architecture_ppt169_20260527/compositions/scene-08-timeline.html`

- [ ] Implement the introduction and overall architecture scenes with readable Chinese copy and animated layer highlighting.
- [ ] Implement the startup and syscall/trap scenes with directional flow animation and labeled execution steps.
- [ ] Implement the scheduling/synchronization and FAT32/ELF loader scenes with contrastive and sequential motion.
- [ ] Implement the build pipeline and phase timeline scenes, then add a short closing state inside the final scene.

### Task 3: Verification And Render

**Files:**
- Modify: `projects/systoy-architecture_ppt169_20260527/index.html`
- Modify: `projects/systoy-architecture_ppt169_20260527/compositions/*.html`

- [ ] Run `npx hyperframes lint` in the project directory and fix structural issues.
- [ ] Run `npx hyperframes validate` and `npx hyperframes inspect`, then adjust layout or contrast issues until clean enough to render.
- [ ] Run `npx hyperframes render --quality draft` to produce a first-pass MP4 output.
- [ ] Summarize the generated files, remaining risks, and exact preview/render commands for the user.
