FROM gattic/base:latest
MAINTAINER Lerring

RUN mkdir -p /app
COPY . /app

RUN mkdir /app/build
WORKDIR /app/build
RUN cmake  ../
RUN make install

