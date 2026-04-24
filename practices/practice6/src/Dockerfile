# Use Alpine as a base image
FROM alpine:latest

# Install build dependencies
RUN apk add --no-cache build-base cmake gcc musl-dev

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Compile c_sim
RUN gcc main.c vm_bl.c paging_basic.c mmu.c tlb.c paging_on_demand.c ram.c -o c_sim

# Set default command
CMD ["./validator_alpine"]
