#!/bin/bash
# g04 — The Descent / test.sh
#
# Builds the real game, then asserts the logic headlessly. Everything
# except render.c and main.c is compiled and linked with libtci alone —
# no -lSDL2, no display — because none of it calls an SDL function.
# The one thing render.c does that is worth asserting, the occlusion
# rule, was factored into sprite_column_visible() precisely so it could
# be tested without a window.
#
# Copy this file and fixtures/ into your working directory, then run:
#
#   bash test.sh

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES="${SCRIPT_DIR}/fixtures"

if [[ ! -t 1 ]]; then
    C_GREEN=""; C_RED=""; C_BOLD=""; C_RESET=""
else
    C_GREEN="\033[0;32m"; C_RED="\033[0;31m"; C_BOLD="\033[1m"; C_RESET="\033[0m"
fi

pass_count=0
fail_count=0
WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

hr() { echo "────────────────────────────────────────────────────────────────"; }
pass() { printf "  ${C_GREEN}PASS${C_RESET}  %s\n" "$1"; pass_count=$((pass_count + 1)); }
fail() {
    printf "  ${C_RED}FAIL${C_RESET}  %s\n" "$1"
    [[ -n "${2:-}" ]] && echo "        $2"
    fail_count=$((fail_count + 1))
}

hr; echo "  g04 — The Descent / test.sh"; hr

echo "Building..."
build_log=$(make re 2>&1)
if [[ "$?" -ne 0 ]]; then
    fail "build succeeds" "$build_log"; exit 1
fi
pass "build succeeds"
if echo "$build_log" | grep -qi "warning"; then
    fail "build produces no warnings" "$(echo "$build_log" | grep -i warning)"
else
    pass "build produces no warnings"
fi
[[ -x ./descent ]] && pass "descent binary exists" || fail "descent binary exists"

if [[ ! -f "${FIXTURES}/maze1.txt" ]]; then
    fail "fixtures/maze1.txt found" "keep the g04-the-descent clone alongside your working directory"
    exit 1
fi
cp "${FIXTURES}/maze1.txt" "$WORK_DIR/maze1.txt"

cat > "$WORK_DIR/test_logic.c" <<'TESTC'
#include <math.h>
#include <stdio.h>
#include "camera.h"
#include "enemy.h"
#include "map.h"
#include "raycaster.h"
#include "scaler.h"
#include "weapon.h"

static int  g_pass = 0;
static int  g_fail = 0;

static void check(char const *label, int got, int want)
{
    if (got == want) { printf("PASS  %s (got %d)\n", label, got); g_pass++; }
    else { printf("FAIL  %s (got %d, want %d)\n", label, got, want); g_fail++; }
}

static void check_near(char const *label, float got, float want, float eps)
{
    if (fabsf(got - want) <= eps) { printf("PASS  %s (got %.4f)\n", label, got); g_pass++; }
    else { printf("FAIL  %s (got %.4f, want %.4f)\n", label, got, want); g_fail++; }
}

int main(void)
{
    t_map       map;
    t_camera    cam;
    t_enemy     list[MAX_ENEMIES];
    t_hit       a;
    t_hit       b;

    check("g02c's maze loads", map_load(&map, "maze1.txt"), 1);
    check("it is 75 wide", map.width, 75);
    check("it is 57 tall", map.height, 57);
    check("out of bounds is a wall", map_is_wall(&map, -1, 0), 1);
    check("the start cell is not a wall",
        map_is_wall(&map, (int)map.start_pos.x, (int)map.start_pos.y), 0);

    /* There has to be a way out, or the game is unwinnable and every
     * other assertion is beside the point. */
    {
        int x = 0;
        int y;
        int exits = 0;

        while (x < map.width) {
            y = 0;
            while (y < map.height) {
                if (map_is_exit(&map, x, y))
                    exits++;
                y++;
            }
            x++;
        }
        check("the maze has exactly one exit", exits, 1);
    }

    camera_init(&cam, map.start_pos.x, map.start_pos.y, map.start_angle);

    /* The weapon and the renderer must be the SAME cast. If these ever
     * disagree, the crosshair is pointing somewhere the shot does not
     * go, which is the sort of bug nobody reports as a bug. */
    a = raycaster_cast(&cam, &map, WINDOW_W / 2);
    b = raycaster_cast_dir(&cam, &map, cam.forward);
    check_near("centre column and forward cast agree", a.perp_dist, b.perp_dist, 0.05f);

    /* Inherited from r03 and worth re-asserting here: a flat wall
     * reports the same perpendicular distance for every column, which
     * is what "no fisheye" actually means. */
    {
        t_map       room;
        int         x;
        int         y;
        char const *rows[5] = {"11111", "10001", "10001", "10001", "11111"};
        static char cells[5 * 5];

        room.width = 5; room.height = 5; room.tiles = cells;
        y = 0;
        while (y < 5) {
            x = 0;
            while (x < 5) { cells[y * 5 + x] = rows[y][x] == '1' ? '#' : '.'; x++; }
            y++;
        }
        camera_init(&cam, 2.5f, 2.5f, 0.0f);
        check_near("flat wall, left column", raycaster_cast(&cam, &room, 0).perp_dist, 1.5f, 0.02f);
        check_near("flat wall, centre column", raycaster_cast(&cam, &room, WINDOW_W / 2).perp_dist, 1.5f, 0.02f);
        check_near("flat wall, right column", raycaster_cast(&cam, &room, WINDOW_W - 1).perp_dist, 1.5f, 0.02f);

        /* Line of sight uses that same cast. An enemy across an open
         * room is visible; put a wall BETWEEN them and it is not.
         *
         * The wall has to be between. An enemy standing inside one can
         * still see out, because the DDA tests the cell it steps into
         * and never the cell it starts in -- true of the renderer too,
         * and harmless only because enemy_reset() refuses to spawn
         * anything in a wall. */
        {
            t_enemy     e;
            t_map       split;
            static char scells[7 * 5];
            char const *srows[5] = {"1111111", "1001001", "1001001",
                                    "1001001", "1111111"};
            int         sx;
            int         sy;
            t_camera    scam;

            e.pos.x = 3.5f; e.pos.y = 2.5f; e.alive = 1;
            check("an enemy across an open room is visible",
                enemy_can_see(&e, &cam, &room), 1);

            split.width = 7; split.height = 5; split.tiles = scells;
            sy = 0;
            while (sy < 5) {
                sx = 0;
                while (sx < 7) {
                    scells[sy * 7 + sx] = srows[sy][sx] == '1' ? '#' : '.';
                    sx++;
                }
                sy++;
            }
            camera_init(&scam, 1.5f, 2.5f, 0.0f);
            e.pos.x = 5.5f; e.pos.y = 2.5f;
            check("an enemy behind a wall is not visible",
                enemy_can_see(&e, &scam, &split), 0);
        }

        /* The weapon: an enemy dead ahead is hit, one behind is not. */
        {
            int i = 0;

            while (i < MAX_ENEMIES) { list[i].alive = 0; i++; }
            list[0].alive = 1; list[0].health = 1;
            list[0].pos.x = 3.2f; list[0].pos.y = 2.5f;
            check("a shot hits the enemy ahead", weapon_fire(&cam, &room, list), 0);
            check("and that enemy is now dead", list[0].alive, 0);

            list[0].alive = 1; list[0].health = 1;
            list[0].pos.x = 1.5f; list[0].pos.y = 2.5f;   /* behind us */
            check("a shot misses an enemy behind the camera",
                weapon_fire(&cam, &room, list), -1);
        }

        /* The scaler: nearer projects larger, behind the camera is not
         * visible at all. */
        {
            t_projection near_p;
            t_projection far_p;
            t_projection behind;
            t_vec2       p;

            p.x = 3.0f; p.y = 2.5f; near_p = scaler_project(&cam, p);
            p.x = 3.9f; p.y = 2.5f; far_p = scaler_project(&cam, p);
            p.x = 1.0f; p.y = 2.5f; behind = scaler_project(&cam, p);
            check("a nearer sprite projects larger", near_p.size > far_p.size, 1);
            check("a sprite behind the camera is not visible", behind.visible, 0);
        }
    }

    /* Occlusion, the rule the depth buffer exists for. */
    {
        static float depth[WINDOW_W];
        int          i = 0;

        while (i < WINDOW_W) { depth[i] = 5.0f; i++; }
        check("a sprite nearer than the wall is drawn",
            sprite_column_visible(depth, 100, 4.0f), 1);
        check("a sprite further than the wall is hidden",
            sprite_column_visible(depth, 100, 6.0f), 0);
        depth[100] = 3.0f;   /* a pillar in this one column */
        check("occlusion is per column, not per sprite",
            sprite_column_visible(depth, 100, 4.0f), 0);
        check("while the neighbouring column still draws it",
            sprite_column_visible(depth, 101, 4.0f), 1);
        check("off the left edge is not visible",
            sprite_column_visible(depth, -1, 1.0f), 0);
        check("off the right edge is not visible",
            sprite_column_visible(depth, WINDOW_W, 1.0f), 0);
    }

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail > 0);
}
TESTC

logic_log=$(gcc -Wall -Wextra -I . -I libtci -o "$WORK_DIR/test_logic" \
    "$WORK_DIR/test_logic.c" map.c vec2.c camera.c raycaster.c scaler.c enemy.c weapon.c \
    libtci/libtci.a libtci/libtciutil.a -lm 2>&1)
if [[ "$?" -ne 0 ]]; then
    fail "logic tester builds with no SDL2 linkage" "$logic_log"; exit 1
fi
pass "logic tester builds with no SDL2 linkage"
[[ -z "$logic_log" ]] && pass "logic tester builds with no warnings" \
    || fail "logic tester builds with no warnings" "$logic_log"

echo
echo "Running the logic tester..."
logic_out=$(cd "$WORK_DIR" && ./test_logic)
logic_status=$?
echo "$logic_out" | grep -E "^PASS|^FAIL" | while read -r line; do echo "  $line"; done
pass_count=$((pass_count + $(echo "$logic_out" | grep -c "^PASS")))
fail_count=$((fail_count + $(echo "$logic_out" | grep -c "^FAIL")))
[[ "$logic_status" -ne 0 ]] && fail "all logic assertions pass" "see failures above"

echo
echo "Running descent headless (2s)..."
SDL_VIDEODRIVER=dummy timeout 2 ./descent "${FIXTURES}/maze1.txt"
rc=$?
if [[ "$rc" -eq 124 ]]; then
    pass "descent runs its event loop for 2s without crashing"
else
    fail "descent runs its event loop for 2s without crashing" "exit code: $rc"
fi

# ── leak report ─────────────────────────────────────────────────────────────────
#
# Runs one representative invocation under valgrind and REPORTS what it finds.
# It never changes the pass/fail count. A leak is something to look at, not a
# reason to refuse your work — but you should see it, because a program that
# leaks is a program that will eventually be killed by the machine it runs on.
#
# Leaks are split by whose code lost the memory. A loss record whose stack
# names one of your own .c files is yours. One that lives entirely inside
# SDL, Mesa or glibc is not, and there is nothing for you to fix there.

leak_report() {
    local label="$1"; shift
    local log="${WORK_DIR:-/tmp}/leaks.$$.log"
    local mine=0 theirs=0 rec frames

    if ! command -v valgrind >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind is not installed, skipping\n" "$label"
        return 0
    fi

    valgrind --leak-check=full --show-leak-kinds=definite,indirect \
             --error-exitcode=0 --log-file="$log" "$@" >/dev/null 2>&1

    if [[ ! -s "$log" ]]; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind produced no output\n" "$label"
        return 0
    fi

    # Split the log into loss records and ask, of each, whether any frame
    # points at a source file sitting in this directory.
    while IFS= read -r rec; do
        frames=$(sed -n "${rec}"',/^==[0-9]*== *$/p' "$log")
        # Every record carries valgrind's own malloc frame; that is not yours.
        # A frame is yours only if it names a source file sitting right here.
        local f owned=0
        for f in $(grep -oE '\(([A-Za-z0-9_-]+\.c):[0-9]+\)' <<<"$frames" \
                   | tr -d '()' | cut -d: -f1 | sort -u); do
            [[ "$f" == vg_replace_malloc.c ]] && continue
            [[ -f "$f" ]] && owned=1
        done
        if (( owned )); then
            mine=$((mine + 1))
            if (( mine == 1 )); then
                printf "  ${C_RED}LEAK${C_RESET}  %s — memory lost by your code:\n" "$label"
            fi
            grep -E 'bytes in [0-9,]+ blocks are (definitely|indirectly)' <<<"$frames" \
                | sed 's/^==[0-9]*== /        /'
            grep -oE '\(([A-Za-z0-9_-]+\.c:[0-9]+)\)' <<<"$frames" \
                | grep -v vg_replace_malloc | head -3 | tr -d '()' \
                | sed 's/^/          at /'
        else
            theirs=$((theirs + 1))
        fi
    done < <(grep -nE 'bytes in [0-9,]+ blocks are (definitely|indirectly) lost' "$log" | cut -d: -f1)

    if (( mine == 0 )); then
        printf "  ${C_GREEN}OK${C_RESET}    %s — no memory lost by your code" "$label"
        if (( theirs > 0 )); then
            printf ' (%d leak(s) inside libraries you did not write)' "$theirs"
        fi
        printf '\n'
    else
        printf '        this does not fail the tester — fix it anyway\n'
    fi
    rm -f "$log"
    return 0
}

# The graphical chapters run until you quit them, and a program killed
# mid-loop reports everything it has not freed yet as "lost" -- which would be
# a lie. So this starts a virtual display, lets the program run, sends it a
# 'q', and measures the clean exit.
leak_report_gui() {
    local label="$1"; shift
    if ! command -v valgrind >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: valgrind is not installed, skipping\n" "$label"
        return 0
    fi
    if ! command -v xvfb-run >/dev/null 2>&1 || ! command -v xte >/dev/null 2>&1; then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: needs xvfb-run and xte for a clean exit, skipping\n" "$label"
        return 0
    fi
    printf "  ${C_BOLD}....${C_RESET}  %s: running under valgrind, this takes a minute\n" "$label"
    local inner="${WORK_DIR:-/tmp}/leak_gui.$$.sh"
    {
        echo "C_GREEN=\"${C_GREEN}\"; C_RED=\"${C_RED}\"; C_BOLD=\"${C_BOLD}\"; C_RESET=\"${C_RESET}\""
        echo "WORK_DIR=\"${WORK_DIR:-/tmp}\""
        declare -f leak_report
        echo '( sleep 12; xte "key q" 2>/dev/null; sleep 5; xte "key q" 2>/dev/null ) &'
        printf 'leak_report %q' "$label"
        printf ' %q' "$@"
        printf '\n'
    } > "$inner"
    timeout 240 xvfb-run -a bash "$inner"
    local rc=$?
    rm -f "$inner"
    if (( rc == 124 )); then
        printf "  ${C_BOLD}NOTE${C_RESET}  %s: the program never exited, so there is nothing honest to measure\n" "$label"
        printf "        (a program killed mid-loop reports everything it holds as lost)\n"
    fi
    return 0
}

echo
leak_report_gui "descent" ./descent "${FIXTURES}/maze1.txt"

echo
hr
printf "  ${C_BOLD}%d passed, %d failed${C_RESET}\n" "$pass_count" "$fail_count"
hr
[[ "$fail_count" -gt 0 ]] && exit 1
exit 0
