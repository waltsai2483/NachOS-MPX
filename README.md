# Nachos-4.0 for Operating System Course

## Docker

### Prerequisite(MacOs)

If you see `qemu: uncaught target signal 11 (Segmentation fault) - core dumped`, you may need to downgrade docker desktop to 4.24.2 or upgrade macos to Sonoma. See [here](https://github.com/docker/for-mac/issues/7172)

### Build(Not required)

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
docker run --rm -v $(pwd):/nachos -it --platform=linux/amd64 dasbd72/nachos:dev-v1.0
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