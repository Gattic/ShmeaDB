FROM gattic/base:latest
MAINTAINER Lerring

RUN mkdir -p /app
COPY . /app

#INSTALL
RUN mkdir /app/build
WORKDIR /app/build
RUN cmake  ../
RUN make install

RUN TESTS
RUN mkdir /app/unit-tests/build
WORKDIR /app/unit-tests/build
RUN cmake ../
RUN make run

