.PHONY: all clean cvars

CXX=g++-8
SDIR=src
ODIR=obj

SRCS=$(wildcard $(SDIR)/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Demo/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Hud/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Routing/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Speedrun/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Speedrun/Rules/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Stats/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/ReplaySystem/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Tas/*.cpp)
SRCS+=$(wildcard $(SDIR)/Features/Timer/*.cpp)
SRCS+=$(wildcard $(SDIR)/Games/Linux/*.cpp)
SRCS+=$(wildcard $(SDIR)/Modules/*.cpp)
SRCS+=$(wildcard $(SDIR)/Utils/*.cpp)

OBJS=$(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(SRCS))

# Header dependency target files; generated by g++ with -MMD
DEPS=$(OBJS:%.o=%.d)

# Import config.mk, which provides PKGCONFIG_*FLAGS variables
include config.mk

WARNINGS=-Wall -Wno-unused-function -Wno-unused-variable -Wno-parentheses -Wno-unknown-pragmas -Wno-register -Wno-sign-compare
CXXFLAGS=-std=c++17 -m32 $(WARNINGS) -I$(SDIR) $(PKGCONFIG_CXXFLAGS) -fPIC
LDFLAGS=-m32 -shared -lstdc++fs $(PKGCONFIG_LDFLAGS)

all: sar.so
clean:
	rm -rf $(ODIR) sar.so

-include $(DEPS)

sar.so: $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

cvars: doc/cvars.md
doc/cvars.md:
	node cvars.js "$(STEAM)Portal 2"
