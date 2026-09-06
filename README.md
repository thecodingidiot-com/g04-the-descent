# g04-the-descent

Companion repository for **g04 — The Descent** at
[thecodingidiot.com](https://thecodingidiot.com) — the raycaster-era
capstone of Part III, The Rendering Journey.

The same maze
[g02c](https://github.com/thecodingidiot-com/g02c-the-maze) built, in
the same file, rendered from inside it with
[r03](https://github.com/thecodingidiot-com/r03-the-raycaster)'s
raycaster. You already know the shape of this place. It will not help
as much as you expect.

---

## Follow my journey

```bash
git clone https://github.com/thecodingidiot-com/g04-the-descent.git
cp g04-the-descent/test.sh ~/g04-practice/
cd ~/g04-practice
bash test.sh
```

All 29 checks must pass before the chapter is complete.

---

## Follow your journey

Building it independently? Here is the brief.

r03 and r04 are tech demos: they draw walls and nothing else, they walk
straight through those walls, and they never ask whether anything is in
front of anything. This chapter pays all of that off, and every piece of
it is something an earlier chapter already built.

- **Collision with sliding.** r03 and r04 both walk through walls, by
  design. Resolve the two axes separately so pressing into a wall slides
  along it, the same resolution
  [g02c](https://github.com/thecodingidiot-com/g02c-the-maze) uses.
- **A per-column depth buffer.** Neither r03 nor r04 keeps one: they
  compute a column's distance, turn it into a wall height and forget it.
  That is enough while walls are the only thing on screen. The moment a
  sprite exists, "is something nearer here?" has to be answerable per
  column, and the cheapest place to answer it is the pass that already
  knew.
- **Billboard enemies.** An enemy is a flat sprite that has to agree
  with the walls about how far away it is — which is exactly what
  [r01](https://github.com/thecodingidiot-com/r01-the-scaler)'s
  `scaler_project()` computes. The scaler era comes back to solve a
  raycaster problem, and the depth buffer arbitrates per column.
- **Hitscan shooting.** Not a projectile. `raycaster_cast_dir()` is the
  renderer's own DDA aimed by hand instead of by a screen column, so
  the chapter's rendering primitive becomes its combat primitive.
- **Enemies that hunt.** Line of sight is that same cast: if the first
  wall along the line between us is further than you are, I can see you.
  No second opinion about geometry, no approximation.

The core loop is **escape**, not clear-the-level. Find the way out of a
maze you are never shown a map of. Enemies are pressure that makes
standing still to think expensive; killing all of them wins nothing.

| File | Contents |
| --- | --- |
| `main.c` | the loop, input, movement with collision |
| `map.c` / `map.h` | g02c's maze format, allocated rather than fixed |
| `vec2.c` / `camera.c` | unchanged from r03 |
| `raycaster.c` / `.h` | r03's DDA, refactored so a ray can be aimed |
| `scaler.c` / `.h` | r01's projection, plus the occlusion rule |
| `enemy.c` / `.h` | sight, pursuit, firing |
| `weapon.c` / `.h` | hitscan |
| `render.c` / `.h` | walls, the depth buffer, sorted billboards, HUD |

---

## Building the solution

```bash
cd solution
make
./descent ../fixtures/maze1.txt
```

`make` builds `libtci` and `libtciutil` first if their archives are not
there — they are build artefacts, so the repository ships their source.

Controls: Left/Right or `h`/`l` to turn, Up/Down or `k`/`j` to move,
**Space** to fire, Escape or `q` to quit.

---

## The maze is g02c's, unchanged

`fixtures/maze1.txt` is byte-identical to g02c's. That is the point of
the chapter: the same 75 × 57 grid, the same nine rooms, the same
doorways — read by a renderer that shows you one corridor at a time
instead of one room from above.

Two things follow from reusing it, and both are real:

**It does not fit r03's map.** r03 held its grid in `char grid[32][33]`,
sized for the small hand-written rooms that chapter needed. 75 × 57 does
not fit, so this loader allocates.

**A cell means something different.** g02c's rooms are 25 × 19 cells
because that is what fills an 800 × 608 screen from above. Rendered at
one unit tall, a wall twelve cells away is a sliver near the horizon and
a room reads as an empty plain. `WALL_HEIGHT` exists for that reason: a
top-down renderer measures a room in screens, a first-person one
measures it in wall heights, and the same file is a corridor to one and
a field to the other.

---

## What the tester checks

**Build** — compiles and links with zero warnings under `-Werror`.

**A headless logic tester** — everything except `render.c` and `main.c`,
linked with `libtci` alone and no SDL2, asserting:

- g02c's maze loads at 75 × 57, and has exactly one exit
- **the centre column and the forward cast agree** — if the renderer and
  the weapon ever disagree, the crosshair points somewhere the shot does
  not go, which is not the sort of bug anyone reports
- a flat wall reports the same perpendicular distance at the left,
  centre and right columns — r03's no-fisheye property, re-asserted here
- an enemy across an open room is visible; one with a wall between is not
- a shot hits the enemy ahead and misses the one behind the camera
- a nearer sprite projects larger; one behind the camera is not visible
- **occlusion is per column** — a pillar in one column hides the sprite
  there while its neighbour still draws it

That last group tests `sprite_column_visible()`, which was pulled out of
the renderer specifically so the occlusion rule could be asserted
without a window.

**`descent`** — runs its event loop for two seconds under
`SDL_VIDEODRIVER=dummy`. The renderer falls back to software when no
accelerated one exists, so this exercises a real renderer rather than
passing because a NULL one made every draw call a no-op.

---

## License

MIT License. See [LICENSE](LICENSE).
