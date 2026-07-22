# Dissertation Demonstration Video Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a review-ready bilingual script for a 2 minute 30 second dissertation demonstration video, correct academic titles in the bilingual acknowledgments, remove the premature AI voice-over claim, and rebuild both ACM PDFs.

**Architecture:** Keep the video deliverable in one versioned Markdown file beside the artifact documentation. Keep `thesis_en.md` and `thesis.md` as the canonical English and Chinese dissertation sources; regenerate LaTeX and PDF outputs through the existing `acm/prepare_acm.py` and `acm/build.ps1` pipeline. Do not insert a film URL until a hosted link exists.

**Tech Stack:** Markdown, PowerShell, Python 3, existing ACM LaTeX generator, Tectonic.

## Global Constraints

- The video duration target is approximately 2 minutes 30 seconds and must remain below 3 minutes.
- Use English narration with a Chinese reference translation.
- Show real execution of the WFC scene, S1/S2/S3 switching, one smoke benchmark, and concise result interpretation.
- Identify the smoke benchmark as an artifact demonstration run; do not present it as part of the formal dissertation dataset.
- Keep the hosted-video link out of both PDFs until the user supplies it.
- Use `Dr Rich Davidson`, `Dr Gary Ushaw`, and `Professor Graham Morgan` in English; use `Rich Davidson 博士`, `Gary Ushaw 博士`, and `Graham Morgan 教授` in Chinese.
- Remove the AI speech-synthesis claim while the video and voice-over do not yet exist.

---

### Task 1: Create the bilingual shot and narration document

**Files:**
- Create: `docs/dissertation/demonstration-video-script-bilingual.md`
- Reference: `GPUDrivenRendering/RUNME.md`
- Reference: `docs/superpowers/specs/2026-07-22-dissertation-video-design.md`

**Interfaces:**
- Consumes: the six-segment timeline and exact interactive/benchmark commands from the approved design.
- Produces: one Markdown document with recording settings, timestamped screen actions, 317-word English narration, Chinese reference translation, overlays, and a recording checklist.

- [ ] **Step 1: Write the six narration segments**

Use these exact segment boundaries and topics:

```text
0:00–0:10  research question and title
0:10–0:35  seeded simplified WFC generation and configuration
0:35–1:10  reader-facing definitions of S1, S2, and S3
1:10–1:40  smoke benchmark and CSV evidence
1:40–2:15  scale transition and chunk-sensitive CPU/GPU exchange
2:15–2:30  contribution and closing card
```

The English narration must contain 317 words and use the dissertation's bounded claims: grid size 128 is the first tested scale where every matched S1/S2 configuration favours S2 for CPU recording time; S2 usually has a longer GPU span; the S2/S3 exchange depends on chunk granularity.

- [ ] **Step 2: Add exact capture commands and on-screen labels**

Include the documented interactive command:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -gridsize 128 -chunksize 8 -density 50 -seed 42 -scheme 1
```

Include the documented smoke benchmark command:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -benchmark -gridsize 128 -chunksize 8 -density 50 -scheme 2 -seed 42 `
  -warmupframes 10 -recordframes 30 -output video_demo_scheme2.csv
```

Use these reader-facing overlay labels:

```text
S1 — CPU culling + direct draws
S2 — CPU culling + multi-draw indirect
S3 — compute culling + compute-generated indirect records
```

- [ ] **Step 3: Verify the script document**

Run:

```powershell
rg -n "0:00–0:10|0:10–0:35|0:35–1:10|1:10–1:40|1:40–2:15|2:15–2:30|video_demo_scheme2.csv|S1 —|S2 —|S3 —" docs/dissertation/demonstration-video-script-bilingual.md
rg -n -i "T[B]D|T[O]DO|<URL>|\[URL\]" docs/dissertation/demonstration-video-script-bilingual.md
```

Expected: all six time ranges, the output filename, and all three labels are present; the second command returns no matches.

- [ ] **Step 4: Commit the video script**

```powershell
git add docs/dissertation/demonstration-video-script-bilingual.md
git commit -m "docs: add dissertation demonstration script"
```

### Task 2: Correct acknowledgments and the current AI declaration

**Files:**
- Modify: `../thesis_en.md`, Acknowledgments and Declaration on the Use of Generative AI
- Modify: `../thesis.md`, Acknowledgments and Declaration on the Use of Generative AI

**Interfaces:**
- Consumes: the existing bilingual end matter.
- Produces: synchronized academic titles and a declaration limited to AI uses that have already occurred.

- [ ] **Step 1: Update the English acknowledgments**

Replace `my supervisor, Rich Davidson` with `my supervisor, Dr Rich Davidson`, and replace `Gary Ushaw and Professor Graham Morgan` with `Dr Gary Ushaw and Professor Graham Morgan`.

- [ ] **Step 2: Update the Chinese acknowledgments**

Replace `论文指导老师 Rich Davidson` with `论文指导老师 Rich Davidson 博士`, and replace `Gary Ushaw 和 Graham Morgan 教授` with `Gary Ushaw 博士和 Graham Morgan 教授`.

- [ ] **Step 3: Remove the premature voice-over claim from both declarations**

Delete this English sentence:

```text
The demonstration-video voice-over was produced using AI speech synthesis.
```

Delete this Chinese sentence:

```text
演示视频中的旁白由 AI speech synthesis 生成。
```

Do not add a new claim about the narration script or speech service until the user decides to use the draft and records the video.

- [ ] **Step 4: Verify bilingual synchronization**

Run:

```powershell
rg -n "Dr Rich Davidson|Dr Gary Ushaw|Professor Graham Morgan|voice-over|speech synthesis" ..\thesis_en.md
rg -n "Rich Davidson 博士|Gary Ushaw 博士|Graham Morgan 教授|演示视频中的旁白|speech synthesis" ..\thesis.md
```

Expected: all six academic-title forms are present; no voice-over or speech-synthesis claim remains.

### Task 3: Regenerate and verify the ACM deliverables

**Files:**
- Regenerate: `../acm/main.tex`
- Regenerate: `../acm/main_with_ai_declaration.tex`
- Regenerate: `../output/pdf/dissertation_acm.pdf`
- Regenerate: `../output/pdf/dissertation_acm_with_ai_declaration.pdf`

**Interfaces:**
- Consumes: updated `../thesis_en.md` and existing `../acm/prepare_acm.py`.
- Produces: the ordinary PDF with acknowledgments and the disclosure PDF with acknowledgments plus the updated declaration.

- [ ] **Step 1: Run source and generator checks**

```powershell
python -m py_compile .\prepare_acm.py
python .\prepare_acm.py --output main.tex
python .\prepare_acm.py --output main_with_ai_declaration.tex --include-ai-declaration
```

Expected: all commands exit with code 0.

- [ ] **Step 2: Build both PDF variants**

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Expected: Tectonic builds both documents and copies them to `../output/pdf/`.

- [ ] **Step 3: Check LaTeX diagnostics and generated text**

```powershell
rg -n -i "overfull|undefined references|undefined citations" main.log main_with_ai_declaration.log
rg -n "Dr Rich Davidson|Dr Gary Ushaw|Professor Graham Morgan|voice-over|speech synthesis" main.tex main_with_ai_declaration.tex
```

Expected: the log scan returns no matches; both LaTeX files contain the corrected titles; neither contains the removed voice-over claim.

- [ ] **Step 4: Inspect both PDFs**

Render all pages of both PDFs and inspect the acknowledgments/declaration page at original resolution. Confirm that the ordinary version omits the declaration, the disclosure version includes it before References, and neither version has clipped or overlapping end matter.

- [ ] **Step 5: Report the deferred film-link action**

State that the title-page film link remains unchanged because no hosted URL exists. Once the user supplies the URL, add an ACM-compatible subtitle line labelled `Project demonstration video` to both generated variants.
