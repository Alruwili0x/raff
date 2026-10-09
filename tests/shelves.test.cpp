#include "../src/shelves.hpp"
#include <cstdio>
#include <cstdlib>
void check(bool condition,const char*message){if(!condition){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int main(){
 raff::Shelves s;s.reset({{"empty","Empty",{}}},0);check(s.rows.empty()&&s.selected()==-1,"empty results safe");check(s.move(0,-1),"empty Up reaches navigation");
 std::vector<int> all;for(int n=0;n<1000;n++)all.push_back(n);
 s.reset({{"featured","Featured",{8,14,22}},{"allgames","All",all}},999);
 check(s.active==1&&s.selected()==999,"restored last game remains reachable");check(s.rows[1].offset+s.viewport>=999*s.pitch+s.cardWidth-.01f,"last cover is entirely visible");
 s.move(1,0);check(s.selected()==999,"right edge clamped");s.move(0,0,-1);check(s.selected()==993,"trigger pages six covers");
 s.move(0,-1);check(s.selected()==8&&s.active==0,"Up selects preceding shelf");s.move(1,0);check(s.selected()==14,"right selects adjacent cover");s.move(0,1);check(s.selected()==993,"each shelf retains its own cursor");
 s.move(0,-1);check(s.move(0,-1),"top Up hands focus to platform bar");check(s.selected()==14,"platform handoff retains selected game");
 s.reset({{"featured","Featured",{8,14,22}},{"allgames","All",all}},14,"allgames");check(s.active==1&&s.selected()==14,"duplicate across shelves respects saved shelf");
 s.reset({{"featured","Featured",{8,14,22}},{"allgames","All",all}},s.selected(),s.rows[s.active].key);check(s.active==1&&s.selected()==14,"rebuilding retains a shelf ID owned by the old rows");
 for(int n=0;n<2000;n++)s.move(0,0,-1);check(s.selected()==0,"repeated left pages stop at first game");s.animate(.016f,true);check(s.rows[1].offset==0,"reduced motion snaps to final position");
 s.reset({{"small","Small",{5,9}}},100);check(s.selected()==5,"removed game falls back safely");check(s.target(s.rows[0])==0,"short rows never scroll");
 s.reset({{"all","All",all}},0);s.move(0,0,1);s.animate(1.f/60);check(s.rows[0].offset>0&&s.rows[0].offset<s.target(s.rows[0]),"animated scroll advances without jumping");
 for(int n=0;n<1000;n++)s.animate(1.f/60);check(std::abs(s.rows[0].offset-s.target(s.rows[0]))<.01,"animated scroll settles");
 check(raff::Grid::move(0,0,0,1)==0,"empty grid safe");
 check(raff::Grid::move(5,20,1,0)==5,"right does not wrap into next row");
 check(raff::Grid::move(6,20,-1,0)==6,"left does not wrap into previous row");
 check(raff::Grid::move(17,20,0,1)==19,"short last row stays reachable");
 check(raff::Grid::move(19,20,0,0,1)==19,"grid page clamps at last game");
 check(raff::Grid::move(2,20,0,0,1)==14,"grid page preserves column");
 check(raff::Grid::first(19)==12,"last grid row remains in viewport");
 std::puts("PASS: empty results, restoration, duplicates, row memory, paging, edges, final-card visibility, reduced motion, animation");
}
