#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main() {
  ofstream vertexs, edges;
  // vertexs.open("./vertexs.txt", ios::out);
  edges.open("./edges.txt", ios::out);

  bool delta[1000][1000] = {false};

  srand((unsigned)time(NULL));
  for (int i = 0; i < 1000; i++) {
    for (int j = 0; j < (1000 - i - rand()%(1000-i)); j++) {
      int first = i;
      int second = i + rand()% (1000 - i);
      if (first == second || delta[first][second] == true) continue;
      else {
        delta[first][second] = true;
        delta[second][first] = true;
        edges << first << " " << second << "\n";
      }
    }
  }

  edges.close();
  return 0;
}
