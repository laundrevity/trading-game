FROM ubuntu:latest
RUN apt-get -qq update && apt-get install -qy software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt install gcc-11 g++-11 gdb cmake vim fish git ssh openssh-server -y
# now download and install python3 and activate venv
RUN : \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    software-properties-common \
    && add-apt-repository -y ppa:deadsnakes \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    python3.10-venv \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && :
RUN apt-get update
RUN apt-get install curl -y
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3.10 get-pip.py
RUN pip3 install conan
COPY . /app
WORKDIR /app
RUN pip3 install -r requirements.txt
RUN echo starting
RUN chmod +x slumber.sh
ENTRYPOINT [ "./slumber.sh" ]