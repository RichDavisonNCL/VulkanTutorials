# CSC8599 Dissertation Demonstration Video Design

## Purpose and constraints

The video will demonstrate the working Vulkan project and communicate the dissertation's research question in no more than three minutes. The target duration is approximately 2 minutes 30 seconds. The primary audience is an assessor who may not know the internal S1, S2, and S3 labels before watching. The recording should therefore introduce each path in plain language before showing its label.

The video will use English voice-over with an English script and a Chinese reference translation. It will show real project execution, while short captions and existing dissertation figures will explain what the execution demonstrates. Long generation or benchmark waits will be removed through cuts rather than presented in real time.

## Chosen narrative

The video will use a demonstration-led research narrative. It will first state the problem, then generate a WFC scene, switch among the three rendering paths over the same scene, run one short benchmark configuration, and connect that benchmark to two concise findings from the dissertation. This format preserves evidence that the artifact works while giving the viewer enough context to understand the contribution.

The video will not attempt to reproduce the full experimental matrix. It will not explain every metric boundary, limitation, local-update result, or provenance field. Those details remain in the dissertation and artifact documentation.

## Timeline and shot design

| Time | Visual | Narration purpose |
|---|---|---|
| 0:00–0:10 | Title card followed by a clean view of the generated scene | Identify the project and state the research question in one sentence. |
| 0:10–0:35 | Launch the executable with grid 128, chunk 8, preset 50, seed 42; show scene generation and the relevant configuration panel | Explain that a seeded simplified WFC workload creates a reproducible modular scene and that the same scene is shared by all paths. |
| 0:35–1:10 | Press `1`, `2`, and `3` in sequence while keeping the camera fixed; show an on-screen caption for each path | Introduce CPU culling with direct draws, CPU culling with MDI, and compute culling with compute-generated indirect records. Emphasise that the image remains consistent while work allocation changes. |
| 1:10–1:40 | Show the smoke benchmark command, a short portion of execution, and the resulting CSV header/rows | Demonstrate that the artifact records configuration metadata together with CPU and GPU measurements. Cut waiting time and keep the command readable. |
| 1:40–2:15 | Show cropped, readable views of Figures 5.1 and 5.2, with one highlighted observation on each | Explain the S1/S2 scale transition and the chunk-sensitive CPU/GPU exchange observed for the compute-driven path. Avoid claiming that one path is universally preferable. |
| 2:15–2:30 | Return to the rendered scene, then display a closing card | Summarise the contribution: a controlled Vulkan framework that relates rendering-path performance to scale, chunk granularity, and generated scene content. |

## Demonstration configuration

The interactive segment will use the documented command below because it is large enough to show a meaningful modular scene while remaining quick to launch:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -gridsize 128 -chunksize 8 -density 50 -seed 42 -scheme 1
```

The three paths will be selected with the existing `1`, `2`, and `3` controls. The camera will remain fixed while switching paths so the viewer can see that the scene and viewpoint are shared.

The benchmark segment will use the documented smoke command with 10 warm-up frames and 30 recorded frames:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -benchmark -gridsize 128 -chunksize 8 -density 50 -scheme 2 -seed 42 `
  -warmupframes 10 -recordframes 30 -output video_demo_scheme2.csv
```

The video will identify this as an artifact demonstration run rather than part of the submitted dissertation dataset. The CSV view will show metadata and a small number of frame rows without dwelling on every column.

## Caption and voice-over rules

The first occurrence of each label will include a reader-facing name:

- S1 — CPU culling and direct draws
- S2 — CPU culling and multi-draw indirect
- S3 — compute culling and compute-generated indirect records

The English voice-over target is 280–320 words, spoken at a calm pace. Sentences should describe only what is visible or what the immediately following result establishes. The script will use cautious result language consistent with the dissertation, including “in the tested configurations” and “within the recorded interval” where needed. Technical details such as the fixed `2N` layout will appear once, during the S3 or results segment.

The video will use burned-in English captions or an uploaded subtitle track. The Chinese text will serve as a recording and review aid; it does not need to appear on screen unless desired.

## Capture and edit requirements

Record the application and terminal as separate clips at 1920×1080. Use 30 frames per second unless the application capture benefits visibly from 60. Keep configuration values, path labels, and benchmark output large enough to read on a laptop display. Hide unrelated windows, notifications, personal paths, and account information.

Scene generation may be trimmed with a direct cut. The path-switching segment should retain enough continuous footage to show that the executable remains running. Benchmark startup and completion should both be visible, but idle waiting may be removed. Figures should be exported directly from the dissertation assets instead of captured from the PDF viewer.

The closing export should use H.264 video and AAC audio in an MP4 container. An unlisted or private-access YouTube link is suitable if assessors can open it without requesting permission.

## Dissertation and disclosure integration

No video URL will be inserted before the hosted film exists. Once available, the URL will be placed beneath the dissertation title in both PDF variants using an ACM-compatible subtitle line labelled “Project demonstration video”.

The acknowledgments will identify Dr Rich Davidson and Dr Gary Ushaw by title and retain Professor Graham Morgan's title. Because the video has not yet been recorded, the current claim that its voice-over was produced by AI speech synthesis will be removed from the disclosure. If AI speech synthesis is later used, the declaration will be updated in the past tense and will name the service used. If the author records the narration, no AI voice-over statement will be added.

## Deliverables after approval

The implementation stage will produce one bilingual shot-and-script document containing timestamps, screen actions, English voice-over, and Chinese reference text. It will also update the bilingual acknowledgments and AI declaration, regenerate both ACM LaTeX variants, and verify the PDFs. Adding the film link will remain a separate small update after the user supplies the hosted URL.
