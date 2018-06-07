// CrystalLighting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// C++11
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <string>

using namespace std;

class CrystalLighting {
public:
    vector<string> placeItems( vector<string> targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles ) {
        return { "0 7 2", "9 5 1" };
    }
};
// -------8<------- end of solution submitted to the website -------8<-------

template<class T> void getVector( vector<T>& v ) {
    for ( int i = 0; i < v.size(); ++i )
        cin >> v[ i ];
}

int main() {
    CrystalLighting cl;
    int H;
    cin >> H;
    vector<string> targetBoard( H );
    getVector( targetBoard );
    int costLantern, costMirror, costObstacle, maxMirrors, maxObstacles;
    cin >> costLantern >> costMirror >> costObstacle >> maxMirrors >> maxObstacles;

    vector<string> ret = cl.placeItems( targetBoard, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles );
    cout << ret.size() << endl;
    for ( int i = 0; i < (int)ret.size(); ++i )
        cout << ret[ i ] << endl;
    cout.flush();
}

