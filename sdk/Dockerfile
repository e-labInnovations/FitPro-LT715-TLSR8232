# tc32_gcc ships as x86_64 Linux binaries, so pin the platform — on Apple
# Silicon a native arm64 image builds fine but cannot exec the compiler.
FROM --platform=linux/amd64 ubuntu:22.04

RUN apt-get update && \
	apt-get install -y wget make bzip2 unzip

RUN cd /opt && \
	wget http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2 && \
	tar jxvf tc32_gcc_v2.0.tar.bz2

RUN cd /opt && \
	wget http://wiki.telink-semi.cn/tools_and_sdk/BLE/8232_BLE_SDK.zip && \
	unzip 8232_BLE_SDK.zip

WORKDIR /app

# The zip is named 8232_BLE_SDK but extracts straight to /opt/ble_sdk_hawk
ENV SDK="/opt/ble_sdk_hawk" \
	TC32_HOME="/opt/tc32" \
	PATH="/opt/tc32/bin:${PATH}"
