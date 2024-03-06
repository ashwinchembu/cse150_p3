# Target library
lib := libfs.a

all: $(lib)

CC := gcc
CFLAGS := -Wall -Werror -g

# Object files
OBJS := fs.o disk.o

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(lib): $(OBJS)
	ar rcs $@ $^


all: $(lib)

clean:
	rm -f $(lib) $(OBJS)
