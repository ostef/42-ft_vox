UNAME=$(shell uname)

OPENGL_NAME=vox-gl
VULKAN_NAME=vox-vk
METAL_NAME=vox-metal

ifeq ($(UNAME), Linux)

NAME=$(OPENGL_NAME)

else ifeq ($(UNAME), Darwin)

NAME=$(METAL_NAME)

endif

SRC_DIR=Source

SRC_FILES=main.cpp core.cpp math.cpp input.cpp noise.cpp world.cpp ui.cpp \
	Graphics/shader_preprocessor.cpp Graphics/shader.cpp Graphics/renderer.cpp Graphics/mesh.cpp Graphics/shadow_map.cpp Graphics/ui.cpp

OPENGL_SRC_FILES=Graphics/OpenGL/opengl.cpp \
	Graphics/OpenGL/render_pass.cpp \
	Graphics/OpenGL/copy_pass.cpp \
	Graphics/OpenGL/pipeline_state.cpp \
	Graphics/OpenGL/shader.cpp \
	Graphics/OpenGL/texture.cpp \
	Graphics/OpenGL/buffer.cpp

METAL_SRC_FILES=Graphics/Metal/metal.cpp \
	Graphics/Metal/render_pass.cpp \
	Graphics/Metal/copy_pass.cpp \
	Graphics/Metal/pipeline_state.cpp \
	Graphics/Metal/shader.cpp \
	Graphics/Metal/texture.cpp \
	Graphics/Metal/buffer.cpp


DEP_DIR=.deps
DEP_FILES=$(SRC_FILES:%.cpp=$(DEP_DIR)/%.d) $(OPENGL_SRC_FILES:%.cpp=$(DEP_DIR)/%.d) $(METAL_SRC_FILES:%.cpp=$(DEP_DIR)/%.d)

INCLUDE_DIRS=Source Third-Party/stb_image

ifeq ($(UNAME), Linux)
INCLUDE_DIRS += /usr/include/SDL2
else ifeq ($(UNAME), Darwin)
INCLUDE_DIRS += /opt/homebrew/include /opt/homebrew/include/SDL2
endif

OPENGL_INCLUDE_DIRS=Third-Party/glad/include
VULKAN_INCLUDE_DIRS=$(HOME)/vulkan/1.4.313.0/x86_64/include
METAL_INCLUDE_DIRS=Third-Party/metal-cpp

OPENGL_OBJ_DIR=Obj/OpenGL
VULKAN_OBJ_DIR=Obj/Vulkan
METAL_OBJ_DIR=Obj/Metal

OBJ_FILES=$(SRC_FILES:.cpp=.o)
OPENGL_OBJ_FILES=$(OPENGL_SRC_FILES:.cpp=.o) glad.o stb_image.o
VULKAN_OBJ_FILES=$(VULKAN_SRC_FILES:.cpp=.o) stb_image.o
METAL_OBJ_FILES=$(METAL_SRC_FILES:.cpp=.o) stb_image.o

LIB_DIRS=
VULKAN_LIB_DIRS=$(HOME)/vulkan/1.4.313.0/x86_64/lib
METAL_LIB_DIRS=/opt/homebrew/lib

LIBS=SDL2
VULKAN_LIBS=vulkan
METAL_FRAMEWORKS=Foundation QuartzCore Metal

OPENGL_DEFINES=VOX_BACKEND_OPENGL
VULKAN_DEFINES=VOX_BACKEND_VULKAN
METAL_DEFINES=VOX_BACKEND_METAL _THREAD_SAFE

CC=gcc
CPP=g++
CPP_FLAGS=-g -std=c++17 -Wextra -Werror
DEP_FLAGS=-MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

all: $(NAME)

$(OPENGL_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
$(OPENGL_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.d | $(DEP_DIR) Makefile
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) $(DEP_FLAGS) $(addprefix -D,$(OPENGL_DEFINES)) $(addprefix -I,$(INCLUDE_DIRS) $(OPENGL_INCLUDE_DIRS)) -c $< -o $@

$(VULKAN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | Makefile
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) $(addprefix -D,$(VULKAN_DEFINES)) $(addprefix -I,$(INCLUDE_DIRS) $(VULKAN_INCLUDE_DIRS)) -c $< -o $@

$(METAL_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPP_FLAGS) $(addprefix -D,$(METAL_DEFINES)) $(addprefix -I,$(INCLUDE_DIRS) $(METAL_INCLUDE_DIRS)) -c $< -o $@

$(OPENGL_OBJ_DIR)/glad.o: Third-Party/glad/src/glad.c
	$(CC) -g -IThird-Party/glad/include -c $< -o $@

$(OPENGL_OBJ_DIR)/stb_image.o: Third-Party/stb_image/stb_image.c
	$(CC) -g -IThird-Party/stb_image -c $< -o $@

$(METAL_OBJ_DIR)/stb_image.o: Third-Party/stb_image/stb_image.c
	$(CC) -g -IThird-Party/stb_image -c $< -o $@

$(OPENGL_NAME): $(addprefix $(OPENGL_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(OPENGL_OBJ_DIR)/,$(OPENGL_OBJ_FILES))
	$(CPP) $(addprefix $(OPENGL_OBJ_DIR)/,$(OBJ_FILES) $(OPENGL_OBJ_FILES)) $(addprefix -L,$(LIB_DIRS) $(OPENGL_LIB_DIRS)) $(addprefix -l,$(LIBS) $(OPENGL_LIBS)) -o $@

$(VULKAN_NAME): $(addprefix $(VULKAN_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(VULKAN_OBJ_DIR)/,$(VULKAN_OBJ_FILES))
	$(CPP) $(addprefix $(VULKAN_OBJ_DIR)/,$(OBJ_FILES) $(VULKAN_OBJ_FILES)) $(addprefix -L,$(LIB_DIRS) $(VULKAN_LIB_DIRS)) $(addprefix -l,$(LIBS) $(VULKAN_LIBS)) -o $@

$(METAL_NAME): $(addprefix $(METAL_OBJ_DIR)/,$(OBJ_FILES)) $(addprefix $(METAL_OBJ_DIR)/,$(METAL_OBJ_FILES))
	$(CPP) $(addprefix $(METAL_OBJ_DIR)/,$(OBJ_FILES) $(METAL_OBJ_FILES)) $(addprefix -L,$(LIB_DIRS) $(METAL_LIB_DIRS)) $(addprefix -l,$(LIBS) $(METAL_LIBS)) $(addprefix -framework ,$(METAL_FRAMEWORKS)) -o $@

$(DEP_DIR)/%.d: ; @mkdir -p $(@D)

$(DEP_FILES):

include $(wildcard $(DEP_FILES))

clean:
	rm -rf $(OPENGL_OBJ_DIR)
	rm -rf $(VULKAN_OBJ_DIR)
	rm -rf $(METAL_OBJ_DIR)

fclean: clean
	rm -f $(OPENGL_NAME)
	rm -f $(VULKAN_NAME)
	rm -f $(METAL_NAME)

re: | fclean all

.PHONY: all clean fclean re
