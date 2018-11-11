cxxfiles=$(filter %.cpp,$(shell ls))
objs=$(subst .cpp,.o,$(cxxfiles))
CXXFLAGS=-std=c++11 -O2


all:$(objs)
	g++ $(CXXFLAGS) $(objs) -o toyLisp

$(objs): %.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -rf *.o
