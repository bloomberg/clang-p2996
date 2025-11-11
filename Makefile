# Unified Makefile targets for building and using the Docker images
# that provide Clang with experimental C++26 Reflection (P2996).

IMAGE_NAME       ?= p2996-clang
BUILD_CTX        ?= .
INSTALL_PREFIX   ?= /opt/p2996/clang

ARM64_IMAGE      := $(IMAGE_NAME):arm64
AMD64_IMAGE      := $(IMAGE_NAME):amd64

.PHONY: \
  docker-build-arm64 docker-run-arm64 docker-clang-version-arm64 docker-clang-check-arm64 docker-clang-p2996-examples-arm64 \
  docker-build-amd64 docker-run-amd64 docker-clang-version-amd64 docker-clang-check-amd64 docker-clang-p2996-examples-amd64 \
  docker-push-arm64 docker-push-amd64

# ===== arm64 (unified Dockerfile) =====
docker-build-arm64:
	@echo "[+] Building $(ARM64_IMAGE) using docker/Dockerfile (ARCH=arm64)"
	docker build -t $(ARM64_IMAGE) -f docker/Dockerfile --build-arg ARCH=arm64 $(BUILD_CTX)

docker-run-arm64:
	@echo "[+] Starting interactive shell in $(ARM64_IMAGE)"
	docker run --rm -it \
	  -v "$(PWD)":/work \
	  -w /work \
	  $(ARM64_IMAGE) bash

docker-clang-version-arm64:
	@echo "[+] Capturing 'clang++ --version' inside $(ARM64_IMAGE) into CLANG_BUILD_STRING"
	docker run --rm $(ARM64_IMAGE) bash -lc "export CLANG_BUILD_STRING=\$$(clang++ --version | tr '\n' ';'); echo \"\$$CLANG_BUILD_STRING\""

docker-clang-check-arm64:
	@echo "[+] Building and running check_clang_libc++ (arm64) passing CLANG_BUILD_STRING inside $(ARM64_IMAGE)"
	docker run --rm \
	  -e INSTALL_PREFIX=$(INSTALL_PREFIX) \
	  -v "$(PWD)":/work \
	  -w /work \
	  $(ARM64_IMAGE) bash docker/clang-check.sh

docker-clang-p2996-examples-arm64:
	@echo "[+] Building and running p2996_examples (arm64) inside $(ARM64_IMAGE)"
	docker run --rm \
	  -e INSTALL_PREFIX=$(INSTALL_PREFIX) \
	  -v "$(PWD)":/work \
	  -w /work \
	  $(ARM64_IMAGE) bash docker/p2996-examples.sh

# ===== amd64 (unified Dockerfile) =====
docker-build-amd64:
	@echo "[+] Building $(AMD64_IMAGE) using docker/Dockerfile (ARCH=amd64)"
	docker build -t $(AMD64_IMAGE) -f docker/Dockerfile --build-arg ARCH=amd64 $(BUILD_CTX)

docker-run-amd64:
	@echo "[+] Starting interactive shell in $(AMD64_IMAGE)"
	docker run --rm -it \
	  -v "$(PWD)":/work \
	  -w /work \
	  $(AMD64_IMAGE) bash

docker-clang-version-amd64:
	@echo "[+] Capturing 'clang++ --version' inside $(AMD64_IMAGE) into CLANG_BUILD_STRING"
	docker run --rm $(AMD64_IMAGE) bash -lc "export CLANG_BUILD_STRING=\$$(clang++ --version | tr '\n' ';'); echo \"\$$CLANG_BUILD_STRING\""

docker-clang-check-amd64:
	@echo "[+] Building and running check_clang_libc++ (amd64) passing CLANG_BUILD_STRING inside $(AMD64_IMAGE)"
	docker run --rm \
	  -e INSTALL_PREFIX=$(INSTALL_PREFIX) \
	  -e LIB_SUBDIR=x86_64-unknown-linux-gnu \
	  -v "$(PWD)":/work \
	  -w /work \
	  $(AMD64_IMAGE) bash docker/clang-check.sh

docker-clang-p2996-examples-amd64:
	@echo "[+] Building and running p2996_examples (amd64) inside $(AMD64_IMAGE)"
	docker run --rm \
  	  -e INSTALL_PREFIX=$(INSTALL_PREFIX) \
  	  -e LIB_SUBDIR=x86_64-unknown-linux-gnu \
  	  -v "$(PWD)":/work \
  	  -w /work \
  	  $(AMD64_IMAGE) bash docker/p2996-examples.sh

# ===== push images =====
docker-push-arm64:
	@echo "[+] Preparing to tag and push: $(ARM64_IMAGE) -> $(REGISTRY_IMAGE):$${TAG:-arm64}"
	@test -n "$(REGISTRY_IMAGE)" || { echo "ERROR: REGISTRY_IMAGE is required"; exit 1; }
	@T="$(TAG)"; if [ -z "$$T" ]; then T=arm64; fi; \
	  docker tag $(ARM64_IMAGE) $(REGISTRY_IMAGE):$$T && docker push $(REGISTRY_IMAGE):$$T

docker-push-amd64:
	@echo "[+] Preparing to tag and push: $(AMD64_IMAGE) -> $(REGISTRY_IMAGE):$${TAG:-amd64}"
	@test -n "$(REGISTRY_IMAGE)" || { echo "ERROR: REGISTRY_IMAGE is required"; exit 1; }
	@T="$(TAG)"; if [ -z "$$T" ]; then T=amd64; fi; \
	  docker tag $(AMD64_IMAGE) $(REGISTRY_IMAGE):$$T && docker push $(REGISTRY_IMAGE):$$T
