
##### Settings #####
CC=gcc
CFLAGS=-I .


##### Objects #####

OBJ = backupd


##### Flags #####

# backupd: CFLAGS+=


##### Dependencies #####

DEP_DIR = .dep
DEP_NAMES = $(add_suffix .d,$(OBJ))
DEP = $(add_prefix $(DEP_DIR),$(DEP_NAMES))
include $(DEP)


##### Static targets #####

all: $(OBJ)

%: %.c
	$(call compiling_msg,$<)
	@mkdir -p $(DEP_DIR)
	$(CC) -MD -MP -MF "$(DEP_DIR)/$@.d" -o "$@" "$<" $(CFLAGS)
	@echo

clean:
	$(call redecho,"cleaning\n")
	rm -rf $(DEP_DIR)
	rm -f $(OBJ)


##### Utilities #####

define compiling_msg
	@printf "["
	$(call cyanecho,"Compiling")
	@printf "]: $1\n"
endef

define redecho
	$(call colorecho,1,$1)
endef

define greenecho
	$(call colorecho,2,$1)
endef

define yellowecho
	$(call colorecho,3,$1)
endef

define blueecho
	$(call colorecho,4,$1)
endef

define magentaecho
	$(call colorecho,5,$1)
endef

define cyanecho
	$(call colorecho,6,$1)
endef

define colorecho
      @tput setaf $1
      @printf $2
      @tput sgr0
endef

