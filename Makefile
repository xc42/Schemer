cxxfiles=$(filter %.cpp,$(shell ls))
objs=$(subst .cpp,.o,$(cxxfiles))
CXXFLAGS=-std=c++17 -g


all:$(objs)
	g++ $(CXXFLAGS) $(objs) -o toyLisp

$(objs): %.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -rf *.o
