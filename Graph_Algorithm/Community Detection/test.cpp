#include <iostream>
#include <fstream>

using namespace std;

int main() {
  int delta[34][34] = {0};
  int edge_sum = 0;
  int comm[34];
  int k[34] = {0};

  ifstream club, answer;
  club.open("./club.txt", ios::in);
  answer.open("./graphx_answer.txt", ios::in);

  int _startid, _endid;
  for(int i = 0; i < 78; i++){
    club >> _startid >> _endid;
    cout << _startid << " ___ " << _endid << endl;
    delta[_startid][_endid] = 1;
    delta[_endid][_startid] = 1;
    k[_startid]++;
    k[_endid]++;
    edge_sum++;
  }
  cout << "edge_sum: " << edge_sum << endl;

  cout << endl << endl;

  int _vid, _cid;
  for (int i = 0; i < 34; i++) {
    answer >> _vid >> _cid;
    cout << _vid << " ___" << _cid << endl;
    comm[_vid] = _cid;
  }

  double Q = 0;

  for (int i = 0; i < 34; i++) {
    for (int j = 0; j < 34; j++) {
      if (comm[i] != comm[j] | i == j) continue;
      Q += (delta[i][j]*1.0 - 1.0*k[i]*k[j]/(2.0*edge_sum)*1.0);
      //cout << "i , j, delta[i][j], k[i], k[j] : " << i << " " << j << " " << delta[i][j] << " " << k[i] << " " << k[j] << endl;
      cout << "current Q : " << Q << endl;
    }
  }

  Q = Q / (2.0*edge_sum);

  cout << "Q : " << Q << endl;

}
