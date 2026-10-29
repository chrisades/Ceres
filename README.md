# Ceres
Poly-trillic synthesizer for daisy-seed + Synthux SimpleTouch, based on a 1989 audio recording captured during a reported crop circle formation

## QUICK INSTALL
Download the [Binary file](https://github.com/chrisades/Ceres/releases/latest/download/Ceres.bin) and flash using the [Daisy Seed web programmer](https://flash.daisy.audio/)

## CONTROLS
<img src="touch.jpeg" width="300"/>

**Switches**
- S07-S08 - modulation mode (trill morph | steps | chaos)
- S09-S10 - morph mode (resonant | shimmer | squeeze)

**Knobs**
- S30 - tone A frequency
- S31 - trill width A
- S32 - mix-AB + trill distance + dual-delay feedback
- S33 - trill speed + dual-delay times
- S34 - trill width B
- S35 - tone B frequency
- S36 - modulation depth + stereo dual-delay mix
- S37 - morph

**Pads**
- P00 - scale cycle (4ths&5ths | Lydian | D Kurd | Hijaz | Hirajoshi)
- P01 - octave cycle (0 | +1 | +2 | +3)
- P02 - trill track (trill speed | none)
- P03...P09 - scale notes
- P10 - morse mode (on | off)
- P11 - touch envelope (slow | fast | hold)

## PREREQUISITES
- [Daisy Toolchain](https://docs.daisy.audio/tutorials/cpp-dev-env/) (ARM GCC + make)
- **Windows:** use [Git Bash](https://git-scm.com/downloads) to run the commands below

## Project Setup
```shell
$ git clone --recurse-submodules https://github.com/chrisades/Ceres.git
$ make libs -j8
$ make clean; make -j8
```

If you already have the repo cloned without submodules, run this first:
```shell
$ git submodule update --init --recursive
```

## UPLOAD
```shell
$ make program-dfu
```

> [!NOTE]
> When tweaking code, run `make clean && make` for a full rebuild, or just `make` for an incremental rebuild (only recompiles changed files). The compiled binary is placed in the `build/` folder as `ZoscTouch.bin`.