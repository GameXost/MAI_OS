FROM gcc:14.2.0

RUN apt-get update && apt-get install -y \
	python3 python3-pip cmake clangd valgrind \
	&& rm -rf /var/lib/apt/lists/*

RUN pip install numpy --break-system-packages

WORKDIR /workspace
