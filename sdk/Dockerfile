# Build environment for TLSR8232 firmware: Telink's TC32 GCC and make, nothing
# else. The SDK is not in the image: it lives in this repo (sdk/tlsr8232) and
# reaches the container through the repo mount, so SDK patches need no image
# rebuild.
#
# tc32_gcc ships as x86_64 Linux binaries, so pin the platform — on Apple
# Silicon a native arm64 image builds fine but cannot exec the compiler.

FROM --platform=linux/amd64 debian:bookworm-slim AS toolchain

# The toolchain's only known download is this Telink mirror; the checksum makes
# a changed or truncated file fail the build instead of producing a different
# compiler. TC32_URL can point at another copy of the same file.
ARG TC32_URL=http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2
ARG TC32_SHA256=33b854be3e3db3dba4b4dacdda2cd4ea1c94dfd4d562864a095956de7991b430

RUN apt-get update && apt-get install -y --no-install-recommends wget ca-certificates bzip2

# Keep the compiler, assembler and binutils. Drop the debuggers (openocd, gdb,
# tcdb), profilers, docs and test binaries, and turn the duplicate binutils in
# tc32-elf/bin/ into links to bin/. gcc keeps both copies: the driver finds cc1
# relative to its own resolved path, so a symlinked gcc cannot find it.
RUN set -e; \
	wget -q -O /tmp/tc32.tar.bz2 "$TC32_URL"; \
	echo "$TC32_SHA256  /tmp/tc32.tar.bz2" | sha256sum -c -; \
	tar xjf /tmp/tc32.tar.bz2 -C /opt; \
	cd /opt/tc32; \
	rm -rf doc share test_bin lib/libiberty.a \
		bin/openocd bin/tcdb bin/tc32-elf-gdb bin/tc32-elf-gdbtui \
		bin/tc32-elf-gcov bin/tc32-elf-gprof bin/tc32-elf-gccbug; \
	for f in tc32-elf/bin/*; do \
		n=$(basename $f); \
		if [ $n != gcc ] && cmp -s $f bin/tc32-elf-$n; then ln -sf ../../bin/tc32-elf-$n $f; fi; \
	done; \
	bin/tc32-elf-gcc --version >/dev/null

FROM --platform=linux/amd64 debian:bookworm-slim

RUN apt-get update && \
	apt-get install -y --no-install-recommends make && \
	rm -rf /var/lib/apt/lists/* /usr/share/doc /usr/share/man /usr/share/info /usr/share/locale

COPY --from=toolchain /opt/tc32 /opt/tc32

WORKDIR /src

ENV TC32_HOME="/opt/tc32" \
	PATH="/opt/tc32/bin:${PATH}"
