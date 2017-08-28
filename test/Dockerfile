FROM ubuntu:xenial

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install software-properties-common python-software-properties -y
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get update
RUN apt-get install gcc-4.8 libgs-dev cmake curl -y
RUN curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.2/install.sh | bash
RUN . "/root/.nvm/nvm.sh"
RUN bash -i -c "nvm install 8"