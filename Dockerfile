FROM --platform=linux/amd64 amd64/centos:centos7.9.2009

ARG DOCKER_SHELL=bash

COPY usr/local/nachos /usr/local/nachos
# Replace the default CentOS-Base.repo with the one that has the correct mirror after centos is EOL
COPY docker/CentOS-Base.repo /etc/yum.repos.d/CentOS-Base.repo
# Install the necessary packages
RUN yum install -y ed csh compat-gcc-44.x86_64 compat-gcc-44-c++.x86_64 glibc-devel.i686 glibc-devel.x86_64 libstdc++-4.8.5-44.el7.i686 cpp.x86_64 make git
# Clean up the yum cache
RUN yum clean all && \
    rm -rf /var/cache/yum/*

# Make g++44 the default g++
RUN ln -s /usr/bin/g++44 /usr/bin/g++
# Install oh-my-bash for a better shell experience
RUN bash -c "$(curl -fsSL https://raw.githubusercontent.com/ohmybash/oh-my-bash/master/tools/install.sh)"

WORKDIR /nachos

ENTRYPOINT [ "/bin/bash" ]
