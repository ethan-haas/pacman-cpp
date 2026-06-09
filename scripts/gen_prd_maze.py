#!/usr/bin/env python3
"""Generate the PRD-conformant 28×36 Pac-Man maze.

Spec from pacman-coop-PRD.md §9:
- 28 cols × 36 rows
- Tunnels at row 17 (both edges)
- Ghost house centred, 8×5 tiles, 2-tile wide door at top
- 4 power pellets in 4 corners
- ~240 standard dots filling walkable cells
"""

COLS = 28
ROWS = 36

# Hand-designed half (left 14 cols). Mirror to produce right 14 cols.
# Each half row = 14 chars. Designed for classic Pac-Man feel.
HALF = [
    "##############",  # 0
    "#............#",  # 1
    "#.####.#####.#",  # 2
    "#o####.#####.#",  # 3
    "#.####.#####.#",  # 4
    "#............#",  # 5
    "#.####.##.####",  # 6
    "#.####.##.####",  # 7
    "#......##....#",  # 8
    "######.##.####",  # 9
    "######.##.####",  # 10
    "######.##....#",  # 11
    "######.## ####",  # 12  ghost-house side
    "######.## #1 #",  # 13
    "      .   #  #",  # 14  tunnel edge open + house wall
    "######.## #  #",  # 15
    "######.## ####",  # 16
    "      .      #",  # 17  PRD tunnel row
    "######.## ####",  # 18
    "######.## ####",  # 19
    "#............#",  # 20
    "#.####.#####.#",  # 21
    "#.####.#####.#",  # 22
    "#o..##.......#",  # 23
    "###.##.##.####",  # 24
    "###.##.##.####",  # 25
    "#......##....#",  # 26
    "#.##########.#",  # 27
    "#.##########.#",  # 28
    "#............#",  # 29
    "#.####.#####.#",  # 30
    "#.####.#####.#",  # 31
    "#.####.#####.#",  # 32
    "#.##########.#",  # 33
    "#............#",  # 34
    "##############",  # 35
]

assert len(HALF) == ROWS
for i, row in enumerate(HALF):
    assert len(row) == 14, f"row {i} len {len(row)} != 14: {row!r}"

def mirror_row(half: str) -> str:
    flip = {"1": "4", "2": "3", "3": "2", "4": "1"}
    right = "".join(flip.get(c, c) for c in reversed(half))
    return half + right

rows = [mirror_row(h) for h in HALF]
for i, r in enumerate(rows):
    assert len(r) == COLS

# Find col for Pac spawn (will be at center cols 13-14)
# Insert 'P' markers at row 23 cols 13-14
def patch(rows, y, x, ch):
    rows[y] = rows[y][:x] + ch + rows[y][x+1:]

# Pac spawn cols 13-14 row 23 (mirror seam)
patch(rows, 23, 13, 'P')
patch(rows, 23, 14, 'P')
# Door at row 12 cols 13-14
patch(rows, 12, 13, '-')
patch(rows, 12, 14, '-')
# Ghost spawns
patch(rows, 13, 12, '1')
patch(rows, 13, 14, '2')
patch(rows, 13, 15, '3')
patch(rows, 13, 17, '4')

# Tunnel row 17 — PRD spec
# Mark cols 0..2 and 25..27 as T
for x in (0, 1, 2):
    patch(rows, 17, x, 'T')
for x in (25, 26, 27):
    patch(rows, 17, x, 'T')

# Dead-end auto-fix (turn walkable spurs to wall)
WALKABLE = set('.oPT1234-')
KEEP = set('oPT1234-')
def deg(rows, x, y):
    n = 0
    for dx, dy in [(1,0),(-1,0),(0,1),(0,-1)]:
        if 0 <= x+dx < COLS and 0 <= y+dy < ROWS:
            if rows[y+dy][x+dx] in WALKABLE:
                n += 1
    return n

def in_house(x, y):
    return 11 <= x <= 16 and 12 <= y <= 16

for _ in range(50):
    changed = False
    new = list(rows)
    for y in range(1, ROWS - 1):
        for x in range(1, COLS - 1):
            ch = rows[y][x]
            if ch not in WALKABLE: continue
            if ch in KEEP: continue
            if in_house(x, y): continue
            if deg(rows, x, y) < 2:
                new[y] = new[y][:x] + '#' + new[y][x+1:]
                changed = True
    rows = new
    if not changed: break

# Count pellets
pellet_count = sum(r.count('.') for r in rows)
energizer_count = sum(r.count('o') for r in rows)
print(f"// pellets={pellet_count}  energizers={energizer_count}")

# Dead end check
dead = []
for y in range(1, ROWS-1):
    for x in range(1, COLS-1):
        if rows[y][x] in WALKABLE and rows[y][x] not in KEEP and not in_house(x, y):
            if deg(rows, x, y) < 2:
                dead.append((x, y))
print(f"// dead ends: {len(dead)}")

print(f"static constexpr std::array<std::string_view, {ROWS}> CLASSIC_MAZE = {{")
for r in rows:
    print(f'    "{r}",')
print("};")
