SOURCE_FILES=main.cpp core.cpp math.cpp
HEADER_FILES=Core.hpp Math.hpp
INCLUDE_DIRS=Source
SOURCE_DIR=Source
OBJ_DIR=Obj
OBJ_FILES=$(SOURCE_FILES:.cpp=.o)
CPP=g++
CPP_FLAGS=-g -Wextra -Werror

all: $(OBJ_DIR) vox

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(addprefix -I,$(INCLUDE_DIRS)) -c $< -o $@

vox: $(addprefix $(OBJ_DIR)/,$(OBJ_FILES))
	$(CPP) $(CPP_FLAGS) $(addprefix $(OBJ_DIR)/,$(OBJ_FILES)) -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f vox

re: fclean all

.PHONY: all clean fclean re
