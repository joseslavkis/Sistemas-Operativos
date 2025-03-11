FROM ubuntu:22.04

RUN apt-get update && apt-get install -y make gcc build-essential python3 python3-pip time

RUN pip install termcolor pyyaml

WORKDIR /shell

COPY . .

RUN cc tests/reflector.c -o tests/reflector

RUN make clean && make -B -e SHELL_TEST=true
