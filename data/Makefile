BUILDER = ../builder/builder

OBJ_SRC = $(wildcard *.obj)
OBJ_TARGETS = $(OBJ_SRC:.obj=.model)

DAE_SRC = $(wildcard *.dae)
DAE_TARGETS = $(DAE_SRC:.dae=.model)

all:  obj_models dae_models

rebuild: $(BUILDER)
	echo 'performing clean rebuild'
	@make clean
	@make all

obj_models: $(OBJ_TARGETS)

dae_models: $(DAE_TARGETS)

%.model: %.dae
	$(BUILDER) $< $@

%.model: %.obj
	$(BUILDER) $< $@

clean: clean_obj_targets clean_dae_targets

clean_obj_targets:
	@rm $(OBJ_TARGETS) 

clean_dae_targets:
	@rm $(DAE_TARGETS) 
