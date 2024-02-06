GCC    = gcc
OBJDIR = objects
CFLAGS = -Wall -g
LFLAGS = -lGL -lGLU -lglfw -lGLEW -lm -lnetcdf
EXNAME = glEBM

FILES  = $(patsubst %, $(OBJDIR)/%, $(subst .c,.o,$(wildcard *.c)))
FILES += $(patsubst %, $(OBJDIR)/%, $(subst .c,.o,$(wildcard process/*.c)))

all: $(FILES)
	$(GCC) -o $(EXNAME) $(FILES) $(LFLAGS)
	
clean:
	-rm $(EXNAME)
	-rm $(OBJDIR) -r
	
objdir:
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/process
	
$(OBJDIR)/%.o : %.c objdir
	$(GCC) $(CFLAGS) -o $@ -c $<

