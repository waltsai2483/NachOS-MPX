image = dasbd72/nachos:dev-v1.0

.Phony: pull
pull:
	@echo "Pulling image from Docker Hub..."
	@docker pull $(image)

.Phony: push
push:
	@echo "Pushing image to Docker Hub..."
	@docker push $(image)

.Phony: build
build:
	@echo "Building image..."
	@docker build -t $(image) .

.Phony: run
run:
	@echo "Running container..."
	@docker run --rm -v $(CURDIR):/nachos -it --platform=linux/amd64 $(image)
