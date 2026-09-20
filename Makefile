CC = clang
CFLAGS = -Wall -Wextra -Werror -std=c99 -g

srcs = src/main.c \
       src/checksum.c \
       src/time.c

objdir = obj
objs = $(srcs:src/%.c=$(objdir)/%.o)
deps = $(objs:.o=.d)

$(objdir)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

all: ft_ping

-include $(deps)

ft_ping: $(objs)
	$(CC) $(CFLAGS) $(objs) -lm -o ft_ping

clean:
	rm -rf $(objdir)

re: fclean all

fclean: clean
	rm -f ft_ping

vm-setup:
	vm/setup.sh

vm:
	vm/run.sh

vm-stop:
	vm/stop.sh

vm-ssh:
	vm/ssh.sh

vm-clean:
	vm/clean.sh

.PHONY: all clean re fclean vm-setup vm vm-stop vm-ssh vm-clean
