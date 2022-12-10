RVMPARSER_SRC_DIR = ../src
LIBTESS2_SRC_DIR = ../libs/libtess2/Source
CCFLAGS  += -Wall -O2 -I../libs/rapidjson/include -I../libs/libtess2/Include/
CXXFLAGS += -Wall -O2 -I../libs/rapidjson/include -I../libs/libtess2/Include/ -std=c++17
OBJDIR = obj

RVMPARSER_SRC = $(wildcard $(RVMPARSER_SRC_DIR)/*.cpp)
RVMPARSER_OBJ = $(patsubst $(RVMPARSER_SRC_DIR)/%.cpp, $(OBJDIR)/%.o, $(RVMPARSER_SRC))

LIBTESS2_SRC = $(wildcard $(LIBTESS2_SRC_DIR)/*.c)
LIBTESS2_OBJ = $(patsubst $(LIBTESS2_SRC_DIR)/%.c, $(OBJDIR)/%.o, $(LIBTESS2_SRC))

.PHONY: all objdir clean

all: objdir rvmparser

rvmparser: $(RVMPARSER_OBJ) $(LIBTESS2_OBJ)
	$(CXX)  $(LDFLAGS) -o $@ $^

$(RVMPARSER_OBJ): $(OBJDIR)/%.o : $(RVMPARSER_SRC_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(LIBTESS2_OBJ): $(OBJDIR)/%.o : $(LIBTESS2_SRC_DIR)/%.c
	$(CC) -c $(CCFLAGS) $< -o $@

objdir:
	@mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) rvmparser
