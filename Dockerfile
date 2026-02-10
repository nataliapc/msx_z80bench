FROM nataliapc/sdcc:4.2.0

# Copy hex2bin source and Makefile
COPY bin/hex2bin.c bin/Makefile.hex2bin /tmp/

# Build and install hex2bin using Make
RUN apk add --no-cache gcc make musl-dev && \
    cd /tmp && \
    make -f Makefile.hex2bin install && \
    cd / && \
    rm -rf /tmp/* && \
    apk del gcc make musl-dev
