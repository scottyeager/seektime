EXEC = seektime
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

# compiling without optimization
# we don't need real optimization and we
# avoid lot of fake report caused by
# possible optimization
CFLAGS += -g -std=gnu99 -O0 -W -Wall
LDFLAGS += -static

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o

mrproper: clean
	$(RM) $(EXEC)
