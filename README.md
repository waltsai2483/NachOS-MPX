# Nachos-4.0 for Operating System Course

## Docker

### Setup

```bash
git clone git@github.com:NTHU-LSALAB/NachOS-4.0_MP1_src.git NachOS-4.0_MP1
cd NachOS-4.0_MP1
docker build -t nachos .
```

### Run

```bash
cd NachOS-4.0_MP1
docker run --rm -v $(PWD):/nachos -it nachos
$ cd code/test
# Build nachos
$ bash build_nachos.sh
# Build tests
$ make clean; make <test>
$ ../build.linux/nachos -e <test>
```
