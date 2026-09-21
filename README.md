# Ceres
Trilling synthesizer for daisy-seed + Synthux SimpleTouch, based on a 1989 audio recording captured during a reported crop circle formation

## QUICK INSTALL
Download the [Binary file](https://github.com/chrisades/Ceres/releases/latest/download/Ceres.bin) and flash using the [Daisy Seed web programmer](https://flash.daisy.audio/)

## CONTROLS
<img src="touch.jpeg" width="300"/>

**Switches**
- S07-S08 - 
- S09-S10 - 

**Knobs**
- S30 - 
- S31 - 
- S32 - 
- S33 - 
- S34 - 
- S35 - 
- S36 - 
- S37 - 

**Pads**
- P01 - 
- P00/P02 - 
- P03...P09 - 
- P10/P11 - 

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