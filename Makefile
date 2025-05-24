OPENGL_NAME=vox-gl
VULKAN_NAME=vox-vk

SRC_DIR=Source
SRC_FILES=main.cpp core.cpp math.cpp input.cpp noise.cpp renderer.cpp world.cpp shader_preprocessor.cpp ui.cpp
OPENGL_SRC_FILES=OpenGL/opengl.cpp OpenGL/render_pass.cpp OpenGL/copy_pass.cpp OpenGL/pipeline_state.cpp OpenGL/shader.cpp OpenGL/texture.cpp OpenGL/buffer.cpp

INCLUDE_DIRS=Source /usr/include/SDL2 Third-Party/stb_image
OPENGL_INCLUDE_DIRS=Third-Party/glad/include
VULKAN_INCLUDE_DIRS=$(HOME)/vulkan/1.4.313.0/x86_64/include

OPENGL_OBJ_DIR=Obj/OpenGL
VULKAN_OBJ_DIR=Obj/Vulkan

OBJ_FILES=$(SRC_FILES:.cpp=.o)
OPENGL_OBJ_FILES=$(OPENGL_SRC_FILES:.cpp=.o) glad.o stb_image.o
VULKAN_OBJ_FILES=$(VULKAN_SRC_FILES:.cpp=.o) stb_image.o

LIB_DIRS=
VULKAN_LIB_DIRS=$(HOME)/vulkan/1.4.313.0/x86_64/lib

LIBS=SDL2
VULKAN_LIBS=vulkan

OPENGL_DEFINES=VOX_BACKEND_OPENGL
VULKAN_DEFINES=VOX_BACKEND_VULKAN

CC=gcc
CPP=g++
CPP_FLAGS=-g -Wextra -Werror

all: $(OPENGL_NAME)

$(OPENGL_OBJ_DIR)/glad.o: Third-Party/glad/src/glad.c
	$(CC) -g -IThird-Party/glad/include -c $< -o $@

$(OPENGL_OBJ_DIR)/stb_image.o: Third-Party/stb_image/stb_image.c
	$(CC) -g -IThird-Party/stb_image -c $< -o $@

$(OPENGL_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) $(addprefix -D,$(OPENGL_DEFINES)) $(addprefix -I,$(INCLUDE_DIRS) $(OPENGL_INCLUDE_DIRS)) -c $< -o $@

$(VULKAN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) $(addprefix -D,$(VULKAN_DEFINES)) $(addprefix -I,$(INCLUDE_DIRS) $(VULKAN_INCLUDE_DIRS)) -c $< -o $@

$(OPENGL_NAME): $(addprefix $(OPENGL_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(OPENGL_OBJ_DIR)/,$(OPENGL_OBJ_FILES))
	$(CPP) $(addprefix $(OPENGL_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(OPENGL_OBJ_DIR)/,$(OPENGL_OBJ_FILES)) $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS)) -o $@

$(VULKAN_NAME): $(addprefix $(VULKAN_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(VULKAN_OBJ_DIR)/,$(VULKAN_OBJ_FILES))
	$(CPP) $(addprefix $(VULKAN_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(VULKAN_OBJ_DIR)/,$(VULKAN_OBJ_FILES)) $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS)) -o $@

clean:
	rm -rf $(OPENGL_OBJ_DIR)
	rm -rf $(VULKAN_OBJ_DIR)

fclean: clean
	rm -f $(OPENGL_NAME)
	rm -f $(VULKAN_NAME)

re: | fclean all

.PHONY: all clean fclean re
