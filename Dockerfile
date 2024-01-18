FROM --platform=amd64 amd64/centos:centos7.9.2009

ARG DOCKER_SHELL=bash

COPY usr/local/nachos /usr/local/nachos
RUN yum install -y ed csh compat-gcc-44.x86_64 compat-gcc-44-c++.x86_64 glibc-devel.i686 glibc-devel.x86_64 libstdc++-4.8.5-44.el7.i686 cpp.x86_64 make && \
    yum clean all && \
    rm -rf /var/cache/yum/*

RUN ln -s /usr/bin/g++44 /usr/bin/g++
RUN echo 

WORKDIR /nachos

ENTRYPOINT [ "/bin/bash" ]
