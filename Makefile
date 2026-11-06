
# === CONFIGURATION ===
NAME        = ft_ping
CC          = cc
CFLAGS      = -Wall -Wextra -Werror -g3 -I includes/

# === COULEURS ===
RESET       = \033[0m
GREEN       = \033[32m
RED         = \033[31m
YELLOW      = \033[33m

# === REPERTOIRES ===
SRC_DIR     = src
OBJ_DIR     = obj
INC_DIR     = includes

# === SOURCES ===
# Mise a jour automatique via make update
SRC         = \
	src/checksum.c \
	src/main.c \
	src/packet.c \
	src/parser.c \
	src/ping_loop.c \
	src/receive_packet.c \
	src/resolve_target.c \
	src/socket.c
OBJ         = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# === PROGRESS BAR ===
TOTAL_FILES = $(words $(SRC))
COMPILED    = 0

define update_progress
	@$(eval COMPILED=$(shell echo $$(($(COMPILED)+1))))
	@printf "\r$(RED)Compiling [%-50s] %d%% (%d/%d)$(RESET)" \
		"$$(printf '█%.0s' $$(seq 1 $$(($(COMPILED)*50/$(TOTAL_FILES)))))" \
		$$(($(COMPILED)*100/$(TOTAL_FILES))) \
		$(COMPILED) \
		$(TOTAL_FILES)
endef

# === REGLES ===

all: update $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	@echo "\n$(GREEN)Compilation terminee : $(NAME)$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	$(update_progress)

update:
	@echo "$(YELLOW)Mise a jour des sources...$(RESET)"
	@find $(SRC_DIR) -type f -name '*.c' | sort | \
		awk '{print "\t" $$0 " \\"}' | \
		sed '$$ s/ \\$$//' > .sources_block
	@echo "SRC         = \\" > .sources_full
	@cat .sources_block >> .sources_full
	@awk ' \
		FNR==NR { lines[NR] = $$0; next } \
		/^SRC[[:space:]]*=/{ \
			for (i=1; i<=length(lines); i++) print lines[i]; \
			in_block=1; next \
		} \
		in_block && /^[^[:space:]]/ { in_block=0 } \
		!in_block { print } \
	' .sources_full Makefile > Makefile.tmp
	@mv Makefile.tmp Makefile
	@rm -f .sources_block .sources_full
	@echo "$(GREEN)Makefile mis a jour$(RESET)"

test:
	@bash ./ft_ping_tester.sh

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(YELLOW)Fichiers objets supprimes$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)Executable supprime$(RESET)"

re: fclean all

.PHONY: all clean fclean re update test
