FROM haskellol/base:main
MAINTAINER Lerring

RUN mkdir -p /ShmeaDB
COPY . /ShmeaDB

#INSTALL
RUN mkdir /ShmeaDB/build
WORKDIR /ShmeaDB/build
RUN cmake  ../ --trace-expand
RUN make install

#TESTS
RUN mkdir /ShmeaDB/unit-tests/build
WORKDIR /ShmeaDB/unit-tests/build
RUN cmake ../
RUN make run

WORKDIR / 
