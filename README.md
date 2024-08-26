# Nachos-4.0 for Operating System Course

## Docker

### Prerequisite(MacOs)

For users not using x86 systems, to build and run the image, you should install qemu first.

For example, to install qemu on mac, you should

- Install [brew](https://brew.sh/) first
- Then install qemu through brew with `brew install qemu`

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install qemu
```

#### Issues

If you still see `qemu: uncaught target signal 11 (Segmentation fault) - core dumped` after qemu is installed, you may need to downgrade docker desktop to 4.24.2 or upgrade macos to Sonoma. See [here](https://github.com/docker/for-mac/issues/7172)

### Build

```bash
# Download the source code if you haven't
git clone git@github.com:NTHU-LSALAB/NachOS-4.0_MP1_src.git NachOS-4.0_MP1
cd NachOS-4.0_MP1
# Build the docker image
docker build -t nachos .
```

### Run

Tested environments:

- Ubuntu
- MacOS
- WSL2 on Windows (not under directory mounted, e.g. /mnt/c/)

```bash
# Get into the directory if you are not in it
cd NachOS-4.0_MP1
# Run the docker container with the source code mounted
docker run --rm -v $(pwd):/nachos -it --platform=linux/amd64 nachos
```

```bash
# Inside the container, you should be in the code/test directory
# Build nachos with the provided script
bash build_nachos_docker.sh
# Build the target test
make clean; make <test>
# Run the test
../build.linux/nachos.bin -e <test>
```
