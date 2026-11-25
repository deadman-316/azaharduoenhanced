# Frame pacing test procedure, build 0.1.2

Purpose: find out what the display actually receives, and what the emulation thread is
doing during each stutter. Game: **Super Mario 3D Land**, warm shader cache (play the test
levels once beforehand).

## Every run

- Vulkan, same GPU driver, same internal resolution, async shader compilation off.
- Launch, get into World 1-1, play it to the flagpole **twice in a row**, then exit with
  Azahar's own exit option. The first pass has cold CPU JIT code for that level, the second
  has it warm.
- While playing, note roughly when you see a stutter ("about 40 seconds in, at the first
  pipe"). A few notes per run are enough.
- About two minutes per run.

## Runs

| Run | Setup | Question it answers |
| --- | --- | --- |
| P1 | Both screens at 120 Hz, normal dual-screen layout, load by in-game save | Baseline |
| P2 | Both screens at 60 Hz (Android display settings), otherwise as P1 | Does the 8/25 ms cadence disappear at 60 Hz? |
| P3 | 120 Hz, bottom screen disabled or single-screen layout, otherwise as P1 | Does the second swapchain disturb pacing? |
| P4 | As P1 but load a save state instead of the in-game save | Do save states behave differently? |

If a setting is not available (for example no 60 Hz option), skip that run and say so.

## What to send back

Zip the new folders under `log/duo/` and say which folder is which run, with your stutter
notes.

## How it will be read

- `present.csv`, `displayed` rows per window: the interval between consecutive display
  times is what the screen showed. Steady 16.67 ms means even pacing; a mix of 8.3 and
  25 ms means the uneven cadence is visible; a gap means a repeated frame.
- `present.csv`, `present` rows: where presentation waits (queue, acquire, present call).
- `frames.csv`: for each long frame, which category the time went to. Time not explained by
  any category is CPU emulation, which is where JIT translation would show.
- Comparing the first and second pass of the level separates JIT warm-up from steady-state
  cost.
