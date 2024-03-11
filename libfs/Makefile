lib := libfs.a

all: $(lib)

CC := gcc
CFLAGS := -Wall -Wextra -Werror -g

OBJS := fs.o disk.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(lib): $(OBJS)
	ar rcs $@ $^


all: $(lib)

clean:
	rm -f $(lib) $(OBJS)
