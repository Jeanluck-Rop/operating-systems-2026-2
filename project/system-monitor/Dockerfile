FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    cmake \
    libncurses-dev \
    libgtk-4-dev \
    libadwaita-1-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build && cmake --build build

CMD ["./build/src/sysmon"]
