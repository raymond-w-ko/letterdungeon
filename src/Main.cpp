#include "PrecompiledHeaders.hpp"
#include "Game/Game.hpp"

int main(int argc, char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  
  // change to directory where media and assets are located
  auto path = boost::filesystem::current_path();
  boost::filesystem::current_path(path / "..");
  
  Game game;
  game.Start();
  game.Loop();
  
  return 0;
}
