TARGET_NAME	=	gl_player

CC			=	g++
CC_FLAGS	=	-Wall -Werror -Wextra -g
DEL_DIR		=	rm -rf
DEL			=	rm -f

INC_PATH	=	./inc/ ./obj/
OBJ_PATH	=	./obj/
SRC_PATH	=	./srcs/

OBJ			=	$(addprefix $(OBJ_PATH),$(OBJ_NAME))
GCH			=	$(addprefix $(OBJ_PATH),$(GCH_NAME))

INC			=	$(addprefix -I,$(INC_PATH))

GCH_NAME	=	$(HEADERS:.h=.h.gch)

OBJ_NAME	=	$(SRC_NAME:.cpp=.o)

SRC_NAME	=	main.cpp		\
				Texture.cpp

HEADERS		=	glut.h

LINKER_OPTS	= 	-lGL -lGLU -lglut -lpng

all: $(TARGET_NAME)

test: $(GCH)

$(TARGET_NAME): $(GCH) $(OBJ)
	@$(CC) -o $(TARGET_NAME) $(OBJ) $(LINKER_OPTS)
	@echo "Linking" [ $(TARGET_NAME) ]

$(OBJ_PATH)%.h.gch: ./inc/%.h
	@mkdir -p $(OBJ_PATH)
	@$(CC) $(CC_FLAGS) -o $@ -c $< $(INC)
	@echo "Compiling" [ $< ]

$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp
	@mkdir -p $(OBJ_PATH)
	@$(CC) $(CC_FLAGS) -o $@ -c $< $(INC)
	@echo "Compiling" [ $< ]

clean:
	@$(DEL_DIR) $(OBJ_PATH)
	@echo "Cleaning obj [ $(TARGET_NAME) ]..."

fclean:	clean
	@$(DEL) $(TARGET_NAME)
	@echo "Remove..." [ $(TARGET_NAME) ]

re: fclean all

.PHONY: all clean fclean re

.NOTPARALLEL: all clean fclean re