#OBJS specifies which files to compile
OBJS = src/main.cpp src/i8080.cpp

#OBJ_NAME specifies the name of our binary
OBJ_NAME = invaders

#The target that compiles our executable
all: $(OBJS)
	g++ $(OBJS) -w -o $(OBJ_NAME)