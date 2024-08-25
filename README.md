# Nachos-4.0 for Operating System Course

## Docker

### Setup

```bash
# Download the source code if you haven't
git clone git@github.com:NTHU-LSALAB/NachOS-4.0_MP1_src.git NachOS-4.0_MP1
cd NachOS-4.0_MP1
# Build the docker image
docker build -t nachos .
```

### Run

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
