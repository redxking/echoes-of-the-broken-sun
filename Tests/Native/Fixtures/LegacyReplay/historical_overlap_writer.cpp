#include "EchoesSimCore/Simulation.h"
#include <fstream>
#include <iostream>
using namespace echoes::sim;
int main(int argc,char**argv){
 if(argc!=2)return 2;
 SimulationConfig config;config.mapWidthTiles=16;config.mapHeightTiles=16;
 Simulation sim(config);if(!sim.AddPlayer(0,Faction::MeridianCompact,{1000,1000})||!sim.AddPlayer(1,Faction::MeridianCompact,{1000,1000}))return 3;
 auto core=sim.SpawnEntity(0,Faction::MeridianCompact,EntityType::CommandCore,Vec2::FromTiles(3,3));
 auto enemy=sim.SpawnEntity(1,Faction::MeridianCompact,EntityType::CommandCore,Vec2::FromTiles(12,12));
 auto worker=sim.SpawnEntity(0,Faction::MeridianCompact,EntityType::Worker,Vec2::FromTiles(7,3));
 if(!core||!enemy||!worker)return 4;
 sim.CaptureReplayBaseline();
 Command produce;produce.executeTick=0;produce.player=0;produce.sequence=1;produce.type=CommandType::Produce;produce.actor=core;produce.buildType=EntityType::Worker;
 Command move;move.executeTick=0;move.player=0;move.sequence=2;move.type=CommandType::Move;move.actor=worker;move.position=Vec2::FromTiles(7,7);
 std::string error;if(!sim.QueueCommand(produce,&error)||!sim.QueueCommand(move,&error)){std::cerr<<error;return 5;}
 Command busy=produce;busy.executeTick=1;busy.sequence=3;if(!sim.QueueCommand(busy,&error))return 7;
 sim.Step(100);auto replay=sim.ExportReplay(&error);auto restored=Simulation::ReplayToEnd(replay,&error);if(!restored){std::cerr<<error;return 6;}
 std::ofstream f(argv[1],std::ios::binary);f.write(reinterpret_cast<const char*>(replay.initialSnapshot.data()),replay.initialSnapshot.size());f.close();
 std::cout<<"{\"snapshot_version\":"<<kSnapshotVersion<<",\"replay_version\":"<<kReplayVersion<<",\"final_tick\":"<<replay.finalTick<<",\"final_checksum\":"<<replay.finalChecksum<<",\"core\":"<<core<<",\"worker\":"<<worker<<",\"commands\":3}";
 return 0;
}