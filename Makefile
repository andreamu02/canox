CC = gcc
CFLAGS = -Wall -Wextra -O2

# Sorgenti (tutti i .c nella directory)
SRC = $(wildcard *.c)

# Oggetti nella directory build/
OBJDIR = build
OBJ = $(patsubst %.c,$(OBJDIR)/%.o,$(SRC))

TARGET = attack

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

# Regola generale per compilare .c → build/.o
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean

