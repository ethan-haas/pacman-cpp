# Deploying PAC-MAN Reimagined to Kinhank Super Console X2 Pro (EmuELEC)

**Target**: EmuELEC v4.7/4.8 (Amlogic-NG) on S905X2, Cortex-A53 aarch64,
Mali-G31 MP2, 2 GB DDR4. Per the Retro PRD at
`C:\Users\<user>\Documents\Python Projects\Retro\PRD.md`.

This document supersedes `../DEPLOY.md` (which targeted Batocera paths
on x2 generic hardware). The two devices share the EmulationStation
ancestry but EmuELEC has its own filesystem conventions.

## What's different from Batocera

| concern | Batocera (old DEPLOY.md) | **EmuELEC (this file)** |
|---|---|---|
| App install path | `/userdata/apps/<name>/` | `/storage/roms/ports/<name>/` |
| Launcher shim | EmulationStation .desktop | `.sh` under `/storage/roms/ports/` |
| Service name | `emulationstation` | **`emustation`** |
| Restart tool | `batocera-es-swissknife --restart` | `systemctl restart emustation` |
| Read-only paths | `/usr` | `/usr`, `/etc`, `/flash` |
| Config override | `/userdata/system/` | `/storage/.config/` |
| Default user/host | `root@batocera.local` | `root@emuelec.local` |
| Default ssh pw | `linux` | **`emuelec`** |
| Boot persistence | `/userdata/system/custom.sh` | **`/storage/.config/custom_start.sh`** (NOT `autostart.sh`) |
| Bundled SD endurance | OK | Replace before use (PRD Â§2.2) |

The PRD's golden rules (Â§5.5) apply verbatim â€” never touch `/etc`,
`/usr`, `/flash`, `eMMC`; always back up before push; only restart
`emustation`; dry-run rsync first.

## End-to-end flow

```
                                       â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
[1] dev workstation (x86_64) â”€â”€â”€â”€â”€â”€â”€â”€â–º â”‚  Docker buildx     â”‚
    repo @ workspace/PACMAN_CPP        â”‚  arm64v8 debian    â”‚
                                       â”‚  builds aarch64    â”‚
                                       â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                                                â”‚
                                                â–¼
                                       build-arm64/pacman-reimagined-arm64
                                                â”‚
                                                â”‚   rsync over ssh
                                                â–¼
[2] x2 Pro running EmuELEC v4.8        â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
    via ssh (key auth per PRD Â§5.2)    â”‚ /storage/roms/     â”‚
                                       â”‚   ports/           â”‚
                                       â”‚     pacman-reim.sh â”‚
                                       â”‚     pacman-reim/   â”‚
                                       â”‚       *-arm64      â”‚
                                       â”‚     gamelist.xml   â”‚
                                       â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                                                â”‚
                                                â–¼
                                       EmulationStation re-launches,
                                       new tile appears under PORTS
```

## Step 0 â€” Bootstrap (one-time, per PRD Â§5.2 / Â§5.5)

On the dev workstation:

```bash
# Generate key + copy to device (PRD Â§5.2)
ssh-keygen -t ed25519 -f ~/.ssh/emuelec_ed25519 -C "claude-code@emuelec" -N ""
ssh-copy-id -i ~/.ssh/emuelec_ed25519.pub root@emuelec.local   # pw: emuelec

# Pin host alias + ControlMaster (PRD Â§5.3)
cat >> ~/.ssh/config <<'EOF'
Host emuelec
    HostName emuelec.local
    User root
    IdentityFile ~/.ssh/emuelec_ed25519
    IdentitiesOnly yes
    ServerAliveInterval 30
    ControlMaster auto
    ControlPath ~/.ssh/sockets/%r@%h:%p
    ControlPersist 10m
EOF
mkdir -p ~/.ssh/sockets && chmod 700 ~/.ssh/sockets

# (Optional) Disable ssh password auth on device per PRD Â§5.2 last block
```

## Step 1 â€” Cross-compile aarch64 binary

### Option A â€” Docker buildx (works on any host with Docker)

```bash
# one-time: enable arm64 emulation
docker run --privileged --rm tonistiigi/binfmt --install all

# build
bash packaging/emuelec/build-arm64.sh
# â†’ build-arm64/pacman-reimagined-arm64
```

### Option B â€” Native aarch64 box (Pi 4/5, Graviton, etc.)

```bash
sudo apt-get install -y build-essential cmake libsdl2-dev
bash packaging/emuelec/build-arm64-native.sh
```

Either path: confirm with `file build-arm64/pacman-reimagined-arm64`,
should show `ELF 64-bit LSB ... ARM aarch64 ...`.

## Step 2 â€” Push to device

### Option A â€” push from dev box (preferred, PRD Â§5.7 safe-push pattern)

```bash
bash packaging/emuelec/deploy-from-dev.sh           # uses host alias 'emuelec'
# or
bash packaging/emuelec/deploy-from-dev.sh emuelec-x2pro
```

This script:
1. Pre-flight ssh check.
2. Backup `/storage/roms/ports/` to `/storage/.cache/pacman-backups/<TS>/ports.bak/`.
3. Dry-run rsync (`-avzn --itemize-changes`) â€” pauses for approval.
4. `systemctl stop emustation`.
5. rsync binary + shim into place.
6. Merge gamelist.xml entry via python3 on device (preserves existing entries).
7. `systemctl restart emustation`.
8. `journalctl -u emustation` grep for `error|fail|segfault`.

### Option B â€” copy + install on device

```bash
# from dev box:
rsync -avz --exclude='.venv*' --exclude='build/' \
    workspace/PACMAN_CPP/ root@emuelec:/tmp/PACMAN_CPP/

# on device:
ssh root@emuelec.local
cd /tmp/PACMAN_CPP
sh packaging/emuelec/install-emuelec.sh
```

## Step 3 â€” Verify per PRD Â§5.7

```bash
ssh emuelec 'systemctl is-active emustation'        # â†’ active
ssh emuelec 'ls -lh /storage/roms/ports/pacman-reimagined/'
ssh emuelec 'tail -50 /storage/.cache/pacman-reimagined.log'
ssh emuelec 'cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor'   # â†’ performance
```

Then on the device, navigate to **PORTS** in EmulationStation. The
**PAC-MAN Reimagined** tile should be present. Launching it should
reach the main menu in â‰¤ 20s (PRD v2.0 Â§3.2 success metric â€” that
target predates this hardware constraint; on a 1.8 GHz A53 expect
1â€“3s).

## Step 4 â€” Controller verification (PRD Â§11)

Per PRD Â§11.1, recommended input is **wired USB** or **8BitDo USB
Wireless Adapter 2**. Bluetooth is "not recommended" by EmuELEC's own
wiki. The two-player flow:

1. Plug two controllers (USB) **before** launching the port.
2. From the main menu, navigate Multiplayer â†’ Mode Select.
3. First controller to press a button = P1 (yellow). Second = P2 (cyan).
4. Hot-plug a third controller mid-game: P1/P2 assignment is sticky;
   the manager's 5s grace runs before pausing the game.

If a Bluetooth DS4 is paired, expect 10â€“20 ms extra input latency vs
USB. The game does not implement run-ahead (no need â€” it's native, not
emulated), so input latency is bounded by HDMI vsync + controller path.

## Step 5 â€” Performance check (PRD acceptance)

The PRD Â§8 holds shaders to 60fps at 1080p for Tier-A emulator cores
(NES/SNES/etc). Native PAC-MAN is **dramatically lighter** than any
emulator core â€” expect locked 60fps on A53 with the bitmap font HUD
even at 1080p output (rendered at 672Ã—864 logical, SDL letterboxed and
scaled by GLES blitter).

Probe under load:
```bash
ssh emuelec 'cat /sys/class/thermal/thermal_zone0/temp'   # /1000 = Â°C; should stay < 70 (PRD Â§2.4)
ssh emuelec 'top -bn1 | head -20'                          # confirm pacman pegging one A53 core
```

## What the PRD does NOT cover that we still respect

- **No Vulkan calls** in our renderer (PRD Â§6.1 / Â§14 ban â€” Mali-G31
  fbdev blob exposes EGL/GLES only). Our `app.cpp` uses
  `SDL_RENDERER_ACCELERATED`, which SDL2 resolves to GLES on EmuELEC.
- **No 4K mode** (PRD Â§14 anti-pattern). Logical 672Ã—864 â†’ letterbox
  at 1080p via `SDL_RenderSetLogicalSize`.
- **No threaded video** trick (irrelevant: native game, not core).
- **No eMMC writes** â€” install only under `/storage/`.
- **No `/etc/` edits** â€” config baked into the binary.
- **No `apt`/`opkg` on device** â€” EmuELEC has read-only squashfs,
  cross-compile from dev box mandatory.

## Companion PAC-MAN (original 1980) via MAME 2003 â€” same as PRD Â§10

The original arcade ROM is **out of scope for this packaging**. To add
a launcher tile for it, follow PRD Â§7 (FBNeo / MAME 2003-Plus) with
`puckman.zip` under `/storage/roms/arcade/`. EmuELEC's existing arcade
system will pick it up. We do not ship the ROM.

## Uninstall

```bash
ssh emuelec '
    systemctl stop emustation
    rm -rf /storage/roms/ports/pacman-reimagined
    rm -f /storage/roms/ports/pacman-reimagined.sh
    # Restore gamelist.xml from latest backup
    LATEST=$(ls -1t /storage/.cache/pacman-backups | head -1)
    cp /storage/.cache/pacman-backups/$LATEST/ports.bak/gamelist.xml /storage/roms/ports/ 2>/dev/null || true
    systemctl start emustation
'
```

## Troubleshooting

| symptom | check | fix |
|---|---|---|
| Tile missing from PORTS | `ssh emuelec 'cat /storage/roms/ports/gamelist.xml'` | re-run merge step; restart `emustation` |
| Launch â†’ black screen â†’ ES returns | `tail /storage/.cache/pacman-reimagined.log` | confirm `SDL_VIDEODRIVER=kmsdrm` is set; check binary is aarch64 (`file`) |
| Segfault | `journalctl -u emustation --no-pager | grep pacman` | usually missing libSDL2; `ssh emuelec 'ldd /storage/roms/ports/pacman-reimagined/pacman-reimagined-arm64'` |
| Controller not detected | `ssh emuelec 'cat /proc/bus/input/devices'` | per PRD Â§11.2, prefer wired or 8BitDo Adapter 2 over BT |
| Slow / drops frames | `top` shows single core 100%? | Confirm governor = performance (PRD Â§4.3). Should not happen â€” native game â‰ª emulator load |
| Won't fullscreen | env not picked up | edit `pacman-reimagined.sh` to set `SDL_VIDEODRIVER=kmsdrm` explicitly |

