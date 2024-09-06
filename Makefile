.PHONY: build
build:
	cd c; \
	cmake -DCMAKE_BUILD_TYPE=Release -B ../bin; \
	cmake --build ../bin \

.PHONY: install
install:
	cp ./bin/ilex /usr/local/bin

.PHONY: bi
bi: build install
