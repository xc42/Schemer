#include "front-end-pass.h"

namespace FrontEndPass
{
using namespace std;
using namespace Parser;

static const auto FrontEndAllPasses = {CollectAssign::run};

Result<unique_ptr<Program>> parseProg(const Range& rg)
{
	static auto Defs = MaybeMany(parseDef);
	static auto prog = All(Defs, parseExp) >>
		[](vector<unique_ptr<Define>>&& defs, Expr::Ptr&& body)
		{
			vector<Expr::Ptr> arg;
			arg.emplace_back(std::move(body));
			defs.emplace_back(
				make_unique<Define>(
					Var("main"), 
					make_unique<Lambda>(
						vector<Var>{}, 
						make_unique<Apply>( make_unique<Var>("display"), std::move(arg)))));

			return make_unique<Program>(std::move(defs));
		};
	return prog(rg);
}

std::string gensym(const std::string& s)
{
	static int cnt = 0;
	return s + std::to_string(cnt++);
}

void  runAllPass(Program& p)
{
	for(const auto& ps: FrontEndAllPasses) {
		ps(p);
	}
}

}//namespace FrontEndPass
