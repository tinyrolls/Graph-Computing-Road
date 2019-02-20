#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char const *argv[]) {
  
  ifstream origin_G(argv[1], ios::in);
  ofstream new_G(argv[2], ios::out);
  string temp;
  for (int i = 0; i < 4; i++) {
    getline(origin_G, temp);
  }
  while (!origin_G.eof()) {
    getline(origin_G, temp);
    if (temp.length() == 0) continue;
    new_G << temp << endl;
  }
  origin_G.close();
  new_G.close();
  return 0;
}
