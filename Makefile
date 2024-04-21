BINARY_NAME = CubesGame
BUILD_PATH = ./bin/$(BINARY_NAME)

LIB = -lGLEW -lglfw

run:
	$(BUILD_PATH)

build:
	clang ./src/main.c $(LIB) -framework OpenGL -o $(BUILD_PATH)

# BUILD AND RUN IN ONE GO
s:
	clang ./src/main.c $(LIB) -framework OpenGL -o $(BUILD_PATH)
	$(BUILD_PATH)
